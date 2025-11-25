#include "util/shine_define.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef  SHINE_PLATFORM_WIN64
#include "sol/sol.hpp"
#include "luajit/luajit.h"
#include "luajit/lauxlib.h"
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

// 全局的应用程序编辑器入口点
int main(int argc, char** argv) {
	// std::string data = shine::base64::base64_test("fuck");
	// fmt::println("base64:{}", data);

	//shine::util::win32::api();



    // {
    //     shine::model::obj obj;
    //     auto result = obj.parseObjFile("E:\\c++\\shine_optimization_tool\\model\\test.obj");
    //     if(result.has_value())
    //     {
        //         fmt::println("加载obj文件成功");
    //     }
    //     else
    //     {
        //         fmt::println("加载obj文件失败:{}", result.error());
    //     }
    // }


    // sol::state lua;
    // lua.open_libraries(sol::lib::base);

    // // 手动加载luaJIT的jit库
    // luaL_requiref(lua.lua_state(), "jit", luaopen_jit, 0);
    // lua_pop(lua.lua_state(), 1); // 移除requiref栈下的结果

    // // 绑定游戏对象到Lua
    // auto gameObjectType = lua.new_usertype<shine::gameplay::SObject>("GameObject",
    //     // 构造函数
    //     sol::constructors<shine::gameplay::SObject()>(),

    //     // 属性
    //     "name", sol::property(&shine::gameplay::SObject::getName, &shine::gameplay::SObject::setName),
    //     "active", sol::property(&shine::gameplay::SObject::isActive, &shine::gameplay::SObject::setActive),
    //     "visible", sol::property(&shine::gameplay::SObject::isVisible, &shine::gameplay::SObject::setVisible),

    //     // 生命周期方法
    //     "OnInit", &shine::gameplay::SObject::OnInit,
    //     "BeginPlay", &shine::gameplay::SObject::onBeginPlay,
    //     "Tick", &shine::gameplay::SObject::onTick,

    //     // 组件管理
    //     "addComponent", &shine::gameplay::SObject::addComponent<shine::gameplay::component::StaticMeshComponent>
    // );

    // // 绑定组件基类
    // auto componentType = lua.new_usertype<shine::gameplay::component::UComponent>("Component",
    //     "BeginPlay", &shine::gameplay::component::UComponent::onBeginPlay,
    //     "Tick", &shine::gameplay::component::UComponent::onTick
    // );

    // // 绑定静态网格组件
    // auto staticMeshComponentType = lua.new_usertype<shine::gameplay::component::StaticMeshComponent>("StaticMeshComponent",
    //     sol::base_classes, sol::bases<shine::gameplay::component::UComponent>()
    // );

    // // 绑定数学库
    // lua.new_usertype<shine::math::Vector3>("Vector3",
    //     sol::constructors<shine::math::Vector3(), shine::math::Vector3(float, float, float)>(),
    //     "x", &shine::math::Vector3::x,
    //     "y", &shine::math::Vector3::y,
    //     "z", &shine::math::Vector3::z
    // );

    // // 绑定输入管理器
    // lua.new_usertype<shine::input_manager::InputManager>("InputManager",
    //     "isKeyPressed", &shine::input_manager::InputManager::isKeyPressed,
    //     "isMouseButtonPressed", &shine::input_manager::InputManager::isMouseButtonPressed,
    //     "getMousePosition", &shine::input_manager::InputManager::getMousePosition
    // );

    // // 创建全局输入管理器访问
    // lua["Input"] = &shine::input_manager::InputManager::get();

    // // 测试脚本 - 完整的游戏脚本系统展示
    // lua.script(R"(
    //     print('=== LuaJIT 脚本系统展示 ===')

    //     -- 创建高级脚本组件
    //     ScriptComponent = {}
    //     ScriptComponent.__index = ScriptComponent

    //     function ScriptComponent:new()
    //         local self = setmetatable({}, ScriptComponent)
    //         self.rotationSpeed = 1.0
    //         self.moveSpeed = 0.5
    //         self.position = Vector3(0, 0, 0)
    //         self.timeElapsed = 0
    //         return self
    //     end

    //     function ScriptComponent:BeginPlay()
    //         print("ScriptComponent: BeginPlay called!")
    //         print("Initial position: (" .. self.position.x .. ", " .. self.position.y .. ", " .. self.position.z .. ")")
    //     end

    //     function ScriptComponent:Tick(deltaTime)
    //         -- 绱Н鏃堕棿
    //         self.timeElapsed = self.timeElapsed + deltaTime

    //         -- 绠€鍗曠殑鏃嬭浆閫昏緫
    //         self.rotationSpeed = self.rotationSpeed + deltaTime * 0.1

    //         -- 绠€鍗曠殑绉诲姩閫昏緫 - 鍦嗗懆杩愬姩
    //         local radius = 2.0
    //         self.position.x = math.cos(self.timeElapsed) * radius
    //         self.position.z = math.sin(self.timeElapsed) * radius
    //         self.position.y = math.sin(self.timeElapsed * 2) * 0.5

    //         -- 妫€鏌ヨ緭鍏?
    //         if Input:isKeyPressed(87) then -- W键
    //             print("W key pressed!")
    //         end

    //         -- 姣忕鎵撳嵃涓€娆＄姸鎬?
    //         if math.floor(self.timeElapsed) ~= math.floor(self.timeElapsed - deltaTime) then
    //             print(string.format("ScriptComponent: Time=%.1f, Pos=(%.2f, %.2f, %.2f), Speed=%.2f",
    //                 self.timeElapsed, self.position.x, self.position.y, self.position.z, self.rotationSpeed))
    //         end
    //     end

    //     -- 创建游戏对象
    //     local obj = GameObject()
    //     obj.name = "MyScriptedObject"
    //     print("Created object: " .. obj.name)

    //     -- 初始化对象生命周期
    //     obj:OnInit()
    //     obj:BeginPlay()

    //     -- 添加C++原生组件
    //     local meshComp = obj:addComponent(StaticMeshComponent())
    //     print("Added StaticMeshComponent")

    //     -- 设置对象属性
    //     obj.active = true
    //     obj.visible = true

    //     -- 创建并运行Lua脚本组件
    //     local scriptComp = ScriptComponent:new()
    //     obj.ScriptComponent = scriptComp

    //     -- 调用脚本组件的BeginPlay
    //     scriptComp:BeginPlay()

    //     -- 存储到全局变量供后续使用
    //     GlobalGameObject = obj

    //     print('=== 鑴氭湰绯荤粺鍒濆鍖栧畬鎴?===')
    //     print('按W键测试输入系统')
    // )");


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
	// 方式2：使用 STexture 资源类（适用于需要保留CPU数据的场景）
	// ========================================================================
	// {
	//     shine::image::STexture texture;
	//     
	//     // 从 AssetHandle 初始化（利用 AssetManager 的资源）
	//     auto assetHandle = assetManager.LoadImage("texture.png");
	//     if (texture.InitializeFromAsset(assetHandle))
	//     {
	//         fmt::println("STexture: 从 AssetHandle 初始化成功");
	//         
	//         // 可以修改纹理数据（如果需要）
	//         // auto& data = texture.getData();
	//         // data[0] = shine::image::RGBA8(255, 0, 0, 255);
	//         
	//         // 创建 GPU 纹理资源（直接调用，内部使用 TextureManager 单例）
	//         auto handle = texture.CreateRenderResource();
	//         if (handle.isValid())
	//         {
	//             fmt::println("STexture: GPU 纹理创建成功");
	//         }
	//     }
	// }

	// ========================================================================
	// 方式3：快捷方式 - 直接从文件创建纹理（内部会使用 AssetManager）
	// 适用于：简单场景，不需要复用资源的情况
	// ========================================================================
	// auto quickHandle = textureManager.CreateTextureFromFile(texturePath);
	// if (quickHandle.isValid())
	// {
	//     fmt::println("快捷方式: 纹理创建成功");
	// }

	// ========================================================================
	// 方式4：从内存数据创建（适用于网络下载、程序生成等场景）
	// ========================================================================
	// {
	//     unsigned char* imageData = ...;
	//     size_t dataSize = ...;
	//     
	//     // 先通过 AssetManager 从内存加载
	//     auto memAssetHandle = assetManager.LoadImageFromMemory(imageData, dataSize, "png");
	//     if (memAssetHandle.isValid())
	//     {
	//         // 再创建纹理
	//         auto memTextureHandle = textureManager.CreateTextureFromAsset(memAssetHandle);
	//     }
	// }

	// ========================================================================
	// 资源统计信息
	// ========================================================================
	size_t textureCount, totalMemory;
	shine::render::TextureManager::get().GetTextureStats(textureCount, totalMemory);
	fmt::println("纹理统计: GPU纹理数量={}, 估算内存={} KB", textureCount, totalMemory / 1024);
	
	// 注意：AssetManager 的资源统计需要额外实现，这里只是示例
	fmt::println("提示: AssetManager 管理资源加载，TextureManager 管理 GPU 纹理");
	fmt::println("     同一资源可以被多个纹理复用，提高内存效率");



	// 涓诲惊鐜?
	bool done = false;
	while (!done) {
		// FPS控制 - 帧开始
		g_FPSManager.BeginFrame();

        g_TestActor.onTick(g_FPSManager.GetCurrentDeltaTime());

        // // 执行Lua对象的Tick
        // if (lua["GlobalGameObject"] != sol::nil)
        // {
        //     auto globalObj = lua["GlobalGameObject"];
        //     if (globalObj != sol::nil)
        //     {
        //         auto objTable = globalObj.as<sol::table>();
        //         // 调用游戏对象的Tick方法
        //         objTable["Tick"](objTable, g_FPSManager.GetCurrentDeltaTime());

        //         // 调用附加的脚本组件的Tick方法
        //         if (objTable["ScriptComponent"] != sol::nil)
        //         {
        //             auto scriptComp = objTable["ScriptComponent"];
        //             if (scriptComp != sol::nil)
        //             {
        //                 scriptComp.as<sol::table>()["Tick"](scriptComp, g_FPSManager.GetCurrentDeltaTime());
        //             }
        //         }
        //     }
        // }

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

		// 3. Show another simple window
		if (show_another_window) {
			ImGui::Begin("Another Window", &show_another_window);
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
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


