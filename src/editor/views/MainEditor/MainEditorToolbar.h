#pragma once

namespace shine::editor::main_editor
{
	class MainEditor;
}
namespace shine::editor::views
{
	class SMainEditorToolbar
	{
	public:

		SMainEditorToolbar(main_editor::MainEditor* _editor);

		void Init();
		void Render();





	private:


		 main_editor::MainEditor* _mainEditor = nullptr;
	};

	
}

