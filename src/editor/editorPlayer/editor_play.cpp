#include "editor_play.h"
#include "./Selection/editor_selection.h"


namespace shine::editor
{

	void SEditorPlayer::init()
	{
		_selectionManager = new selection::SelectionManager();
	}
}