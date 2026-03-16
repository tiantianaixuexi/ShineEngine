#include "crash_handler.h"

// Windows platform guard — entire TU is excluded on non-Windows builds.
#if defined(_WIN32) || defined(_WIN64)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <DbgHelp.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#pragma comment(lib, "DbgHelp.lib")

namespace shine::util::crash_handler
{
    REGISTER_LOG_GROUP_END(CrashHandlerLog)

    // -----------------------------------------------------------------------
    //  Module-local state
    // -----------------------------------------------------------------------
    namespace
    {
        std::atomic<bool>            g_installed{ false };
        CrashHandlerConfig           g_config;
        LPTOP_LEVEL_EXCEPTION_FILTER g_prevExceptionFilter{ nullptr };
        std::terminate_handler       g_prevTerminate{ nullptr };
        bool                         g_symInitialized{ false };

        // -----------------------------------------------------------------------
        //  DbgHelp symbol helpers
        // -----------------------------------------------------------------------

        // Call once at Install() time so PDB files are found before any crash.
        void EnsureSymbols()
        {
            if (g_symInitialized) return;

            char exePath[MAX_PATH]{};
            GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            const auto exeDir    = std::filesystem::path(exePath).parent_path();
            const auto parentDir = exeDir.parent_path();

            // Search in exe dir + its parent (covers build_msvc/Debug next to exe/)
            const std::string searchPath = exeDir.string() + ";" + parentDir.string();

            SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
            g_symInitialized = SymInitialize(GetCurrentProcess(), searchPath.c_str(), TRUE) != FALSE;
        }

        // Resolve a raw address to "FunctionName+0xNN [file:line]" (or module+RVA fallback).
        std::string SymbolizeFrame(DWORD64 addr)
        {
            HANDLE hProcess = GetCurrentProcess();

            alignas(SYMBOL_INFO) char symBuf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};
            auto* sym          = reinterpret_cast<SYMBOL_INFO*>(symBuf);
            sym->SizeOfStruct  = sizeof(SYMBOL_INFO);
            sym->MaxNameLen    = MAX_SYM_NAME;

            std::string result;

            DWORD64 symDisp{};
            if (SymFromAddr(hProcess, addr, &symDisp, sym))
            {
                result = sym->Name;
                if (symDisp > 0)
                    result += fmt::format("+0x{:X}", symDisp);
            }
            else
            {
                // Fallback: module name + RVA
                HMODULE hMod{};
                GetModuleHandleExA(
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCSTR>(static_cast<uintptr_t>(addr)), &hMod);
                char modPath[MAX_PATH]{};
                GetModuleFileNameA(hMod, modPath, MAX_PATH);
                const auto stem = std::filesystem::path(modPath).stem().string();
                result = fmt::format("{}+0x{:X}",
                    stem.empty() ? "?" : stem,
                    addr - reinterpret_cast<DWORD64>(hMod));
            }

            IMAGEHLP_LINE64 lineInfo{};
            lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD lineDisp{};
            if (SymGetLineFromAddr64(hProcess, addr, &lineDisp, &lineInfo))
                result += fmt::format(" [{}:{}]", lineInfo.FileName, lineInfo.LineNumber);

            return result;
        }

        // -----------------------------------------------------------------------
        //  Helpers
        // -----------------------------------------------------------------------
        std::string MakeTimestamp()
        {
            const auto now = std::chrono::system_clock::now();
            const auto tt  = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            localtime_s(&tm, &tt);
            return fmt::format("{:04d}{:02d}{:02d}_{:02d}{:02d}{:02d}",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
        }

        std::string MakeBasePath(const std::string& ext)
        {
            std::filesystem::path dir;
            if (!g_config.outputDir.empty())
            {
                dir = std::filesystem::path(g_config.outputDir.sv());
            }
            else
            {
                // Place dumps next to the executable
                char exePath[MAX_PATH]{};
                GetModuleFileNameA(nullptr, exePath, MAX_PATH);
                dir = std::filesystem::path(exePath).parent_path();
            }

            const std::string prefix = g_config.filePrefix.empty()
                ? "crash"
                : std::string(g_config.filePrefix.sv());

            return (dir / (prefix + "_" + MakeTimestamp() + ext)).string();
        }

        void WriteMiniDump(EXCEPTION_POINTERS* ep)
        {
            if (!g_config.writeMiniDump)
                return;

            const std::string path = MakeBasePath(".dmp");
            HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0,
                                       nullptr, CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile == INVALID_HANDLE_VALUE)
                return;

            MINIDUMP_EXCEPTION_INFORMATION info{
                .ThreadId          = GetCurrentThreadId(),
                .ExceptionPointers = ep,
                .ClientPointers    = FALSE
            };

            MiniDumpWriteDump(
                GetCurrentProcess(),
                GetCurrentProcessId(),
                hFile,
                static_cast<MINIDUMP_TYPE>(
                    MiniDumpWithFullMemoryInfo |
                    MiniDumpWithHandleData     |
                    MiniDumpWithThreadInfo),
                ep ? &info : nullptr,
                nullptr,
                nullptr);

            CloseHandle(hFile);
            SHINE_LOG_ERROR(CrashHandlerLog, "crash", "MiniDump written: {}", path);
        }

