#include "util/shine_define.h"

#ifdef SHINE_PLATFORM_WASN

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES/gl2.h>

static void update()
{
    auto t = emscripten_get_now();
    glClear(GL_COLOR_BUFFER_BIT);
}

int main(int argc,char** argv)
{

	EmscriptenWebGLContextAttributes  attrs;
	attrs.alpha = false;
    attrs.depth = true;
    attrs.stencil = true;
    attrs.antialias = true;
    attrs.premultipliedAlpha = false;
    attrs.preserveDrawingBuffer = false;
    attrs.preferLowPowerToHighPerformance = false;
    attrs.failIfMajorPerformanceCaveat = false;
    attrs.majorVersion = 1;
    attrs.minorVersion = 0;
    attrs.enableExtensionsByDefault = false;

	int ctx = emscripten_webgl_create_context(0, &attrs);
    if(!ctx)
    {
        printf("Webgl ctx could not be created!\n");
        return -1;
    }    

    emscripten_webgl_make_context_current(ctx);
    glClearColor(0,0,1,1);
    glClear(GL_COLOR_BUFFER_BIT);


	emscripten_set_main_loop(WasmMainLoop, 0, 1);

    return 1;
}



#else



#include "file_util.h"




#ifdef WITH_EDITOR

#endif

#include <array>
#include <filesystem>


#include "imgui.h"


#include "editor/mainEditor.h"
#include "fmt/format.h"

#include "render/backend/render_backend.h"
#include "gameplay/camera.h"


#include "manager/CameraManager.h"
#include "platform/InitWindows.h"
#include "platform/WindowsInfo.h"
#include "timer/function_timer.h"

#include "util/fps_controller.h"


#include "quickjs/quickjs.h"
#include "render/renderManager.h"

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

	windows::InitWindowsPlatform();

	auto& info = windows::WindowsInfo::get().info;

	// camera
	shine::manager::CameraManager::get().setMainCamera(&g_Camera);


	std::array<float, 4> clear_color = { 0.45f, 0.55f, 0.60f, 1.00f };
	

	RenderBackend = render::RenderManager::get().GetRenderBackend();


	shine::editor::main_editor::MainEditor* mainEditor = nullptr;
	mainEditor = new shine::editor::main_editor::MainEditor();
	mainEditor->Init();



	auto& g_FPSManager = util::EngineFPSManager::get();
	bool done = false;
	while (!done) {
		
		// FPS控制 - 帧开始
		g_FPSManager.BeginFrame();
		const double dt_d = g_FPSManager.GetCurrentDeltaTime();
		const float dt = static_cast<float>(dt_d);

		// Poll and handle messages (inputs, window resize, etc.)
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}

		if (done)
			break;

        // 渲染服务，帧开始
        render::RendererService::get().beginFrame();


        // 编辑器UI渲染
        mainEditor->Render();
		
		// 应用摄像机
        manager::CameraManager::get().getMainCamera()->Apply();

        // 统一用渲染服务提供帧缓冲渲染/显示
        render::RendererService::get().endFrame(clear_color);

		// FPS控制 - 帧结束
		g_FPSManager.EndFrame();

	}

	// 清理ImGui
	render::RenderManager::get().GetRenderBackend()->ClearUp(info.hwnd);

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