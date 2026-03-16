#include "editor/main_editor/EditorCompositionRoot.h"

#include "EngineCore/engine_context.h"
#include "editor/mainEditor.h"
#include "editor/ShineAsset/core/RuntimeAssetRegistry.h"
#include "editor/ShineAsset/registry/EditorAssetRegistry.h"

namespace shine::editor::main_editor
{
    void EditorCompositionRoot::RegisterEditorSystems(EngineContext& context)
    {
        context.Register(new shine::asset::RuntimeAssetRegistry());
        context.Register(new shine::editor::asset::EditorAssetRegistry());
    }

    std::unique_ptr<MainEditor> EditorCompositionRoot::BuildMainEditor(EngineContext& context)
    {
        return std::make_unique<MainEditor>(context);
    }
}
