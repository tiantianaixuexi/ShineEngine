#include "FpsControlView.h"

#include "fps_controller.h"

#include "imgui/imgui.h"


namespace shine::editor
{
	FpsControlView::FpsControlView()
	{

	}

	void FpsControlView::Init()
	{
		gameFPSEnabled = false;
		editorFPSEnabled = true;
	}

	void FpsControlView::Render()
	{
		auto& g_FPSManager = util::EngineFPSManager::get();

		static double editorFps = g_FPSManager.GetEditorController().GetTargetFPS();
		static double gameFps = g_FPSManager.GetGameController().GetTargetFPS();
		static float  editorFPS_float = static_cast<float>(editorFps);
		static float  gameFPS_float = static_cast<float>(gameFps);

		ImGui::Begin("FPS Controller", &IsOpen);

		// 显示当前FPS信息
		ImGui::Text("当前模式: %s",
			g_FPSManager.IsGameMode() ? "游戏" : "编辑器");
		ImGui::Text("Current FPS: %.1f / %.1f", g_FPSManager.GetCurrentFPS(),
			g_FPSManager.GetCurrentTargetFPS());
		ImGui::Text("Delta Time: %.2f ms", g_FPSManager.GetCurrentDeltaTime());

		ImGui::Separator();

		// 编辑器FPS控制
		
		if (ImGui::SliderFloat("编辑器 FPS", &editorFPS_float, 30.0f, 144.0f, "%.0f")) {
			g_FPSManager.SetEditorFPS(editorFps);
		}

	
		if (ImGui::Checkbox("Enable Editor FPS Limit", &editorFPSEnabled)) {
			g_FPSManager.SetEditorFPSEnabled(editorFPSEnabled);
		}

		ImGui::Separator();

		// 游戏FPS控制

		if (ImGui::SliderFloat("游戏 FPS", &gameFPS_float, 30.0f, 240.0f, "%.0f")) {
			g_FPSManager.SetGameFPS(gameFps);
		}

		if (ImGui::Checkbox("开启游戏FPS监控", &gameFPSEnabled)) {
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
}
