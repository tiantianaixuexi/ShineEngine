
#ifdef BUILD_EDITOR

#include "editor/editorPlayer/editor_play.h"
#endif

#include "manager/InputManager.h"

#ifdef SHINE_PLATFORM_WASM

#include <GLES/gl2.h>

int main(int argc, char **argv) {

    return 1;
}

#else

#include "file_util.ixx"

#ifdef WITH_EDITOR

#endif

#include <array>
#include <filesystem>

#include "imgui/imgui.h"

#include "fmt/printf.h"

#include "quickjs/quickjs.h"

#include "gameplay/camera.h"
#include "gameplay/tick/tickManager.h"
#include "gameplay/world/world_service.h"

#include "manager/CameraManager.h"

#include "platform/InitWindows.h"
#include "platform/WindowsInfo.h"

#include "EngineCore/engine_context.h"

#include "util/EngineDirectoryService.h"
#include "util/fps_controller.h"
#include "util/watcher/FileWatchService.h"

#include "editor/mainEditor.h"
#include "editor/main_editor/EditorCompositionRoot.h"

#include "render/backend/render_backend.h"
#include "render/debug/pass_texture_manager.h"
#include "script/ScriptSystem.h"

#include "loader/model/objLoader.h"
#include "util/crash_handler/crash_handler.h"

#define TRACY_ENABLE
#include "tracy/tracy/Tracy.hpp"

// Include Memory
#ifdef SHINE_USE_MODULE
import shine.memory;
#else
#include "memory/memory.ixx"
#endif

using namespace shine;

#ifdef SHINE_OPENGL

#endif

shine::render::backend::IRenderBackend *RenderBackend = nullptr; // Non-owning, lifetime managed by RendererService

shine::gameplay::Camera g_Camera("默认相机");

int main(int argc, char **argv) {

    ZoneScoped;

    // Install crash handler AFTER ZoneScoped so Tracy's SetUnhandledExceptionFilter
    // (installed during Tracy profiler init) is overridden by ours.
    shine::util::crash_handler::Install({
        .outputDir  = shine::SString("Logs"),
        .filePrefix = shine::SString("crash"),
    });

    for (int i = 0; i < argc; i++) {
        fmt::println("命令行参数[{}]: {}", i, argv[i]);
    }

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    shine::loader::ObjMesh::read("E:\\kitty mod.obj");

    shine::EngineContext context;

    // Register Subsystems
    context.Register(new windows::WindowsDeviceInfo());
    context.Register(new windows::WindowsInfo());
    context.Register(new input::InputManager());
    context.Register(new util::FPSController());
    context.Register(new util::EngineDirectoryService());
    editor::main_editor::EditorCompositionRoot::RegisterEditorSystems(context);
    context.Register(new manager::CameraManager());
    context.Register(new util::watcher::FileWatchService());
    context.Register(new script::ScriptSystem());

    context.Register(new editor::SEditorPlayer());

    // context.Register(new render::RenderManager());
    context.Register(new render::TextureManager());
    context.Register(new render::PassTextureManager());
    context.Register(new render::RendererService());

    context.Register(new gameplay::tick::TickManager());
    context.Register(new gameplay::world::WorldService());

    if (!context.InitAll()) {
        fmt::println("EngineContext::InitAll 失败");
        return -1;
    }

    windows::InitWindowsPlatform(context);

    auto &info = context.GetSystem<windows::WindowsInfo>()->info;

    // camera
    context.GetSystem<manager::CameraManager>()->setMainCamera(&g_Camera);

    std::array<float, 4> clear_color = {0.45f, 0.55f, 0.60f, 1.00f};

    // RenderBackend = context.GetSystem<render::RenderManager>()->GetRenderBackend();
    RenderBackend = context.GetSystem<render::RendererService>()->GetBackend();

    auto mainEditor = editor::main_editor::EditorCompositionRoot::BuildMainEditor(context);
    mainEditor->Init();

    editor::SEditorPlayer *editorPlayer = context.GetSystem<editor::SEditorPlayer>();
    editorPlayer->init();

    auto  RenderService = context.GetSystem<render::RendererService>();
    auto  Camera        = context.GetSystem<manager::CameraManager>();
    auto* TickManager   = context.GetSystem<gameplay::tick::TickManager>();
    auto* WorldService  = context.GetSystem<gameplay::world::WorldService>();
    auto* FileWatchService = context.GetSystem<util::watcher::FileWatchService>();
    auto& g_FPSManager  = util::FPSController::get();

    bool done = false;
    float frameDeltaTime = 0.0f;

    while (!done) {
        // shine::co::MemoryScope frameScope(shine::co::MemoryTag::Core);

        // FPS控制 - 帧开始
        {
            g_FPSManager.BeginFrame();
            const double dt_d = g_FPSManager.GetDeltaTime();
            frameDeltaTime = static_cast<float>(dt_d * 0.001);
        }

        // Poll and handle messages (inputs, window resize, etc.)
        {
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
        }

        if (done)
            break;

        if (TickManager) {
            TickManager->ExecuteAll(frameDeltaTime);
        }
        if (WorldService) {
            WorldService->tickStreaming();
        }
        if (FileWatchService) {
            FileWatchService->PumpEvents();
        }

        {
            // shine::co::MemoryScope renderScope(shine::co::MemoryTag::Render);
            RenderService->beginFrame();

            // 编辑器UI渲染
            mainEditor->Render();

            shine::co::MemoryScope coreScope(shine::co::MemoryTag::Job);
            Camera->getMainCamera()->Apply();

            RenderService->endFrame(clear_color);
        }

        {
            g_FPSManager.EndFrame();
            // fmt::println("FPS: {:.2f}", g_FPSManager.GetActualFPS());
        }
    }

    // 清理ImGui
    RenderBackend->ClearUp(info.hwnd);

    // 清理编辑器
    mainEditor.reset();

    ::DestroyWindow(info.hwnd);
    //::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}
#endif
