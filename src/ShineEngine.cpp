#include "file_util.h"
#include "util/shine_define.h"




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

//
//static JSValue js_MoveActor(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
//{
//	int actorID = 0;
//	double x = 0.0, y = 0.0;
//	JS_ToInt32(ctx, &actorID, argv[0]);
//	JS_ToFloat64(ctx, &x, argv[1]);
//	JS_ToFloat64(ctx, &y, argv[2]);
//
//	fmt::println("[C++] Actor {} << move to {},{}", actorID, x, y);
//	return JS_UNDEFINED;
//}
//
//static JSValue js_Log(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
//	const char* msg = JS_ToCString(ctx, argv[0]);
//
//	JS_FreeCString(ctx, msg);
//	return JS_UNDEFINED;
//}

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



    //// 创建一个三维立方体网格并加载到对象上，注册到渲染服务
    //{
    //    auto mesh = std::make_shared<shine::gameplay::StaticMesh>();
    //    mesh->initCubeWithNormals();
    //    // 使用全局共享的PBR材质，可在UI调节点参数
    //    mesh->setMaterial(shine::render::Material::GetPBR());
    //    auto* smc = g_TestActor.addComponent<shine::gameplay::component::StaticMeshComponent>();
    //    smc->setMesh(mesh);
    //    shine::render::RendererService::get().registerObject(&g_TestActor);
    //}
	//
	//// 加载纹理示例（类似 UE5 的 LoadObject，一步到位）
	//{
	//	shine::util::FunctionTimer __function_timer__("加载纹理", shine::util::TimerPrecision::Nanoseconds);
	//	
	//	auto texture = shine::manager::AssetManager::Get().LoadTexture("test_texture.png");
	//	if (texture && mainEditor->imageViewerView)
	//	{
	//		mainEditor->imageViewerView->SetTexture(texture);
	//	}
	//}

	
	// ========================================================================
	// 资源统计信息
	// ========================================================================
	//size_t textureCount, totalMemory;
	//shine::render::TextureManager::get().GetTextureStats(textureCount, totalMemory);
	//fmt::println("纹理统计: GPU纹理数量={}, 估算内存={} KB", textureCount, totalMemory / 1024);
	
	// 注意：AssetManager 的资源统计需要额外实现，这里只是示例
	//fmt::println("提示: AssetManager 管理资源加载，TextureManager 管理 GPU 纹理");
	//fmt::println("     同一资源可以被多个纹理复用，提高内存效率");



	auto& g_FPSManager = util::EngineFPSManager::get();
	bool done = false;
	while (!done) {
		
		// FPS控制 - 帧开始
		g_FPSManager.BeginFrame();
		const double dt_d = g_FPSManager.GetCurrentDeltaTime();
		const float dt = static_cast<float>(dt_d);
        //g_TestActor.onTick(dt);


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

