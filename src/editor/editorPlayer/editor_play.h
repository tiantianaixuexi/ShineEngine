#pragma once
#include "singleton.h"



namespace shine::editor::selection
{
	struct SelectionData;
	class SelectionManager;
}


namespace shine::editor
{

	class SEditorPlayer : public util::Singleton<SEditorPlayer>
	{
	public:


		void init();

		void selct(selection::SelectionData& _Data);

		void remove(selection::SelectionData& _Data);

		bool isSelect(selection::SelectionData& _Data);


		selection::SelectionManager* _selectionManager = nullptr;

	};


}
