#include "file_util.h"
#include "util/shine_define.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef  SHINE_PLATFORM_WIN64


#include <windows.h>
#include <shellapi.h>
#include <GL/glew.h>

#endif



#ifdef WITH_EDITOR

#endif

#include <imgui/backends/imgui_impl_win32.h>

#ifdef SHINE_IMPORT_STD
// std module import removed
#else
#include <array>
#include <filesystem>

#endif

#include "imgui.h"

// 全局模块导入
#include "editor/mainEditor.h"
#include "editor/views/ImageViewerView.h"
#include "fmt/format.h"

#include "render/core/render_backend.h"
#include "render/render_backend_export.h"
#include "gameplay/camera.h"

#include "render/resources/texture_manager.h"
#include "image/Texture.h"

#include "gameplay/object.h"
#include "gameplay/component/StaticMeshComponent.h"
#include "gameplay/mesh/StaticMesh.h"
#include "manager/CameraManager.h"
#include "manager/InputManager.h"
#include "manager/AssetManager.h"
#include "timer/function_timer.h"

#include "util/fps_controller.h"


#include "quickjs/quickjs.h"



#ifdef SHINE_OPENGL

#endif

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

shine::render::backend::IRenderBackend* RenderBackend = nullptr;

shine::gameplay::Camera g_Camera("默认相机");
// 测试对象，实例化一个静态网格组件
static shine::gameplay::SObject g_TestActor;

// FPS控制器
shine::util::EngineFPSManager g_FPSManager(60.0,
	120.0); // 编辑器60FPS，游戏120FPS

// 初始化共享组件
void InitSharedComponents() 
{
	// 初始化资源管理器（单例，自动管理生命周期）
	shine::manager::AssetManager::Get().Initialize();
}

void InitGlobalData() {
	// auto& plugins = shine::manager::PluginsManager::get();

	const std::string& _testFile =
		"E:\\C++\\shine_optimization_tool\\test\\fuckTest.glb";

	const std::string& ext =
		std::filesystem::path(_testFile).extension().string().substr(1);
	if (ext.empty()) {

		fmt::println("Fuck ext is empty");
		return;
	}

}

// 平台特定的代码
#if defined(SHINE_PLATFORM_WASM) || defined(__EMSCRIPTEN__)
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void initGame() {
        // 初始化游戏逻辑
        InitSharedComponents();
        InitGlobalData();
        // TODO: 添加 WASM 特定的初始化代码
    }

    EMSCRIPTEN_KEEPALIVE
    void updateGame() {
        // 游戏更新循环
        static bool initialized = false;
        if (!initialized) {
            initialized = true;
            // 进行一次初始化
            InitSharedComponents();
            InitGlobalData();
        }

        // 执行一次更新
        g_FPSManager.BeginFrame();
        g_TestActor.onTick(g_FPSManager.GetCurrentDeltaTime());
        g_FPSManager.EndFrame();
    }

    EMSCRIPTEN_KEEPALIVE
    void renderGame() {
        // 渲染一帧
        // TODO: 添加渲染逻辑
    }

    EMSCRIPTEN_KEEPALIVE
    void resetGame() {
        // 重置游戏状态
        // TODO: 添加重置逻辑
    }
}
#endif

// 平台检查宏
#if defined(SHINE_PLATFORM_X64)
    #pragma message("Building for x64 platform")
#elif defined(SHINE_PLATFORM_WASM)
    #pragma message("Building for WASM platform")
#else
    #pragma message("Building for unknown platform")
#endif

