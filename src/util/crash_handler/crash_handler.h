#pragma once
// ============================================================
//  crash_handler — Windows SEH + MiniDump + C++23 stacktrace
//
//  Usage:
//      Call Install() once at the start of main().
//      On any unhandled exception or std::terminate, a .dmp
//      and a .log file are written next to the executable.
//
//  Platform: Windows only.
// ============================================================

#include "EngineCore/log/LogSystem.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::util::crash_handler
{
    REGISTER_LOG_GROUP(CrashHandlerLog)
    struct CrashHandlerConfig
    {
        /// Directory where .dmp / .log files are written.
        /// Empty = same directory as the executable.
        SString outputDir;

        /// Base name prefix for crash files (e.g. "MyApp").
        /// Files will be named "<prefix>_YYYYMMDD_HHMMSS.{dmp,log}".
        SString filePrefix = "crash";

        /// Write a MiniDump (.dmp) on crash (requires DbgHelp.dll).
        bool writeMiniDump = true;

        /// Write a plain-text log (.log) with exception info + stacktrace.
        bool writeTextLog = true;

        /// Number of additional stacktrace frames to skip from the top,
        /// beyond the crash_handler internals already skipped internally.
        /// Set to 0 unless you wrap Install() in another layer.
        int stackSkipFrames = 0;
    };

    /// Install global unhandled-exception and std::terminate handlers.
    /// Call exactly once at the beginning of main().
    void Install(const CrashHandlerConfig& config = {});

    /// Uninstall handlers and restore previous ones.
    void Uninstall();

} // namespace shine::util::crash_handler
