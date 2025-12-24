#pragma once
#include "singleton.h"



namespace shine::editor::selection
{
	class SelectionManager;
}


namespace shine::editor
{

	class SEditorPlayer : public util::Singleton<SEditorPlayer>
	{
	public:


		void init();


		selection::SelectionManager* _selectionManager = nullptr;

	};


}
