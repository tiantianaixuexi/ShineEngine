#include "AssetDependencyView.h"

#include "imgui/imgui.h"
#include "editor/ShineAsset/EditorAssetRegistry.h"

namespace shine::editor::views
{
    void AssetDependencyView::onInit()
    {
        SetName("资产依赖");
    }

    void AssetDependencyView::onRender()
    {
        if (ImGui::Begin(name.c_str(), &isOpen))
        {
            if (!registry_ || selectedUuid_.empty())
            {
                ImGui::TextUnformatted("未选择资产");
                ImGui::End();
                CheckIsOpenChange();
                return;
            }

            const auto* entry = registry_->Find(selectedUuid_);
            if (!entry)
            {
                ImGui::TextUnformatted("未找到资产记录");
                ImGui::End();
                CheckIsOpenChange();
                return;
            }

            // Header
            ImGui::Text("UUID: %s", entry->uuid.c_str());
            ImGui::Text("类型: %s", entry->record.type.c_str());
            ImGui::Text("路径: %s", entry->diskPath.c_str());
            if (entry->isDangling)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "⚠ 文件已缺失 (悬挂引用)");
            }
            ImGui::Separator();

            // Forward dependencies (what this asset depends on)
            const auto& deps = registry_->DependencyGraph().GetDependencies(selectedUuid_);
            if (ImGui::TreeNodeEx("依赖项 (本资产依赖的)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (deps.empty())
                {
                    ImGui::TextDisabled("无依赖");
                }
                else
                {
                    for (const auto& depUuid : deps)
                    {
                        const auto* depEntry = registry_->Find(depUuid);
                        bool isDangling = !depEntry || (depEntry && depEntry->isDangling);
                        ImVec4 color = isDangling
                            ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
                            : ImVec4(0.8f, 0.8f, 0.8f, 1.0f);

                        ImGui::TextColored(color, "%s", depUuid.c_str());
                        if (depEntry)
                        {
                            ImGui::SameLine();
                            ImGui::TextDisabled("(%s)", depEntry->record.type.c_str());
                        }
                        if (isDangling)
                        {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[缺失]");
                        }
                    }
                }
                ImGui::TreePop();
            }

            // Reverse dependencies (what depends on this asset)
            const auto& dependents = registry_->GetDependents(selectedUuid_);
            if (ImGui::TreeNodeEx("被依赖项 (依赖本资产的)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (dependents.empty())
                {
                    ImGui::TextDisabled("无被依赖项");
                }
                else
                {
                    for (const auto& depUuid : dependents)
                    {
                        const auto* depEntry = registry_->Find(depUuid);
                        ImGui::Text("%s", depUuid.c_str());
                        if (depEntry)
                        {
                            ImGui::SameLine();
                            ImGui::TextDisabled("(%s)", depEntry->record.type.c_str());
                        }
                    }
                }
                ImGui::TreePop();
            }
        }

        ImGui::End();
        CheckIsOpenChange();
    }

    void AssetDependencyView::onShutDown()
    {
    }

} // namespace shine::editor::views