static JSValue js_MoveActor(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
{
	int actorID = 0;
	double x = 0.0, y = 0.0;
	JS_ToInt32(ctx, &actorID, argv[0]);
	JS_ToFloat64(ctx, &x, argv[1]);
	JS_ToFloat64(ctx, &y, argv[2]);

	fmt::println("[C++] Actor {} << move to {},{}", actorID, x, y);
	return JS_UNDEFINED;
}

static JSValue js_Log(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
	const char* msg = JS_ToCString(ctx, argv[0]);

	JS_FreeCString(ctx, msg);
	return JS_UNDEFINED;
}

int main(int argc, char** argv) {


#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif

	// 初始化共享组件
	InitSharedComponents();

	//  鍒濆鍖栨彃浠?
	InitGlobalData();

	// ========== 编辑器模式 - 使用ImGui和原生OpenGL窗口 ==========

	// Create application window
	WNDCLASSEXW wc = { sizeof(wc),
					  CS_OWNDC,
					  WndProc,
					  0L,
					  0L,
					  GetModuleHandle(nullptr),
					  nullptr,
					  nullptr,
					  nullptr,
					  nullptr,
					  L"ImGui Example",
					  nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowExW(WS_EX_ACCEPTFILES, wc.lpszClassName,
		L"Dear ImGui Win32+OpenGL3 Example",
		WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800,
		nullptr, nullptr, wc.hInstance, nullptr);


	// Initialize
	RenderBackend = new shine::render::backend::SRenderBackend();

	if (const int result = RenderBackend->init(hwnd, wc) != 0) {
		return result;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	// IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends

	RenderBackend->InitImguiBackend(hwnd);

	// Load Fonts
	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\simkai.ttf", 18.0f, nullptr,
		io.Fonts->GetGlyphRangesChineseFull());


	// camera
	shine::manager::CameraManager::get().setMainCamera(&g_Camera);

	// Our state
	bool show_demo_window = true;
	bool show_another_window = true;
	std::array<float, 4> clear_color = { 0.45f, 0.55f, 0.60f, 1.00f };

	// 在显示窗口后，进入主循环前添加
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// 创建帧缓冲
	RenderBackend->g_Width = 1280;
	RenderBackend->g_Height = 720;
	RenderBackend->CreateFrameBuffer();

	shine::editor::main_editor::MainEditor* mainEditor = nullptr;
	mainEditor = new shine::editor::main_editor::MainEditor();
	mainEditor->Init(RenderBackend);

    // 创建一个三维立方体网格并加载到对象上，注册到渲染服务
    {
        auto mesh = std::make_shared<shine::gameplay::StaticMesh>();
        mesh->initCubeWithNormals();
        // 使用全局共享的PBR材质，可在UI调节点参数
        mesh->setMaterial(shine::render::Material::GetPBR());
        auto* smc = g_TestActor.addComponent<shine::gameplay::component::StaticMeshComponent>();
        smc->setMesh(mesh);
        shine::render::RendererService::get().registerObject(&g_TestActor);
    }
	
	// 加载纹理示例（类似 UE5 的 LoadObject，一步到位）
	{
		shine::util::FunctionTimer __function_timer__("加载纹理", shine::util::TimerPrecision::Nanoseconds);
		
		auto texture = shine::manager::AssetManager::Get().LoadTexture("test_texture.png");
		if (texture && mainEditor->imageViewerView)
		{
			mainEditor->imageViewerView->SetTexture(texture);
		}
	}

	
	// ========================================================================
	// 资源统计信息
	// ========================================================================
	//size_t textureCount, totalMemory;
	//shine::render::TextureManager::get().GetTextureStats(textureCount, totalMemory);
	//fmt::println("纹理统计: GPU纹理数量={}, 估算内存={} KB", textureCount, totalMemory / 1024);
	
	// 注意：AssetManager 的资源统计需要额外实现，这里只是示例
	//fmt::println("提示: AssetManager 管理资源加载，TextureManager 管理 GPU 纹理");
	//fmt::println("     同一资源可以被多个纹理复用，提高内存效率");



	JSRuntime* runtime = JS_NewRuntime();
	JSContext* ctx = JS_NewContext(runtime);

	JSValue global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx, global, "MoveActor", JS_NewCFunction(ctx, js_MoveActor, "MoveActor", 3));
	JS_SetPropertyStr(ctx, global, "Log",JS_NewCFunction(ctx, js_Log, "Log", 1));
	JS_FreeValue(ctx, global);

	JSValue updateFunc{};
	auto result = shine::util::read_file_text("../build/script/game.js");
	if (result.has_value())
	{
		std::string scriptCode = result.value();
		JS_Eval(ctx, scriptCode.c_str(), scriptCode.size(), "game.js", JS_EVAL_TYPE_GLOBAL);

		// 获取 update 函数
		updateFunc = JS_GetPropertyStr(ctx, JS_GetGlobalObject(ctx), "update");
		if (!JS_IsFunction(ctx, updateFunc)) {
			fmt::print("update function not found in script");
			return 1;
		}
	}
	else
	{
		fmt::print("无法读取脚本文件: {}", result.error());
	}
	
	bool done = false;
	while (!done) {
		// FPS控制 - 帧开始
		g_FPSManager.BeginFrame();
		const double dt_d = g_FPSManager.GetCurrentDeltaTime();
		const float dt = static_cast<float>(dt_d);
        g_TestActor.onTick(dt);

		JSValue arg = JS_NewFloat64(ctx, dt_d); // 转换为秒
		JSValue ret = JS_Call(ctx, updateFunc, JS_UNDEFINED, 1, &arg);
		JS_FreeValue(ctx, arg);

		if (JS_IsException(ret)) {
			JSValue error = JS_GetException(ctx);

			// 转成字符串
			const char* error_str = JS_ToCString(ctx, error);
			fmt::print("脚本运行时错误: {}\n", error_str);
			JS_FreeCString(ctx, error_str);

			// 处理 error.stack (traceback)
			JSValue stack = JS_GetPropertyStr(ctx, error, "stack");
			if (!JS_IsUndefined(stack)) {
				const char* stack_str = JS_ToCString(ctx, stack);
				fmt::print("脚本调用栈:\n{}\n", stack_str);
				JS_FreeCString(ctx, stack_str);
			}

			JS_FreeValue(ctx, stack);
			JS_FreeValue(ctx, error);
		}
		JS_FreeValue(ctx, ret);


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

		if (::IsIconic(hwnd)) {
			Sleep(10);
			continue;
		}

        // Start the Dear ImGui frame
        RenderBackend->ImguiNewFrame();
        // 渲染服务，帧开始（保留，留给未来跨线程渲染时用）
        shine::render::RendererService::get().beginFrame();

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::NewFrame();

		// 1. Show the big demo window (commented out - requires imgui_demo.cpp)
		//if (show_demo_window)
		//	ImGui::ShowDemoWindow(&show_demo_window);

		// 2. FPS控制窗口
		{
			ImGui::Begin("FPS Controller");

			// 显示当前FPS信息
			ImGui::Text("Current Mode: %s",
				g_FPSManager.IsGameMode() ? "Game" : "Editor");
			ImGui::Text("Current FPS: %.1f / %.1f", g_FPSManager.GetCurrentFPS(),
				g_FPSManager.GetCurrentTargetFPS());
			ImGui::Text("Delta Time: %.2f ms", g_FPSManager.GetCurrentDeltaTime());

			ImGui::Separator();

			// 编辑器FPS控制
			static float editorFPS =
				static_cast<float>(g_FPSManager.GetEditorController().GetTargetFPS());
			if (ImGui::SliderFloat("Editor FPS", &editorFPS, 30.0f, 144.0f, "%.0f")) {
				g_FPSManager.SetEditorFPS(static_cast<double>(editorFPS));
			}

			static bool editorFPSEnabled =
				g_FPSManager.GetEditorController().IsEnabled();
			if (ImGui::Checkbox("Enable Editor FPS Limit", &editorFPSEnabled)) {
				g_FPSManager.SetEditorFPSEnabled(editorFPSEnabled);
			}

			ImGui::Separator();

			// 游戏FPS控制
			static float gameFPS =
				static_cast<float>(g_FPSManager.GetGameController().GetTargetFPS());
			if (ImGui::SliderFloat("Game FPS", &gameFPS, 30.0f, 240.0f, "%.0f")) {
				g_FPSManager.SetGameFPS(static_cast<double>(gameFPS));
			}

			static bool gameFPSEnabled = g_FPSManager.GetGameController().IsEnabled();
			if (ImGui::Checkbox("Enable Game FPS Limit", &gameFPSEnabled)) {
				g_FPSManager.SetGameFPSEnabled(gameFPSEnabled);
			}

			ImGui::Separator();

			// 模式切换
			static bool gameMode = g_FPSManager.IsGameMode();
			if (ImGui::Checkbox("Game Mode (vs Editor Mode)", &gameMode)) {
				g_FPSManager.SetGameMode(gameMode);
			}

			ImGui::Separator();

			// 调试信息
			ImGui::Text("Debug Info:");
			ImGui::TextWrapped("%s", g_FPSManager.GetDebugInfo().c_str());

			ImGui::End();
		}


        // 编辑器UI渲染
        mainEditor->Render();
        shine::manager::CameraManager::get().getMainCamera()->Apply();

        // 统一用渲染服务提供帧缓冲渲染/显示
        shine::render::RendererService::get().endFrame(clear_color);

		// FPS控制 - 帧结束
		g_FPSManager.EndFrame();
	}

	// 清理ImGui
	RenderBackend->ClearUp(hwnd);

	// 清理编辑器
	if (mainEditor)
	{
		delete mainEditor;
		mainEditor = nullptr;
	}

	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}

// ========== 编辑器模式专用函数 ==========

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED) {
			RenderBackend->ReSizeFrameBuffer(LOWORD(lParam), HIWORD(lParam));
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;

	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;

	case WM_DROPFILES: {
		HDROP hDrop = reinterpret_cast<HDROP>(wParam);

		UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
		std::wstring filename;
		for (UINT i = 0; i < fileCount; i++) {
			// 获取文件名的长度
			UINT filenameLength = DragQueryFileW(hDrop, i, nullptr, 0);
			if (filenameLength > 0) {
				filename.resize(filenameLength);

				// 获取文件名
				DragQueryFileW(hDrop, i, &filename[0], filenameLength + 1);

				// std::string uf8FileName =
				// shine::util::StringUtil::WstringToUTF8(filename);
				// if(shine::util::LoadTextureFromFile(uf8FileName.c_str(),&RenderBackend->my_image_texture,&my_image_width
				// , &my_image_height ))
				//{
				//     std::println("加载图片成功");
				// }else{
				//     std::println("加载图片失败:{}",uf8FileName);
				// }
			}
		}
		DragFinish(hDrop);
	} break;
	}

	shine::input_manager::InputManager::get().processWin32Message(msg, wParam,
		lParam);

	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}


