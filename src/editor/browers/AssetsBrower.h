#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>


#include "imgui/imgui.h"
#include "editor/views/BaseView.h"
#include "string/shine_string.h"
#include "editor/browers/IAssetThumbnailProvider.h"
#include "editor/browers/ThumbnailProviderRegistry.h"

namespace shine::editor::asset
{
    class EditorAssetRegistry;
    class ImportPipeline;
    class IAssetImporter;
}

namespace shine::editor::assets_brower 
{

    class AssetsBrower : public views::BaseView
    {
        public:
            shine::SString title = "资产浏览器";
            
            void SetEditorAssetRegistry(shine::editor::asset::EditorAssetRegistry* registry) { editorAssetRegistry_ = registry; }

            /**
             * @brief 注册自定义缩略图提供者
             *
             * 先注册的提供者具有更高优先级（在内置提供者之前被匹配到）。
             * 若要低于内置优先级，请在 onInit() 之后调用。
             * @param provider  实现了 IAssetThumbnailProvider 的提供者实例（转移所有权）
             */
            void RegisterThumbnailProvider(std::unique_ptr<IAssetThumbnailProvider> provider)
            {
                thumbnailRegistry_.Register(std::move(provider));
            }

            void onInit() override;
            void onRender() override;
            void onShutDown() override;

        private:
            void RenderDirectoryNode(const std::filesystem::path& path);
            void RenderDirectoryContextMenu(const std::filesystem::path& path);
            void RenderAssetGrid();
            void UpdateLayoutSizes(float availWidth, int itemCount);
            void RenderAssetContextMenu(const std::filesystem::directory_entry& entry);
            void RenderOperationsPopup();
            void RenderImportPopup();
            void OpenEntry(const std::filesystem::directory_entry& entry);
            bool RenameEntry(const std::filesystem::path& sourcePath, const shine::SString& newName);
            bool DeleteEntry(const std::filesystem::path& path);
            bool MoveEntry(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationDirectory);
            bool CreateFolder(const std::filesystem::path& parentDir, const shine::SString& name);
            bool RenameDirectory(const std::filesystem::path& oldPath, const shine::SString& newName);
            bool DeleteDirectory(const std::filesystem::path& path);
            void SyncAssetRecordMove(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
            void SyncAssetRecordMoveDir(const std::filesystem::path& oldDir, const std::filesystem::path& newDir);
            void SyncAssetRecordDelete(const std::filesystem::path& path);
            shine::SString ToDisplayText(const std::filesystem::path& path) const;
            bool PassesSearchFilter(const std::filesystem::path& path) const;
            static ImGuiID PathToID(const std::filesystem::path& path);

        private:
            // -----------------------------------------------------------------------
            //  待导入数据（由 EnqueueExternalDrop 或右键菜单"导入"填充）
            // -----------------------------------------------------------------------
            struct ImportPending {
                std::filesystem::path                 sourcePath;
                shine::editor::asset::IAssetImporter* importer = nullptr;  // 非拥有指针
                std::string                           settingsJson;         // raw JSON
            };

        private:
            std::filesystem::path contentRoot_;          // 固定的 Content 根目录
            std::filesystem::path selectedDirectory_;    // 右侧网格的当前目录
            std::filesystem::path contextEntryPath_;     // 网格右键目标文件
            std::filesystem::path contextDirPath_;       // 目录树右键目标文件夹

            // 网格条目缓存（每帧重建，供 AdapterIndexToStorageId 访问）
            std::vector<std::filesystem::directory_entry> gridEntries_;

            ImGuiSelectionBasicStorage selection_;       // 多选状态

            char searchBuffer_[128]{};
            char renameBuffer_[256]{};
            char moveTargetBuffer_[512]{};
            char newFolderBuffer_[256]{};
            char renameDirBuffer_[256]{};

            float iconSize_          = 64.0f;
            float iconSpacing_       = 10.0f;
            int   iconHitSpacing_    = 4;
            bool  stretchSpacing_    = true;
            float zoomWheelAccum_    = 0.0f;

            bool requestRenamePopup_    = false;
            bool requestDeletePopup_    = false;
            bool requestMovePopup_      = false;
            bool requestNewFolderPopup_ = false;
            bool requestRenameDirPopup_ = false;
            bool requestDeleteDirPopup_ = false;
            bool requestImportPopup_    = false;

            std::optional<ImportPending> pendingImport_;
            std::string                  importErrorMsg_;

            // 布局缓存（由 UpdateLayoutSizes 填充）
            ImVec2 layoutItemSize_{};
            ImVec2 layoutItemStep_{};
            float  layoutItemSpacing_       = 0.0f;
            float  layoutSelectableSpacing_ = 0.0f;
            float  layoutOuterPadding_      = 0.0f;
            int    layoutColumnCount_       = 1;
            int    layoutLineCount_         = 0;

        shine::editor::asset::EditorAssetRegistry* editorAssetRegistry_ = nullptr;
        shine::editor::asset::ImportPipeline*       importPipeline_       = nullptr;

        ThumbnailProviderRegistry thumbnailRegistry_;
    };
}
