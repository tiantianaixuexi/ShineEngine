#include <iostream>
#include <Windows.h>
#include <string>

#ifdef LAUNCHER_BUILD
#include <filesystem>
namespace fs = std::filesystem;
#endif

int main(int argc, char* argv[])
{
    std::cout << "ShineEngine Launcher v1.0.0" << std::endl;

#ifdef LAUNCHER_BUILD
    std::cout << "Running in launcher mode..." << std::endl;

    // 鏌ユ壘 MainEngine.exe
    fs::path exePath = fs::current_path() / "exe" / "MainEngine.exe";
    if (!fs::exists(exePath)) {
        exePath = fs::current_path() / "exe" / "MainEngined.exe";
    }

    if (!fs::exists(exePath)) {
        std::cout << "Error: MainEngine executable not found!" << std::endl;
        std::cout << "Please run 'build.bat run' first to build the engine." << std::endl;
        return 1;
    }

    std::cout << "Launching: " << exePath.string() << std::endl;

    // 鍚姩涓诲紩鎿?
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    std::string command = "\"" + exePath.string() + "\"";
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            command += " \"";
            command += argv[i];
            command += "\"";
        }
    }

    if (CreateProcess(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 0;
    } else {
        std::cout << "Failed to launch MainEngine.exe" << std::endl;
        return 1;
    }

#else
    std::cout << "This is a test launcher build." << std::endl;
    std::cout << "To use the full engine, please run: build.bat run" << std::endl;
    return 0;
#endif
}


