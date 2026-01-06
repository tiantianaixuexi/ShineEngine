#pragma once

#include "../../EngineCore/subsystem.h"
#include "../../EngineCore/engine_context.h"

namespace shine::editor::selection
{
	struct SelectionData;
	class SelectionManager;
}

namespace shine::editor
{
	class SEditorPlayer : public shine::Subsystem
	{
	public:

		void init();

		void selct(selection::SelectionData& _Data);

		void remove(selection::SelectionData& _Data);

		bool isSelect(selection::SelectionData& _Data);

		selection::SelectionManager* _selectionManager = nullptr;
	};
}
