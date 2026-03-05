#include "editor/main_editor/editor_composition_root.h"

#include "EngineCore/engine_context.h"
#include "editor/asset/AssetDirectoryService.h"
#include "editor/asset/editor_asset_manager.h"
#include "editor/asset/editor_runtime_asset_bridge.h"
#include "editor/mainEditor.h"

namespace shine::editor::main_editor
{
    void EditorCompositionRoot::RegisterEditorSystems(EngineContext& context)
    {
        auto* editorAssetManager = new editor::asset::EditorAssetManager();
        auto* runtimeAssetBridge = new editor::asset::EditorRuntimeAssetBridge(editorAssetManager);
        auto* assetDirectoryService = new editor::asset::AssetDirectoryService();
        context.Register(editorAssetManager);
        context.Register(runtimeAssetBridge);
        context.Register(assetDirectoryService);
    }

    std::unique_ptr<MainEditor> EditorCompositionRoot::BuildMainEditor(EngineContext& context)
    {
        return std::make_unique<MainEditor>(context);
    }
}
