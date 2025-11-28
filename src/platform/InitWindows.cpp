#include "InitWindows.h"


#include "util/shine_define.h"

#include "WindowsInfo.h"

#include "manager/InputManager.h"

#include "imgui.h"
#include "render/renderManager.h"

// Forward-declare WndProc handler from imgui Win32 backend (header intentionally leaves it commented out)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace shine::windows
{

	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool InitWindowsPlatform()
	{

		WindowsDeviceInfo::get().InitDisplayInfo();
		
		auto& mainDisplay = WindowsDeviceInfo::get().MainDisplayInfo;

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
		WindowsInfo::get().info.hwnd = ::CreateWindowExW(WS_EX_ACCEPTFILES, wc.lpszClassName,
			L"ShineEngine",
			WS_OVERLAPPEDWINDOW, 0, 0, mainDisplay.workSize[0], mainDisplay.workSize[1],
			nullptr, nullptr, wc.hInstance, nullptr);


		auto& info = WindowsInfo::get().info;
		
		render::RenderManager::get().CreateRenderBackend();
		if (const int result = render::RenderManager::get().GetRenderBackend()->init(info.hwnd, wc); result != 0)
		{
			return result;
		}

		render::RenderManager::get().GetRenderBackend()->InitImguiBackend(info.hwnd);

		// Show the window
		::ShowWindow(info.hwnd, SW_MAXIMIZE);
		::UpdateWindow(info.hwnd);

		// 初始化 Imgui 
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		// Setup Dear ImGui style
		ImGui::StyleColorsDark();




		// Load Fonts
		io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\simkai.ttf", 18.0f, nullptr,
			io.Fonts->GetGlyphRangesChineseFull());

		return true;
	}



	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg) {

		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED) {
				render::RenderManager::get().GetRenderBackend()->ReSizeFrameBuffer(LOWORD(lParam), HIWORD(lParam));
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

}
