#include "editor/main_editor/EditorCompositionRoot.h"

#include "EngineCore/engine_context.h"
#include "editor/mainEditor.h"
#include "editor/ShineAsset/core/RuntimeAssetRegistry.h"
#include "editor/ShineAsset/registry/EditorAssetRegistry.h"
#include "editor/ShineAsset/importers/ImportPipeline.h"

namespace shine::editor::main_editor
{
    void EditorCompositionRoot::RegisterEditorSystems(EngineContext& context)
    {
        context.Register(new shine::asset::RuntimeAssetRegistry());
        context.Register(new shine::editor::asset::EditorAssetRegistry());
        context.Register(new shine::editor::asset::ImportPipeline());
    }

    std::unique_ptr<MainEditor> EditorCompositionRoot::BuildMainEditor(EngineContext& context)
    {
        return std::make_unique<MainEditor>(context);
    }
}
