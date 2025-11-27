#pragma once

namespace shine::editor
{


	class FpsControlView
	{

	public:
		FpsControlView();


		void Init();
		void Render();



		bool IsOpen = false;
		bool gameFPSEnabled = false;
		bool editorFPSEnabled = true;
	};
}
