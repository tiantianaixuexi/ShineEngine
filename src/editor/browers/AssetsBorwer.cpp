#include "AssetsBrower.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>

#include "EngineCore/engine_context.h"
#include "editor/asset/AssetDirectoryService.h"
#include "editor/asset/editor_asset_manager.h"
#include "editor/asset/editor_runtime_asset_bridge.h"
#include "util/path_util.h"

namespace shine::editor::assets_brower
{
    void AssetsBrower::onInit()
    {
        SetName(title.to_string());
        if (EngineContext::IsInitialized())
        {
            assetDirectoryService_ = EngineContext::Get().GetSystem<editor::asset::AssetDirectoryService>();
            editorAssetManager_ = EngineContext::Get().GetSystem<editor::asset::EditorAssetManager>();
            runtimeAssetBridge_ = EngineContext::Get().GetSystem<editor::asset::EditorRuntimeAssetBridge>();
        }
        if (assetDirectoryService_)
        {
            assetDirectoryService_->RefreshIfDirty();
            selectedDirectory_ = assetDirectoryService_->GetContentRootPath();
        }
    }

    void AssetsBrower::onRender()
    {
        if (assetDirectoryService_)
        {
            assetDirectoryService_->RefreshIfDirty();
        }
        if (ImGui::Begin(name.c_str(), &isOpen))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::MenuItem("刷新") && assetDirectoryService_)
                {
                    assetDirectoryService_->ForceRefresh();
                    assetDirectoryService_->RefreshIfDirty();
                }
                if (ImGui::BeginMenu("视图"))
                {
                    ImGui::SliderFloat("图标尺寸", &iconSize_, 32.0f, 128.0f, "%.0f");
                    ImGui::SliderFloat("图标间距", &iconSpacing_, 4.0f, 32.0f, "%.0f");
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::BeginChild("AssetsTree", ImVec2(260, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
            ImGui::InputTextWithHint("##asset_search", "搜索文件", searchBuffer_, sizeof(searchBuffer_));
            const std::filesystem::path contentRootPath = assetDirectoryService_ ? assetDirectoryService_->GetContentRootPath() : std::filesystem::path{};
            if (ImGui::TreeNodeEx(ToDisplayText(contentRootPath).c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow))
            {
                if (assetDirectoryService_)
                {
                    for (const auto& path : assetDirectoryService_->GetRootDirectories())
                    {
                        RenderDirectoryNode(path);
                    }
                }
                ImGui::TreePop();
            }
            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("AssetsList", ImVec2(0, 0), ImGuiChildFlags_Borders);
            ImGui::TextUnformatted(ToDisplayText(selectedDirectory_).c_str());
            ImGui::Separator();
            RenderAssetGrid();
            RenderOperationsPopup();
            ImGui::EndChild();
        }

        ImGui::End();

        CheckIsOpenChange();
    }

    void AssetsBrower::onShutDown()
    {
        assetDirectoryService_ = nullptr;
        editorAssetManager_ = nullptr;
        runtimeAssetBridge_ = nullptr;
    }

    void AssetsBrower::RenderDirectoryNode(const std::filesystem::path& path)
    {
        if (!assetDirectoryService_)
        {
            return;
        }

        std::error_code ec;
        const bool selected = std::filesystem::equivalent(path, selectedDirectory_, ec);
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (selected)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        const bool opened = ImGui::TreeNodeEx(path.string().c_str(), flags, "%s", ToDisplayText(path.filename()).c_str());
        if (ImGui::IsItemClicked())
        {
            selectedDirectory_ = path;
            selectedEntryPath_.clear();
        }
        if (!opened)
        {
            return;
        }

        for (const auto& childPath : assetDirectoryService_->GetChildDirectories(path))
        {
            RenderDirectoryNode(childPath);
        }
        ImGui::TreePop();
    }

    void AssetsBrower::RenderAssetGrid()
    {
        const auto& entries = assetDirectoryService_ ? assetDirectoryService_->GetEntries(selectedDirectory_) : std::vector<std::filesystem::directory_entry>{};
        filteredEntryIndices_.clear();
        filteredEntryIndices_.reserve(entries.size());
        for (int i = 0; i < static_cast<int>(entries.size()); ++i)
        {
            if (PassesSearchFilter(entries[i].path()))
            {
                filteredEntryIndices_.push_back(i);
            }
        }
        if (filteredEntryIndices_.empty())
        {
            ImGui::TextUnformatted("当前目录没有可显示项目");
            return;
        }

        const float itemStepX = iconSize_ + iconSpacing_;
        const float itemStepY = GetGridItemHeight() + iconSpacing_;
        const float availWidth = ImGui::GetContentRegionAvail().x;
        const int columnCount = std::max(1, static_cast<int>(availWidth / std::max(itemStepX, 1.0f)));
        const int lineCount = (static_cast<int>(filteredEntryIndices_.size()) + columnCount - 1) / columnCount;
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        ImGui::SetNextWindowContentSize(ImVec2(0.0f, lineCount * itemStepY));

        ImGuiListClipper clipper;
        clipper.Begin(lineCount, itemStepY);
        while (clipper.Step())
        {
            for (int lineIndex = clipper.DisplayStart; lineIndex < clipper.DisplayEnd; ++lineIndex)
            {
                const int beginIndex = lineIndex * columnCount;
                const int endIndex = std::min(beginIndex + columnCount, static_cast<int>(filteredEntryIndices_.size()));
                for (int i = beginIndex; i < endIndex; ++i)
                {
                    const int columnIndex = i - beginIndex;
                    const int sourceIndex = filteredEntryIndices_[i];
                    RenderAssetItem(entries[sourceIndex], sourceIndex, startPos, columnCount, lineIndex, columnIndex);
                }
            }
        }
    }

    void AssetsBrower::RenderAssetItem(const std::filesystem::directory_entry& entry, int index, const ImVec2& startPos, int columnCount, int lineIndex, int columnIndex)
    {
        const float itemStepX = iconSize_ + iconSpacing_;
        const float itemStepY = GetGridItemHeight() + iconSpacing_;
        const ImVec2 pos = ImVec2(startPos.x + columnIndex * itemStepX, startPos.y + lineIndex * itemStepY);
        const ImVec2 itemSize = ImVec2(iconSize_, GetGridItemHeight());

        ImGui::PushID(index);
        ImGui::SetCursorScreenPos(pos);
        const bool isSelected = (selectedEntryPath_ == entry.path());
        ImGui::Selectable("##asset_item", isSelected, ImGuiSelectableFlags_AllowDoubleClick, itemSize);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            selectedEntryPath_ = entry.path();
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            OpenEntry(entry);
        }
        RenderAssetContextMenu(entry);

        if (ImGui::BeginDragDropSource())
        {
            const auto payload = entry.path().string();
            ImGui::SetDragDropPayload("SHINE_ASSET_PATH", payload.data(), payload.size() + 1);
            ImGui::TextUnformatted(ToDisplayText(entry.path().filename()).c_str());
            ImGui::EndDragDropSource();
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        std::error_code ec;
        const bool isDir = entry.is_directory(ec);
        const ImU32 iconColor = isDir ? IM_COL32(94, 151, 255, 230) : IM_COL32(80, 80, 80, 230);
        const ImU32 borderColor = isSelected ? IM_COL32(255, 205, 80, 255) : IM_COL32(150, 150, 150, 120);
        const ImVec2 iconMin = ImVec2(pos.x + 4.0f, pos.y + 4.0f);
        const ImVec2 iconMax = ImVec2(pos.x + iconSize_ - 4.0f, pos.y + iconSize_ - 8.0f);
        drawList->AddRectFilled(iconMin, iconMax, iconColor, 6.0f);
        drawList->AddRect(iconMin, iconMax, borderColor, 6.0f, 0, isSelected ? 2.0f : 1.0f);
        drawList->AddText(ImVec2(iconMin.x + 8.0f, iconMin.y + 6.0f), IM_COL32(255, 255, 255, 255), isDir ? "DIR" : "FILE");

        auto label = ToDisplayText(entry.path().filename()).to_string();
        const float maxLabelWidth = iconSize_ - 8.0f;
        while (!label.empty() && ImGui::CalcTextSize(label.c_str()).x > maxLabelWidth)
        {
            label.pop_back();
        }
        if (label != ToDisplayText(entry.path().filename()).to_string() && label.size() > 2)
        {
            label.replace(label.end() - 2, label.end(), "..");
        }
        drawList->AddText(ImVec2(pos.x + 4.0f, pos.y + iconSize_ - 2.0f), IM_COL32(230, 230, 230, 255), label.c_str());
        ImGui::PopID();
        (void)columnCount;
    }

    void AssetsBrower::RenderAssetContextMenu(const std::filesystem::directory_entry& entry)
    {
        if (!ImGui::BeginPopupContextItem("AssetContext"))
        {
            return;
        }
        contextEntryPath_ = entry.path();
        if (ImGui::MenuItem("打开"))
        {
            OpenEntry(entry);
        }
        if (ImGui::MenuItem("重命名"))
        {
            const auto name = entry.path().filename().string();
            std::memset(renameBuffer_, 0, sizeof(renameBuffer_));
            std::strncpy(renameBuffer_, name.c_str(), sizeof(renameBuffer_) - 1);
            requestRenamePopup_ = true;
        }
        if (ImGui::MenuItem("移动"))
        {
            const auto dir = selectedDirectory_.string();
            std::memset(moveTargetBuffer_, 0, sizeof(moveTargetBuffer_));
            std::strncpy(moveTargetBuffer_, dir.c_str(), sizeof(moveTargetBuffer_) - 1);
            requestMovePopup_ = true;
        }
        if (ImGui::MenuItem("删除"))
        {
            requestDeletePopup_ = true;
        }
        ImGui::EndPopup();
    }

    void AssetsBrower::RenderOperationsPopup()
    {
        if (requestRenamePopup_)
        {
            ImGui::OpenPopup("AssetRenamePopup");
            requestRenamePopup_ = false;
        }
        if (ImGui::BeginPopupModal("AssetRenamePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("新名称", renameBuffer_, sizeof(renameBuffer_));
            if (ImGui::Button("确认"))
            {
                if (RenameEntry(contextEntryPath_, shine::SString(renameBuffer_)))
                {
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("取消"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (requestMovePopup_)
        {
            ImGui::OpenPopup("AssetMovePopup");
            requestMovePopup_ = false;
        }
        if (ImGui::BeginPopupModal("AssetMovePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("目标目录", moveTargetBuffer_, sizeof(moveTargetBuffer_));
            if (ImGui::Button("确认"))
            {
                std::filesystem::path targetDirectory = std::filesystem::path(moveTargetBuffer_);
                if (!targetDirectory.is_absolute() && assetDirectoryService_)
                {
                    targetDirectory = assetDirectoryService_->GetContentRootPath() / targetDirectory;
                }
                if (MoveEntry(contextEntryPath_, targetDirectory))
                {
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("取消"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (requestDeletePopup_)
        {
            ImGui::OpenPopup("AssetDeletePopup");
            requestDeletePopup_ = false;
        }
        if (ImGui::BeginPopupModal("AssetDeletePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("确认删除该资产?");
            if (ImGui::Button("删除"))
            {
                if (DeleteEntry(contextEntryPath_))
                {
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("取消"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void AssetsBrower::OpenEntry(const std::filesystem::directory_entry& entry)
    {
        std::error_code ec;
        if (entry.is_directory(ec))
        {
            selectedDirectory_ = entry.path();
            selectedEntryPath_.clear();
            return;
        }
        auto record = FindAssetRecordByPath(entry.path());
        if (!record.has_value() || !runtimeAssetBridge_)
        {
            return;
        }
        if (record->type == editor::asset::EEditorAssetType::Scene)
        {
            auto result = runtimeAssetBridge_->ActivateWorldMapByEditorId(record->assetID);
            (void)result;
            return;
        }
        auto result = runtimeAssetBridge_->LoadRuntimeAssetByEditorId(record->assetID);
        (void)result;
    }

    bool AssetsBrower::RenameEntry(const std::filesystem::path& sourcePath, const shine::SString& newName)
    {
        if (sourcePath.empty() || newName.empty())
        {
            return false;
        }
        const auto targetPath = sourcePath.parent_path() / std::filesystem::path(newName.to_string());
        if (targetPath == sourcePath)
        {
            return false;
        }
        std::error_code ec;
        std::filesystem::rename(sourcePath, targetPath, ec);
        if (ec)
        {
            return false;
        }
        SyncAssetRecordMove(sourcePath, targetPath);
        if (selectedEntryPath_ == sourcePath)
        {
            selectedEntryPath_ = targetPath;
        }
        if (contextEntryPath_ == sourcePath)
        {
            contextEntryPath_ = targetPath;
        }
        if (assetDirectoryService_)
        {
            assetDirectoryService_->ForceRefresh();
            assetDirectoryService_->RefreshIfDirty();
        }
        return true;
    }

    bool AssetsBrower::DeleteEntry(const std::filesystem::path& path)
    {
        if (path.empty())
        {
            return false;
        }
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec))
        {
            std::filesystem::remove_all(path, ec);
        }
        else
        {
            std::filesystem::remove(path, ec);
        }
        if (ec)
        {
            return false;
        }
        SyncAssetRecordDelete(path);
        if (selectedEntryPath_ == path)
        {
            selectedEntryPath_.clear();
        }
        if (contextEntryPath_ == path)
        {
            contextEntryPath_.clear();
        }
        if (assetDirectoryService_)
        {
            assetDirectoryService_->ForceRefresh();
            assetDirectoryService_->RefreshIfDirty();
        }
        return true;
    }

    bool AssetsBrower::MoveEntry(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationDirectory)
    {
        if (sourcePath.empty() || destinationDirectory.empty())
        {
            return false;
        }
        std::error_code ec;
        std::filesystem::create_directories(destinationDirectory, ec);
        if (ec)
        {
            return false;
        }
        const auto targetPath = destinationDirectory / sourcePath.filename();
        std::filesystem::rename(sourcePath, targetPath, ec);
        if (ec)
        {
            return false;
        }
        SyncAssetRecordMove(sourcePath, targetPath);
        if (selectedEntryPath_ == sourcePath)
        {
            selectedEntryPath_ = targetPath;
        }
        if (contextEntryPath_ == sourcePath)
        {
            contextEntryPath_ = targetPath;
        }
        if (assetDirectoryService_)
        {
            assetDirectoryService_->ForceRefresh();
            assetDirectoryService_->RefreshIfDirty();
        }
        return true;
    }

    std::optional<editor::asset::EditorAssetRecord> AssetsBrower::FindAssetRecordByPath(const std::filesystem::path& path) const
    {
        if (!editorAssetManager_)
        {
            return std::nullopt;
        }
        std::vector<std::string> candidates;
        candidates.push_back(path.generic_string());
        if (assetDirectoryService_)
        {
            std::error_code ec;
            const auto rel = std::filesystem::relative(path, assetDirectoryService_->GetContentRootPath(), ec);
            if (!ec)
            {
                candidates.push_back(rel.generic_string());
            }
        }
        for (const auto& candidate : candidates)
        {
            auto result = editorAssetManager_->GetAssetRecordByPath(candidate);
            if (result.has_value())
            {
                return result.value();
            }
        }
        return std::nullopt;
    }

    void AssetsBrower::SyncAssetRecordMove(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
    {
        if (!editorAssetManager_)
        {
            return;
        }
        const bool sourceIsDirectory = std::filesystem::is_directory(oldPath);
        std::string oldAbsolute = util::normalize_asset_path(oldPath.generic_string());
        std::string newAbsolute = util::normalize_asset_path(newPath.generic_string());
        std::string oldRelative;
        std::string newRelative;
        if (assetDirectoryService_)
        {
            std::error_code ec;
            oldRelative = util::normalize_asset_path(std::filesystem::relative(oldPath, assetDirectoryService_->GetContentRootPath(), ec).generic_string());
            ec = {};
            newRelative = util::normalize_asset_path(std::filesystem::relative(newPath, assetDirectoryService_->GetContentRootPath(), ec).generic_string());
        }
        if (newRelative.empty())
        {
            newRelative = newAbsolute;
        }

        auto remapField = [&](std::string& field) {
            auto normalized = util::normalize_asset_path(field);
            if (normalized.empty())
            {
                return false;
            }
            if (!sourceIsDirectory)
            {
                if (normalized == oldAbsolute || (!oldRelative.empty() && normalized == oldRelative))
                {
                    field = newRelative;
                    return true;
                }
                return false;
            }

            auto remapPrefix = [&](const std::string& fromPrefix, const std::string& toPrefix) {
                if (fromPrefix.empty())
                {
                    return false;
                }
                if (normalized == fromPrefix)
                {
                    field = toPrefix;
                    return true;
                }
                if (normalized.starts_with(fromPrefix + "/"))
                {
                    field = toPrefix + normalized.substr(fromPrefix.size());
                    return true;
                }
                return false;
            };
            if (remapPrefix(oldRelative, newRelative))
            {
                return true;
            }
            return remapPrefix(oldAbsolute, newAbsolute);
        };

        auto allRecords = editorAssetManager_->GetAllAssetRecords();
        for (auto& record : allRecords)
        {
            bool changed = false;
            changed |= remapField(record.logicalPath);
            changed |= remapField(record.sourcePath);
            changed |= remapField(record.packagePath);
            if (!changed)
            {
                continue;
            }
            editorAssetManager_->UpsertAssetRecord(record);
            if (const auto runtimeAsset = editorAssetManager_->GetAsset(record.assetID))
            {
                runtimeAsset->SetPath(record.logicalPath.empty() ? record.sourcePath : record.logicalPath);
            }
        }
    }

    void AssetsBrower::SyncAssetRecordDelete(const std::filesystem::path& path)
    {
        if (!editorAssetManager_)
        {
            return;
        }
        const bool deletingDirectory = std::filesystem::is_directory(path);
        if (!deletingDirectory)
        {
            auto record = FindAssetRecordByPath(path);
            if (!record.has_value())
            {
                return;
            }
            editorAssetManager_->RemoveAsset(record->assetID);
            return;
        }

        std::string absolutePrefix = util::normalize_asset_path(path.generic_string());
        std::string relativePrefix;
        if (assetDirectoryService_)
        {
            std::error_code ec;
            relativePrefix = util::normalize_asset_path(std::filesystem::relative(path, assetDirectoryService_->GetContentRootPath(), ec).generic_string());
        }

        auto allRecords = editorAssetManager_->GetAllAssetRecords();
        for (const auto& record : allRecords)
        {
            auto underPrefix = [&](const std::string& candidate) {
                const auto normalized = util::normalize_asset_path(candidate);
                if (normalized.empty())
                {
                    return false;
                }
                auto matchPrefix = [&](const std::string& prefix) {
                    if (prefix.empty())
                    {
                        return false;
                    }
                    if (normalized == prefix)
                    {
                        return true;
                    }
                    return normalized.starts_with(prefix + "/");
                };
                return matchPrefix(relativePrefix) || matchPrefix(absolutePrefix);
            };

            if (underPrefix(record.logicalPath) || underPrefix(record.sourcePath) || underPrefix(record.packagePath))
            {
                editorAssetManager_->RemoveAsset(record.assetID);
            }
        }
    }

    shine::SString AssetsBrower::ToDisplayText(const std::filesystem::path& path) const
    {
        auto name = path.filename().string();
        if (name.empty())
        {
            name = path.string();
        }
        return shine::SString(name);
    }

    bool AssetsBrower::PassesSearchFilter(const std::filesystem::path& path) const
    {
        std::string filter = searchBuffer_;
        std::transform(filter.begin(), filter.end(), filter.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (filter.empty())
        {
            return true;
        }
        auto lower = ToDisplayText(path.filename()).to_string();
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return lower.find(filter) != std::string::npos;
    }

    float AssetsBrower::GetGridItemHeight() const
    {
        return iconSize_ + ImGui::GetFontSize() + 10.0f;
    }
}
