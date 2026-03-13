#pragma once
// ============================================================
//  AssetDependencyView — editor panel showing asset dependencies.
//
//  Shows forward and reverse dependencies for the selected asset,
//  and highlights dangling (broken) references.
// ============================================================

#include "editor/views/BaseView.h"
#include "string/shine_string.h"

namespace shine::editor::asset
{
    class EditorAssetRegistry;
}

namespace shine::editor::views
{
    class AssetDependencyView : public BaseView
    {
    public:
        void SetEditorAssetRegistry(asset::EditorAssetRegistry* registry) { registry_ = registry; }
        void SetSelectedAssetUUID(STextView uuid) { selectedUuid_ = SString(uuid); }

        void onInit() override;
        void onRender() override;
        void onShutDown() override;

    private:
        asset::EditorAssetRegistry* registry_ = nullptr;
        SString selectedUuid_;
    };

} // namespace shine::editor::views
