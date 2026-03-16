#include "AssetsBrower.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <queue>
#include <string>

#include "EngineCore/engine_context.h"
#include "editor/ShineAsset/registry/EditorAssetRegistry.h"
#include "editor/ShineAsset/importers/ImportPipeline.h"
#include "editor/ShineAsset/importers/IAssetImporter.h"
#include "util/EngineDirectoryService.h"
#include "editor/browers/AssetDropQueue.h"
#include "editor/browers/builtin_thumbnail_providers.h"

namespace shine::editor::assets_brower
{
    // -----------------------------------------------------------------------
    //  外部文件拖放队列（跨线程安全）
    // -----------------------------------------------------------------------
    namespace
    {
        std::mutex                         g_dropMutex;
        std::queue<std::filesystem::path>  g_dropQueue;
    }

    void EnqueueExternalDrop(std::vector<std::filesystem::path> paths)
    {
        std::lock_guard<std::mutex> lock(g_dropMutex);
        for (auto& p : paths)
            g_dropQueue.push(std::move(p));
    }
    void AssetsBrower::onInit()
    {
        SetName(title.to_string());
        if (EngineContext::IsInitialized())
        {
            auto& ctx = EngineContext::Get();
            editorAssetRegistry_ = ctx.GetSystem<shine::editor::asset::EditorAssetRegistry>();
            importPipeline_      = ctx.GetSystem<shine::editor::asset::ImportPipeline>();

            auto* dirService = ctx.GetSystem<util::EngineDirectoryService>();
            if (dirService && !dirService->GetContentDirectory().empty())
            {
                contentRoot_       = dirService->GetContentDirectory();
                selectedDirectory_ = contentRoot_;
            }
        }

        // Register built-in thumbnail providers (Image + Model).
        // Call RegisterThumbnailProvider() before or after this to control priority.
        RegisterBuiltinThumbnailProviders(thumbnailRegistry_);
    }

