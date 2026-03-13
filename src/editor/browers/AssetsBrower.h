#pragma once

#include <filesystem>
#include <optional>
#include <vector>


#include "imgui/imgui.h"
#include "editor/views/BaseView.h"
#include "string/shine_string.h"

namespace shine::editor::asset
{
    class EditorAssetRegistry;
}

namespace shine::editor::assets_brower 
{

    class AssetsBrower : public views::BaseView
    {
        public:
            shine::SString title = "资产浏览器";
            
            void SetEditorAssetRegistry(shine::editor::asset::EditorAssetRegistry* registry) { editorAssetRegistry_ = registry; }

            void onInit() override;
            void onRender() override;
            void onShutDown() override;

        private:
            void RenderDirectoryNode(const std::filesystem::path& path);
            void RenderAssetGrid();
            void RenderAssetItem(const std::filesystem::directory_entry& entry, int index, const ImVec2& startPos, int columnCount, int lineIndex, int columnIndex);
            void RenderAssetContextMenu(const std::filesystem::directory_entry& entry);
            void RenderOperationsPopup();
            void OpenEntry(const std::filesystem::directory_entry& entry);
            bool RenameEntry(const std::filesystem::path& sourcePath, const shine::SString& newName);
            bool DeleteEntry(const std::filesystem::path& path);
            bool MoveEntry(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationDirectory);
            void SyncAssetRecordMove(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
            void SyncAssetRecordDelete(const std::filesystem::path& path);
            shine::SString ToDisplayText(const std::filesystem::path& path) const;
            bool PassesSearchFilter(const std::filesystem::path& path) const;
            float GetGridItemHeight() const;

        private:
            std::filesystem::path selectedDirectory_;
            std::filesystem::path selectedEntryPath_;
            std::filesystem::path contextEntryPath_;
            std::vector<int> filteredEntryIndices_;
            char searchBuffer_[128]{};
            char renameBuffer_[256]{};
            char moveTargetBuffer_[512]{};
            float iconSize_ = 64.0f;
            float iconSpacing_ = 14.0f;
            bool requestRenamePopup_ = false;
            bool requestDeletePopup_ = false;
            bool requestMovePopup_ = false;

        shine::editor::asset::EditorAssetRegistry* editorAssetRegistry_ = nullptr;
    };
}