        void WriteTextLog(EXCEPTION_POINTERS* ep,
                          void* const* frames, USHORT frameCount)
        {
            if (!g_config.writeTextLog)
                return;

            const std::string path = MakeBasePath(".log");
            std::ofstream log(path);
            if (!log.is_open())
                return;

            log << "=== ShineEngine Crash Report ===\n";
            log << fmt::format("Timestamp: {}\n\n", MakeTimestamp());

            if (ep)
            {
                const EXCEPTION_RECORD* rec = ep->ExceptionRecord;
                log << fmt::format("Exception code:    0x{:08X}\n",
                                   static_cast<unsigned>(rec->ExceptionCode));
                log << fmt::format("Exception address: 0x{:016X}\n",
                                   reinterpret_cast<uintptr_t>(rec->ExceptionAddress));
                log << fmt::format("Exception flags:   0x{:08X}\n\n",
                                   static_cast<unsigned>(rec->ExceptionFlags));
                SHINE_LOG_ERROR(CrashHandlerLog, "crash",
                    "CRASH! code=0x{:08X} addr=0x{:016X} log={}",
                    static_cast<unsigned>(rec->ExceptionCode),
                    reinterpret_cast<uintptr_t>(rec->ExceptionAddress),
                    path);
            }
            else
            {
                log << "Triggered via std::terminate (unhandled C++ exception or "
                       "explicit terminate call).\n\n";
                SHINE_LOG_ERROR(CrashHandlerLog, "crash",
                    "CRASH! std::terminate triggered. log={}", path);
            }

            log << "--- Stack Trace ---\n";
            for (USHORT i = 0; i < frameCount; ++i)
            {
                if (!frames[i]) break;
                log << SymbolizeFrame(
                    static_cast<DWORD64>(reinterpret_cast<uintptr_t>(frames[i]))) << '\n';
            }
        }

        void HandleCrash(EXCEPTION_POINTERS* ep)
        {
            // Refresh symbol table in case DLLs were loaded after Install().
            SymRefreshModuleList(GetCurrentProcess());

            // Capture raw return addresses before WriteMiniDump (which allocates).
            // Skip: HandleCrash(1) + ExceptionFilter/TerminateHandler(1) + user skip.
            constexpr DWORD kMaxFrames = 128;
            void*  frames[kMaxFrames]{};
            const USHORT captured = CaptureStackBackTrace(
                static_cast<DWORD>(2 + g_config.stackSkipFrames),
                kMaxFrames, frames, nullptr);

            WriteMiniDump(ep);
            WriteTextLog(ep, frames, captured);
        }

        // -----------------------------------------------------------------------
        //  Exception filter (SEH — catches access violations, div-by-zero, etc.)
        // -----------------------------------------------------------------------
        LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* ep)
        {
            HandleCrash(ep);
            // Let the OS show its default crash dialog / invoke JIT debugger.
            return EXCEPTION_CONTINUE_SEARCH;
        }

        // -----------------------------------------------------------------------
        //  terminate handler (catches uncaught C++ exceptions, std::terminate)
        // -----------------------------------------------------------------------
        void TerminateHandler()
        {
            HandleCrash(nullptr);
            // Chain to the previously installed handler (usually std::abort).
            if (g_prevTerminate)
                g_prevTerminate();
            else
                std::abort();
        }
    } // anonymous namespace

    // -----------------------------------------------------------------------
    //  Public API
    // -----------------------------------------------------------------------
    void Install(const CrashHandlerConfig& config)
    {
        bool expected = false;
        if (!g_installed.compare_exchange_strong(expected, true))
            return; // already installed

        g_config = config;

        // Create output directory if it does not exist
        if (!g_config.outputDir.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(
                std::filesystem::path(g_config.outputDir.sv()), ec);
        }

        ADD_LOG_CATEGORY_WITH_CONSOLE(CrashHandlerLog, "crash", true)

        // Initialize DbgHelp now so the PDB search path is set before any crash.
        EnsureSymbols();

        g_prevExceptionFilter = SetUnhandledExceptionFilter(ExceptionFilter);
        g_prevTerminate       = std::set_terminate(TerminateHandler);

        SHINE_LOG_INFO(CrashHandlerLog, "crash", "CrashHandler installed. output={}",
            g_config.outputDir.empty() ? "exe dir" : std::string(g_config.outputDir.sv()));
    }

    void Uninstall()
    {
        bool expected = true;
        if (!g_installed.compare_exchange_strong(expected, false))
            return; // not installed

        SetUnhandledExceptionFilter(g_prevExceptionFilter);
        std::set_terminate(g_prevTerminate);
        g_prevExceptionFilter = nullptr;
        g_prevTerminate       = nullptr;

        SHINE_LOG_INFO(CrashHandlerLog, "crash", "CrashHandler uninstalled.");
    }

} // namespace shine::util::crash_handler

#endif // _WIN32 || _WIN64
