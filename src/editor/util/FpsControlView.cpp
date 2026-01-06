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
		auto& g_FPSManager = util::FPSController::get();

		static double gameFps = g_FPSManager.GetActualFPS();
		static float  gameFPS_float = static_cast<float>(gameFps);

		ImGui::Begin("FPS Controller", &IsOpen);

		ImGui::Text("Current FPS: %.1f", gameFPS_float);
		ImGui::Text("Delta  Time: %.2f", g_FPSManager.GetDeltaTime());

		ImGui::Separator();


		ImGui::End();

	}
}
