
#ifdef BUILD_EDITOR

#include "editor/editorPlayer/editor_play.h"
#endif

#include "manager/InputManager.h"

#ifdef SHINE_PLATFORM_WASN

#include <GLES/gl2.h>



int main(int argc,char** argv)
{

    return 1;
}



#else



#include "file_util.ixx"




#ifdef WITH_EDITOR

#endif

#include <array>
#include <filesystem>


#include "imgui/imgui.h"


#include "editor/mainEditor.h"
#include "fmt/format.h"

#include "render/backend/render_backend.h"
#include "gameplay/camera.h"


#include "manager/CameraManager.h"
#include "platform/InitWindows.h"
#include "platform/WindowsInfo.h"

#include "quickjs/quickjs.h"
// #include "render/renderManager.h"
#include "fps_controller.h"
#include "manager/AssetManager.h"
#include "gameplay/tick/tickManager.h"
#include "EngineCore/engine_context.h"

// Include Memory
#ifdef SHINE_USE_MODULE
import shine.memory;
#else
#include "memory/memory.ixx"
#endif


using namespace shine;

#ifdef SHINE_OPENGL

#endif



shine::render::backend::IRenderBackend* RenderBackend = nullptr;

shine::gameplay::Camera g_Camera("默认相机");


int main(int argc, char** argv) {


	for (int i =0 ;i<argc;i++)
	{
		fmt::println("命令行参数[{}]: {}", i, argv[i]);
	}

#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif


    shine::EngineContext context;

    // Register Subsystems
    context.Register(new windows::WindowsDeviceInfo());
    context.Register(new windows::WindowsInfo());
	context.Register(new input::InputManager());
	context.Register(new util::FPSController());
	context.Register(new manager::AssetManager());
    context.Register(new manager::CameraManager());


	context.Register(new editor::SEditorPlayer());

    // context.Register(new render::RenderManager());
    context.Register(new render::TextureManager());
    context.Register(new render::RendererService());

    context.Register(new gameplay::tick::TickManager());

	windows::InitWindowsPlatform(context);
	

	auto& info = context.GetSystem<windows::WindowsInfo>()->info;

	// camera
	context.GetSystem<manager::CameraManager>()->setMainCamera(&g_Camera);


	std::array<float, 4> clear_color = { 0.45f, 0.55f, 0.60f, 1.00f };
	

	// RenderBackend = context.GetSystem<render::RenderManager>()->GetRenderBackend();
    RenderBackend = context.GetSystem<render::RendererService>()->GetBackend();


	editor::main_editor::MainEditor* mainEditor = nullptr;
	mainEditor = new editor::main_editor::MainEditor(context);
	mainEditor->Init();


	editor::SEditorPlayer* editorPlayer = context.GetSystem<editor::SEditorPlayer>();
	editorPlayer->init();


	auto RenderService = context.GetSystem<render::RendererService>();
	auto Camera = context.GetSystem<manager::CameraManager>();
	auto& g_FPSManager = util::FPSController::get();

	bool done = false;
	while (!done) {
        // shine::co::MemoryScope frameScope(shine::co::MemoryTag::Core);
		
		// FPS控制 - 帧开始
        {
             shine::co::MemoryScope scope(shine::co::MemoryTag::Physics);
		     g_FPSManager.BeginFrame();
		     const double dt_d = g_FPSManager.GetDeltaTime();
		     const float dt = static_cast<float>(dt_d);
        }

		// Poll and handle messages (inputs, window resize, etc.)
        {
            shine::co::MemoryScope scope(shine::co::MemoryTag::AI);
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

        // 渲染服务，帧开始
        {
             shine::co::MemoryScope renderScope(shine::co::MemoryTag::Render);
		     RenderService->beginFrame();
        }


        // 编辑器UI渲染
        {
             shine::co::MemoryScope editorScope(shine::co::MemoryTag::Render);
             mainEditor->Render();
        }
		
		// 应用摄像机
        {
             shine::co::MemoryScope coreScope(shine::co::MemoryTag::Job);
		     Camera->getMainCamera()->Apply();
        }

        // 统一用渲染服务提供帧缓冲渲染/显示
        {
             shine::co::MemoryScope renderScope(shine::co::MemoryTag::Render);
		     RenderService->endFrame(clear_color);
        }

		// FPS控制 - 帧结束
        {
             shine::co::MemoryScope scope(shine::co::MemoryTag::Physics);
		     g_FPSManager.EndFrame();
        }

	}

	// 清理ImGui
	RenderBackend->ClearUp(info.hwnd);

	// 清理编辑器
	if (mainEditor)
	{
		delete mainEditor;
		mainEditor = nullptr;
	}

	::DestroyWindow(info.hwnd);
	//::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}
#endif