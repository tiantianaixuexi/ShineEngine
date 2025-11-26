#include <iostream>
#include <Windows.h>
#include <string>

#ifdef LAUNCHER_BUILD
#include <filesystem>
namespace fs = std::filesystem;

// ImGui includes
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"

// OpenGL includes
#include <GL/glew.h>
#include <GL/wglew.h>

// Launcher GUI
#include "launcher_gui.h"

namespace shine::launcher
{
    extern std::unique_ptr<LauncherGUI> g_Launcher;
}

// Window procedure for the launcher
LRESULT WINAPI LauncherWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            // Handle window resize
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main(int argc, char* argv[])
{
    std::cout << "ShineEngine Launcher v1.0.0" << std::endl;

    // Check if we should skip GUI and launch directly
    bool skipGUI = false;
    std::string directProjectPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-gui" || arg == "--launch") {
            skipGUI = true;
        } else if (arg == "--project" && i + 1 < argc) {
            directProjectPath = argv[i + 1];
            skipGUI = true;
            ++i;
        }
    }

    if (skipGUI) {
        // Original command-line behavior
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

        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        std::string command = "\"" + exePath.string() + "\"";
        if (!directProjectPath.empty()) {
            command += " --project \"" + directProjectPath + "\"";
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
    }

    // GUI Mode
    std::cout << "Starting GUI launcher..." << std::endl;

    // Register window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, LauncherWndProc, 0L, 0L,
                     GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                     "ShineEngine Launcher", NULL };
    RegisterClassEx(&wc);

    // Create window
    HWND hwnd = CreateWindow(wc.lpszClassName, "ShineEngine Launcher",
                           WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800,
                           NULL, NULL, wc.hInstance, NULL);

    if (!hwnd) {
        std::cout << "Failed to create window!" << std::endl;
        return 1;
    }

    // Initialize OpenGL
    HDC hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 8, 0,
        PFD_MAIN_PLANE, 0, 0, 0, 0
    };

    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    HGLRC hrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hrc);

    // Initialize GLEW
    glewInit();

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup ImGui backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Setup fonts - use Chinese font for better readability
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f);
    if (!font) {
        // Try alternative Chinese fonts
        font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\simhei.ttf", 18.0f);
        if (!font) {
            font = io.Fonts->AddFontDefault();
        }
    }

    // Initialize launcher GUI
    shine::launcher::g_Launcher = std::make_unique<shine::launcher::LauncherGUI>();
    shine::launcher::g_Launcher->Init(nullptr); // We don't need the full render backend for launcher

    // Show window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render launcher GUI
        shine::launcher::g_Launcher->Render();

        // Render ImGui
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.15f, 0.15f, 0.15f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SwapBuffers(hdc);
    }

    // Cleanup
    shine::launcher::g_Launcher->Shutdown();
    shine::launcher::g_Launcher.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hrc);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;

#else
    std::cout << "This is a test launcher build." << std::endl;
    std::cout << "To use the full engine, please run: build.bat run" << std::endl;
    return 0;
#endif
}


