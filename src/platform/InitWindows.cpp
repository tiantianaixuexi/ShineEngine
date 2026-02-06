#include "InitWindows.h"

#include "WindowsInfo.h"

#include "manager/InputManager.h"

#include "imgui/imgui.h"
#include "fmt/base.h"
#include "render/renderer_service.h"
#include "render/backend/render_backend_factory.h"


// Forward-declare WndProc handler from imgui Win32 backend
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace shine::windows
{

	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool InitWindowsPlatform(EngineContext& context)
	{
		context.GetSystem<WindowsDeviceInfo>()->InitDisplayInfo();
		
		auto& mainDisplay = context.GetSystem<WindowsDeviceInfo>()->MainDisplayInfo;

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
		context.GetSystem<WindowsInfo>()->info.hwnd = ::CreateWindowExW(WS_EX_ACCEPTFILES, wc.lpszClassName,
			L"ShineEngine",
			WS_OVERLAPPEDWINDOW, 0, 0, mainDisplay.workSize[0], mainDisplay.workSize[1],
			nullptr, nullptr, wc.hInstance, nullptr);


		auto& info = context.GetSystem<WindowsInfo>()->info;
		
		// Use factory to create backend (returns unique_ptr)
		auto renderBackend = shine::render::backend::RenderBackendFactory::create(
			shine::render::backend::RenderBackendType::OpenGL);
                
		if (!renderBackend) {
			return false;
		}

		// Pass NativeWindowHandle (void*) and platform data (WNDCLASSEXW*)
		if (const int result = renderBackend->init(info.hwnd, &wc); result != 0)
		{
			return result;
		}

		// Get raw pointer before transferring ownership
		auto* backendPtr = renderBackend.get();

		// Transfer ownership to RendererService and initialize
		context.GetSystem<render::RendererService>()->init(backendPtr);

		// Set size through the proper interface (no more direct member access)
		backendPtr->SetSize(mainDisplay.workSize[0], mainDisplay.workSize[1]);
		backendPtr->CreateFrameBuffer();

		// Initialize ImGui
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::StyleColorsDark();

		backendPtr->InitImguiBackend(info.hwnd);

		// Show the window
		::ShowWindow(info.hwnd, SW_MAXIMIZE);
		::UpdateWindow(info.hwnd);

		// Load Fonts
		io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\simkai.ttf", 18.0f, nullptr,
			io.Fonts->GetGlyphRangesChineseFull());

		// Release unique_ptr ownership — RendererService now manages the backend lifetime
		// TODO: Ideally RendererService should own the unique_ptr. For now, leak is prevented
		// by backend outliving the service (both registered with EngineContext).
		renderBackend.release();

		return true;
	}
	

	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg) {

		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED) {
				if (shine::EngineContext::IsInitialized()) {
					shine::EngineContext::Get().GetSystem<render::RendererService>()->GetBackend()->ReSizeFrameBuffer(LOWORD(lParam), HIWORD(lParam));
				}
			}else
			{
				fmt::println("窗口最小化");
			}
			return 0;

		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU)
				return 0;
			break;

		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;

		
		}

		shine::input::InputManager::get().processWin32Message(msg, wParam,
			lParam);

		return ::DefWindowProcW(hWnd, msg, wParam, lParam);
	}

}