    void AssetsBrower::onRender()
    {
        thumbnailRegistry_.TickAll();

        // 每帧从外部拖放队列取一条路径，触发导入弹窗（非阻塞）
        if (!requestImportPopup_)
        {
            std::lock_guard<std::mutex> lock(g_dropMutex);
            if (!g_dropQueue.empty())
            {
                auto path = std::move(g_dropQueue.front());
                g_dropQueue.pop();
                if (importPipeline_)
                {
                    ImportPending pending;
                    pending.sourcePath = std::move(path);
                    pending.importer   = importPipeline_->FindImporter(pending.sourcePath);
                    pendingImport_     = std::move(pending);
                    importErrorMsg_.clear();
                    requestImportPopup_ = true;
                }
            }
        }

        if (ImGui::Begin(name.c_str(), &isOpen))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::MenuItem("刷新"))
                {
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
            if (!contentRoot_.empty())
            {
                RenderDirectoryNode(contentRoot_);
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
        // Release GPU textures held by thumbnail providers while the
        // engine context (TextureManager) is still alive.
        thumbnailRegistry_.Clear();
    }

    void AssetsBrower::RenderDirectoryNode(const std::filesystem::path& path)
    {
        std::error_code ec;
        const bool selected = !selectedDirectory_.empty() &&
                              std::filesystem::equivalent(path, selectedDirectory_, ec) && !ec;
        const bool isRoot   = !contentRoot_.empty() &&
                              std::filesystem::equivalent(path, contentRoot_, ec) && !ec;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (selected)
            flags |= ImGuiTreeNodeFlags_Selected;
        if (isRoot)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        const bool opened = ImGui::TreeNodeEx(path.string().c_str(), flags, "%s", ToDisplayText(path.filename()).c_str());
        if (ImGui::IsItemClicked())
        {
            selectedDirectory_ = path;
        }
        RenderDirectoryContextMenu(path);
        if (!opened)
        {
            return;
        }

        {
            std::vector<std::filesystem::path> childDirs;
            std::error_code iterEc;
            for (const auto& fse : std::filesystem::directory_iterator(path, iterEc))
            {
                std::error_code isDirEc;
                if (fse.is_directory(isDirEc))
                {
                    childDirs.push_back(fse.path());
                }
            }
            for (const auto& childPath : childDirs)
            {
                RenderDirectoryNode(childPath);
            }
        }
        ImGui::TreePop();
    }

    // -----------------------------------------------------------------------
    //  PathToID — 将磁盘路径映射为稳定 ImGuiID（FNV-1a 32-bit）
    // -----------------------------------------------------------------------
    ImGuiID AssetsBrower::PathToID(const std::filesystem::path& path)
    {
        const auto s = path.string();
        uint32_t h = 2166136261u;
        for (const unsigned char c : s) { h ^= c; h *= 16777619u; }
        return static_cast<ImGuiID>(h);
    }

    // -----------------------------------------------------------------------
    //  UpdateLayoutSizes — 根据可用宽度计算网格布局参数
    // -----------------------------------------------------------------------
    void AssetsBrower::UpdateLayoutSizes(float availWidth, int itemCount)
    {
        const float labelH   = ImGui::GetFontSize() + 6.0f;
        const float rawItemW = floorf(iconSize_);
        const float rawItemH = floorf(iconSize_) + labelH;

        float spacing = iconSpacing_;
        float aw = availWidth;
        if (stretchSpacing_)
            aw += floorf(spacing * 0.5f);

        layoutColumnCount_ = std::max(1, static_cast<int>(aw / (rawItemW + spacing)));
        layoutLineCount_   = (itemCount + layoutColumnCount_ - 1) / layoutColumnCount_;

        if (stretchSpacing_ && layoutColumnCount_ > 1)
            spacing = floorf(aw - rawItemW * static_cast<float>(layoutColumnCount_))
                      / static_cast<float>(layoutColumnCount_);

        layoutItemSize_          = ImVec2(rawItemW, rawItemH);
        layoutItemStep_          = ImVec2(rawItemW + spacing, rawItemH + spacing);
        layoutItemSpacing_       = spacing;
        layoutSelectableSpacing_ = std::max(floorf(spacing) - static_cast<float>(iconHitSpacing_), 0.0f);
        layoutOuterPadding_      = floorf(spacing * 0.5f);
    }

    // -----------------------------------------------------------------------
    //  RenderAssetGrid — 基于 ImGuiMultiSelect 的资产网格
    //  支持：框选、键盘导航、Ctrl+滚轮缩放、虚拟列表裁剪
    // -----------------------------------------------------------------------
    void AssetsBrower::RenderAssetGrid()
    {
        // 1. 扫描并过滤当前目录
        // 只显示：子目录 + .sasset 文件；原始源文件（.png/.glb 等）不在网格展示
        gridEntries_.clear();
        if (!selectedDirectory_.empty())
        {
            std::error_code scanEc;
            for (const auto& fse : std::filesystem::directory_iterator(selectedDirectory_, scanEc))
            {
                std::error_code ec;
                const bool isDir    = fse.is_directory(ec);
                const bool isSAsset = !isDir && fse.path().extension() == ".sasset";
                if ((isDir || isSAsset) && PassesSearchFilter(fse.path()))
                    gridEntries_.push_back(fse);
            }
        }

        if (gridEntries_.empty())
        {
            ImGui::TextUnformatted("当前目录没有可显示项目");
            return;
        }

        const int itemCount = static_cast<int>(gridEntries_.size());

        // 2. 初步计算布局（供 SetNextWindowContentSize 使用）
        UpdateLayoutSizes(ImGui::GetContentRegionAvail().x, itemCount);
        ImGui::SetNextWindowContentSize(
            ImVec2(0.0f, layoutOuterPadding_ + static_cast<float>(layoutLineCount_) * layoutItemStep_.y));

        if (!ImGui::BeginChild("AssetGridScroll", ImVec2(0.0f, 0.0f),
                               ImGuiChildFlags_None, ImGuiWindowFlags_NoMove))
        {
            ImGui::EndChild();
            return;
        }

        // 3. 用 child 内实际宽度重新计算布局
        const float availWidth = ImGui::GetContentRegionAvail().x;
        UpdateLayoutSizes(availWidth, itemCount);

        ImDrawList*     drawList = ImGui::GetWindowDrawList();
        const ImGuiIO&  io       = ImGui::GetIO();

        ImVec2 startPos = ImGui::GetCursorScreenPos();
        startPos.x += layoutOuterPadding_;
        startPos.y += layoutOuterPadding_;
        ImGui::SetCursorScreenPos(startPos);

        // 4. BeginMultiSelect
        const ImGuiMultiSelectFlags msFlags =
            ImGuiMultiSelectFlags_ClearOnEscape    |
            ImGuiMultiSelectFlags_ClearOnClickVoid |
            ImGuiMultiSelectFlags_BoxSelect2d      |
            ImGuiMultiSelectFlags_NavWrapX;

        ImGuiMultiSelectIO* msIo = ImGui::BeginMultiSelect(msFlags, selection_.Size, itemCount);

        selection_.UserData = this;
        selection_.AdapterIndexToStorageId = [](ImGuiSelectionBasicStorage* self_, int idx)
        {
            auto* b = static_cast<AssetsBrower*>(self_->UserData);
            return PathToID(b->gridEntries_[idx].path());
        };
        selection_.ApplyRequests(msIo);

        // 5. 压缩 ItemSpacing（保留框选空隙）
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
            ImVec2(layoutSelectableSpacing_, layoutSelectableSpacing_));

        // 6. 虚拟列表裁剪（按行）
        ImGuiListClipper clipper;
        clipper.Begin(layoutLineCount_, layoutItemStep_.y);
        if (msIo->RangeSrcItem != -1)
            clipper.IncludeItemByIndex(static_cast<int>(msIo->RangeSrcItem) / layoutColumnCount_);

        const bool displayLabel = (layoutItemSize_.x >= ImGui::CalcTextSize("WWW").x);
        const ImU32 iconBgColor  = ImGui::GetColorU32(IM_COL32(35, 35, 35, 220));

        while (clipper.Step())
        {
            for (int lineIdx = clipper.DisplayStart; lineIdx < clipper.DisplayEnd; ++lineIdx)
            {
                const int itemMin = lineIdx * layoutColumnCount_;
                const int itemMax = std::min(itemMin + layoutColumnCount_, itemCount);

                for (int itemIdx = itemMin; itemIdx < itemMax; ++itemIdx)
                {
                    const auto&   entry  = gridEntries_[itemIdx];
                    const ImGuiID itemId = PathToID(entry.path());
                    ImGui::PushID(static_cast<int>(itemId));

                    const ImVec2 pos(
                        startPos.x + static_cast<float>(itemIdx % layoutColumnCount_) * layoutItemStep_.x,
                        startPos.y + static_cast<float>(lineIdx)                      * layoutItemStep_.y);
                    ImGui::SetCursorScreenPos(pos);

                    ImGui::SetNextItemSelectionUserData(itemIdx);
                    bool itemIsSelected = selection_.Contains(itemId);
                    ImGui::Selectable("##item", itemIsSelected,
                                      ImGuiSelectableFlags_None, layoutItemSize_);

                    if (ImGui::IsItemToggledSelection())
                        itemIsSelected = !itemIsSelected;

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        OpenEntry(entry);

                    RenderAssetContextMenu(entry);

                    // 拖拽源
                    if (ImGui::BeginDragDropSource())
                    {
                        const auto pathStr = entry.path().string();
                        ImGui::SetDragDropPayload("SHINE_ASSET_PATH",
                            pathStr.data(), pathStr.size() + 1);
                        if (selection_.Size > 1)
                            ImGui::Text("移动 %d 个项目", selection_.Size);
                        else
                            ImGui::TextUnformatted(ToDisplayText(entry.path().filename()).c_str());
                        ImGui::EndDragDropSource();
                    }

                    // 图标渲染（仅对可见区域）
                    if (ImGui::IsRectVisible(layoutItemSize_))
                    {
                        std::error_code ec;
                        const bool isDir = entry.is_directory(ec);

                        const ImVec2 boxMin(pos.x - 1.0f, pos.y - 1.0f);
                        const ImVec2 boxMax(pos.x + layoutItemSize_.x + 1.0f,
                                           pos.y + layoutItemSize_.y + 1.0f);
                        drawList->AddRectFilled(boxMin, boxMax, iconBgColor);

                        const ImVec2 iconMin(pos.x + 4.0f, pos.y + 4.0f);
                        const ImVec2 iconMax(pos.x + layoutItemSize_.x - 4.0f,
                                            pos.y + layoutItemSize_.y - ImGui::GetFontSize() - 8.0f);

                        // Ask registered providers first; fall back to default coloured rect
                        bool thumbnailDrawn = false;
                        if (auto* provider = thumbnailRegistry_.Find(entry.path()))
                            thumbnailDrawn = provider->DrawThumbnail(drawList, entry.path(),
                                                                     iconMin, iconMax, itemIsSelected);

                        if (!thumbnailDrawn)
                        {
                            const ImU32 iconColor   = isDir ? IM_COL32(94, 151, 255, 230) : IM_COL32(80, 80, 80, 230);
                            const ImU32 borderColor = itemIsSelected ? IM_COL32(255, 205, 80, 255) : IM_COL32(150, 150, 150, 120);
                            drawList->AddRectFilled(iconMin, iconMax, iconColor, 6.0f);
                            drawList->AddRect(iconMin, iconMax, borderColor, 6.0f, 0,
                                              itemIsSelected ? 2.0f : 1.0f);
                            drawList->AddText(ImVec2(iconMin.x + 8.0f, iconMin.y + 6.0f),
                                IM_COL32(255, 255, 255, 255), isDir ? "DIR" : "FILE");
                        }

                        if (displayLabel)
                        {
                            auto label = ToDisplayText(entry.path().filename()).to_string();
                            const float maxW = layoutItemSize_.x - 8.0f;
                            while (!label.empty() && ImGui::CalcTextSize(label.c_str()).x > maxW)
                                label.pop_back();
                            if (label.size() > 2 && !label.empty() &&
                                label.back() != entry.path().filename().string().back())
                                label.replace(label.end() - 2, label.end(), "..");
                            drawList->AddText(
                                ImVec2(pos.x + 4.0f,
                                       pos.y + layoutItemSize_.y - ImGui::GetFontSize() - 2.0f),
                                ImGui::GetColorU32(itemIsSelected ? ImGuiCol_Text : ImGuiCol_TextDisabled),
                                label.c_str());
                        }
                    }

                    ImGui::PopID();
                }
            }
        }
        clipper.End();
        ImGui::PopStyleVar(); // ItemSpacing

        // 7. 结束 multi-select
        msIo = ImGui::EndMultiSelect();
        selection_.ApplyRequests(msIo);

        // 8. Ctrl+滚轮缩放（与 imgui_demo ExampleAssetsBrowser 相同逻辑）
        if (ImGui::IsWindowAppearing())
            zoomWheelAccum_ = 0.0f;
        if (ImGui::IsWindowHovered() && io.MouseWheel != 0.0f &&
            ImGui::IsKeyDown(ImGuiMod_Ctrl) && !ImGui::IsAnyItemActive())
        {
            zoomWheelAccum_ += io.MouseWheel;
            if (fabsf(zoomWheelAccum_) >= 1.0f)
            {
                const float hoverNx  = (io.MousePos.x - startPos.x + layoutItemSpacing_ * 0.5f) / layoutItemStep_.x;
                const float hoverNy  = (io.MousePos.y - startPos.y + layoutItemSpacing_ * 0.5f) / layoutItemStep_.y;
                const int   hoverIdx = static_cast<int>(hoverNy) * layoutColumnCount_
                                     + static_cast<int>(hoverNx);

                iconSize_ *= powf(1.1f, static_cast<float>(static_cast<int>(zoomWheelAccum_)));
                iconSize_  = std::clamp(iconSize_, 16.0f, 128.0f);
                zoomWheelAccum_ -= static_cast<float>(static_cast<int>(zoomWheelAccum_));
                UpdateLayoutSizes(availWidth, itemCount);

                const float relY   = (static_cast<float>(hoverIdx / layoutColumnCount_) +
                                      fmodf(hoverNy, 1.0f)) * layoutItemStep_.y;
                const float mouseY = io.MousePos.y - ImGui::GetWindowPos().y;
                ImGui::SetScrollY(relY - mouseY);
            }
        }

        ImGui::EndChild(); // AssetGridScroll
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
        // 导入弹窗
        RenderImportPopup();

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
            // Check for dependents before allowing deletion
            bool hasDependents = false;
            if (editorAssetRegistry_ && contextEntryPath_.extension() == ".sasset")
            {
                const auto* assetEntry = editorAssetRegistry_->FindByPath(contextEntryPath_);
                if (assetEntry)
                {
                    const auto& dependents = editorAssetRegistry_->GetDependents(assetEntry->uuid);
                    if (!dependents.empty())
                    {
                        hasDependents = true;
                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f),
                            "⚠ 该资产被 %zu 个其他资产依赖!", dependents.size());
                        ImGui::TextUnformatted("依赖的资产:");
                        for (const auto& depUuid : dependents)
                        {
                            const auto* depEntry = editorAssetRegistry_->Find(depUuid);
                            if (depEntry)
                                ImGui::BulletText("%s (%s)", depEntry->diskPath.c_str(), depEntry->record.type.c_str());
                            else
                                ImGui::BulletText("%s", depUuid.c_str());
                        }
                        ImGui::Separator();
                        ImGui::TextUnformatted("确认强制删除? 这会导致悬挂引用!");
                    }
                }
            }

            if (!hasDependents)
                ImGui::TextUnformatted("确认删除该资产?");

            if (ImGui::Button(hasDependents ? "强制删除" : "删除"))
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

        // -----------------------------------------------------------------------
        //  目录操作弹窗
        // -----------------------------------------------------------------------
        if (requestNewFolderPopup_)
        {
            ImGui::OpenPopup("DirNewFolderPopup");
            requestNewFolderPopup_ = false;
        }
        if (ImGui::BeginPopupModal("DirNewFolderPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("新建文件夹于:");
            ImGui::TextUnformatted(contextDirPath_.string().c_str());
            ImGui::Separator();
            ImGui::InputText("文件夹名称", newFolderBuffer_, sizeof(newFolderBuffer_));
            if (ImGui::Button("创建"))
            {
                if (CreateFolder(contextDirPath_, shine::SString(newFolderBuffer_)))
                    ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("取消"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (requestRenameDirPopup_)
        {
            ImGui::OpenPopup("DirRenamePopup");
            requestRenameDirPopup_ = false;
        }
        if (ImGui::BeginPopupModal("DirRenamePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("新名称", renameDirBuffer_, sizeof(renameDirBuffer_));
            if (ImGui::Button("确认"))
            {
                if (RenameDirectory(contextDirPath_, shine::SString(renameDirBuffer_)))
                    ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("取消"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (requestDeleteDirPopup_)
        {
            ImGui::OpenPopup("DirDeletePopup");
            requestDeleteDirPopup_ = false;
        }
        if (ImGui::BeginPopupModal("DirDeletePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "⚠ 将删除整个文件夹及其内容!");
            ImGui::TextUnformatted(contextDirPath_.string().c_str());
            if (ImGui::Button("确认删除"))
            {
                if (DeleteDirectory(contextDirPath_))
                    ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("取消"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    // -----------------------------------------------------------------------
    //  RenderImportPopup — 导入设置弹窗
    //  由 RenderOperationsPopup() 调用；requestImportPopup_ 控制开关。
    // -----------------------------------------------------------------------
    void AssetsBrower::RenderImportPopup()
    {
        if (requestImportPopup_)
        {
            ImGui::OpenPopup("##ImportSettings");
            requestImportPopup_ = false;
        }

        if (!ImGui::BeginPopupModal("##ImportSettings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        if (!pendingImport_.has_value())
        {
            ImGui::EndPopup();
            return;
        }

        auto& pending = *pendingImport_;

        ImGui::Text("导入: %s", pending.sourcePath.filename().string().c_str());
        ImGui::Separator();

        const bool hasImporter = (pending.importer != nullptr);
        if (!hasImporter)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "不支持的文件类型，无法导入！");
        }
        else
        {
            // 将 std::string 包装为 glz::raw_json 交给导入器渲染设置 UI
            glz::raw_json rawJson{ pending.settingsJson };
            if (pending.importer->RenderImportSettingsUI(rawJson))
                pending.settingsJson = rawJson.str;
        }

        if (!importErrorMsg_.empty())
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", importErrorMsg_.c_str());

        ImGui::Separator();

        ImGui::BeginDisabled(!hasImporter || !importPipeline_);
        if (ImGui::Button("导入"))
        {
            const auto destDir = selectedDirectory_.empty() ? contentRoot_ : selectedDirectory_;
            glz::raw_json rawJson{ pending.settingsJson };
            auto result = importPipeline_->ExecuteImport(
                *pending.importer,
                pending.sourcePath,
                destDir,
                contentRoot_,
                std::move(rawJson),
                editorAssetRegistry_);

            if (result.succeeded)
            {
                pendingImport_.reset();
                importErrorMsg_.clear();
                ImGui::CloseCurrentPopup();
            }
            else
            {
                importErrorMsg_ = result.errorMessage;
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("取消"))
        {
            pendingImport_.reset();
            importErrorMsg_.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    void AssetsBrower::OpenEntry(const std::filesystem::directory_entry& entry)
    {
        std::error_code ec;
        if (entry.is_directory(ec))
        {
            selectedDirectory_ = entry.path();
            return;
        }

        // Look up asset record via the editor registry
        if (editorAssetRegistry_)
        {
            const auto* assetEntry = editorAssetRegistry_->FindByPath(entry.path());
            if (assetEntry)
            {
                // Dispatch based on asset type
                const auto& type = assetEntry->record.type;
                if (type == "world")
                {
                    // TODO: Load world map via WorldService
                }
                else if (type == "material")
                {
                    // TODO: Open material editor
                }
                // Other asset types can be handled here
            }
        }
    }

    bool AssetsBrower::RenameEntry(const std::filesystem::path& sourcePath, const shine::SString& newName)
    {
        if (sourcePath.empty() || newName.empty())
            return false;
        const auto targetPath = sourcePath.parent_path() / std::filesystem::path(newName.to_string());
        if (targetPath == sourcePath)
            return false;
        std::error_code ec;
        std::filesystem::rename(sourcePath, targetPath, ec);
        if (ec)
            return false;
        SyncAssetRecordMove(sourcePath, targetPath);
        if (contextEntryPath_ == sourcePath)
            contextEntryPath_ = targetPath;
        return true;
    }

    bool AssetsBrower::DeleteEntry(const std::filesystem::path& path)
    {
        if (path.empty())
            return false;
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec))
            std::filesystem::remove_all(path, ec);
        else
            std::filesystem::remove(path, ec);
        if (ec)
            return false;
        SyncAssetRecordDelete(path);
        if (contextEntryPath_ == path)
            contextEntryPath_.clear();
        return true;
    }

    bool AssetsBrower::MoveEntry(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationDirectory)
    {
        if (sourcePath.empty() || destinationDirectory.empty())
            return false;
        std::error_code ec;
        std::filesystem::create_directories(destinationDirectory, ec);
        if (ec)
            return false;
        const auto targetPath = destinationDirectory / sourcePath.filename();
        std::filesystem::rename(sourcePath, targetPath, ec);
        if (ec)
            return false;
        SyncAssetRecordMove(sourcePath, targetPath);
        if (contextEntryPath_ == sourcePath)
            contextEntryPath_ = targetPath;
        return true;
    }

    void AssetsBrower::SyncAssetRecordMove(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
    {
        if (editorAssetRegistry_)
        {
            editorAssetRegistry_->OnFileMoved(oldPath, newPath);
        }
    }

    void AssetsBrower::SyncAssetRecordDelete(const std::filesystem::path& path)
    {
        if (editorAssetRegistry_)
        {
            editorAssetRegistry_->OnFileDeleted(path);
        }
    }

    // -----------------------------------------------------------------------
    //  Directory context menu (目录树右键)
    // -----------------------------------------------------------------------
    void AssetsBrower::RenderDirectoryContextMenu(const std::filesystem::path& path)
    {
        if (!ImGui::BeginPopupContextItem())
            return;

        contextDirPath_ = path;

        if (ImGui::MenuItem("新建文件夹"))
        {
            std::memset(newFolderBuffer_, 0, sizeof(newFolderBuffer_));
            requestNewFolderPopup_ = true;
        }

        // 根目录不允许重命名/删除
        std::error_code ec;
        const bool isRoot = !contentRoot_.empty() &&
                            std::filesystem::equivalent(path, contentRoot_, ec) && !ec;
        if (!isRoot)
        {
            if (ImGui::MenuItem("重命名"))
            {
                const auto name = path.filename().string();
                std::memset(renameDirBuffer_, 0, sizeof(renameDirBuffer_));
                std::strncpy(renameDirBuffer_, name.c_str(), sizeof(renameDirBuffer_) - 1);
                requestRenameDirPopup_ = true;
            }
            if (ImGui::MenuItem("删除文件夹"))
            {
                requestDeleteDirPopup_ = true;
            }
        }

        ImGui::EndPopup();
    }

    // -----------------------------------------------------------------------
    //  Directory operations
    // -----------------------------------------------------------------------
    bool AssetsBrower::CreateFolder(const std::filesystem::path& parentDir, const shine::SString& name)
    {
        if (parentDir.empty() || name.empty())
            return false;
        const auto newDir = parentDir / std::filesystem::path(name.to_string());
        std::error_code ec;
        std::filesystem::create_directory(newDir, ec);
        return !ec;
    }

    bool AssetsBrower::RenameDirectory(const std::filesystem::path& oldPath, const shine::SString& newName)
    {
        if (oldPath.empty() || newName.empty())
            return false;
        const auto newPath = oldPath.parent_path() / std::filesystem::path(newName.to_string());
        if (newPath == oldPath)
            return false;

        // 重命名前先收集内部所有 .sasset 的旧路径
        // 转移文件系统
        std::error_code ec;
        std::filesystem::rename(oldPath, newPath, ec);
        if (ec)
            return false;

        // 同步注册表：用新路径反推旧路径批量更新
        SyncAssetRecordMoveDir(oldPath, newPath);

        // 如果当前选中目录在被重命名的目录下，同步更新
        // 注意：此时 oldPath 已不存在，不能用 equivalent()（需要双路径存在），改为词法比较
        if (!selectedDirectory_.empty())
        {
            if (selectedDirectory_ == oldPath)
            {
                selectedDirectory_ = newPath;
            }
            else
            {
                std::error_code relEc;
                const auto rel = std::filesystem::relative(selectedDirectory_, oldPath, relEc);
                if (!relEc && !rel.empty() && rel.native()[0] != '.')
                    selectedDirectory_ = newPath / rel;
            }
        }
        return true;
    }

    bool AssetsBrower::DeleteDirectory(const std::filesystem::path& path)
    {
        if (path.empty())
            return false;

        // 先通知注册表内部所有 .sasset 被删除
        if (editorAssetRegistry_)
        {
            std::error_code scanEc;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path, scanEc))
            {
                if (entry.path().extension() == ".sasset")
                    editorAssetRegistry_->OnFileDeleted(entry.path());
            }
        }

        std::error_code ec;
        std::filesystem::remove_all(path, ec);
        if (ec)
            return false;

        // 如果选中目录在已删除目录下，退回根目录
        // 注意：此时 path 已不存在，不能用 equivalent()（需要双路径存在），改为词法比较
        if (!selectedDirectory_.empty())
        {
            const bool wasExact = (selectedDirectory_ == path);
            std::error_code relEc;
            const auto rel = std::filesystem::relative(selectedDirectory_, path, relEc);
            const bool wasInside = !relEc && !rel.empty() && rel.native()[0] != '.';
            if (wasExact || wasInside)
                selectedDirectory_ = contentRoot_;
        }
        return true;
    }

    // 文件夹整体移动/重命名后，用新路径反推旧路径，批量调用 OnFileMoved
    void AssetsBrower::SyncAssetRecordMoveDir(const std::filesystem::path& oldDir,
                                               const std::filesystem::path& newDir)
    {
        if (!editorAssetRegistry_)
            return;
        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(newDir, ec))
        {
            if (entry.path().extension() != ".sasset")
                continue;
            std::error_code relEc;
            const auto rel = std::filesystem::relative(entry.path(), newDir, relEc);
            if (!relEc)
                editorAssetRegistry_->OnFileMoved(oldDir / rel, entry.path());
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
        std::ranges::transform(filter, filter.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (filter.empty())
        {
            return true;
        }
        auto lower = ToDisplayText(path.filename()).to_string();
        std::ranges::transform(lower, lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return lower.find(filter) != std::string::npos;
    }

}    // end namespace

