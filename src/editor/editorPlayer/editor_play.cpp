#include "editor_play.h"
#include "./Selection/editor_selection.h"


namespace shine::editor
{

	void SEditorPlayer::init()
	{
		_selectionManager = new selection::SelectionManager();
	}

	void SEditorPlayer::selct(selection::SelectionData& _Data)
	{
		_selectionManager->selct(_Data);
	}

	void SEditorPlayer::remove(selection::SelectionData& _Data)
	{
		_selectionManager->remove(_Data);
	}

	bool SEditorPlayer::isSelect(selection::SelectionData& _Data)
	{
		return _selectionManager->isSelect(_Data);
	}
}
