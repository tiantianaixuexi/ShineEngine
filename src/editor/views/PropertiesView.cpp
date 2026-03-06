#include "PropertiesView.h"
#include "EngineCore/engine_context.h"
#include "imgui/imgui.h"
#include "fmt/format.h"
#include "EngineCore/reflection/Reflection.h"
#include "gameplay/component/ScriptComponent.h"
#include "script/ScriptSystem.h"
#include "../util/InspectorBuilder.h"
#include <cfloat>
#include <cstdint>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <variant>
#include <vector>

namespace shine::editor::views
{

    void PropertiesView::onShutDown() {
        selectedObject_ = nullptr;
    }
    void PropertiesView::onInit() {
        SetName("属性编辑器");
    }
    void PropertiesView::onRender()
    {
        if(ImGui::Begin(name.c_str(), &isOpen))
        {

            if (selectedObject_ == nullptr)
            {
                ImGui::Text("未选择任何对象");
            }
            else
            {
                RenderObjectProperties(selectedObject_);
            }
        }
                    
        ImGui::End();
    }

    void PropertiesView::SetSelectedObject(shine::gameplay::SObject* obj)
    {
        selectedObject_ = obj;
    }

    void PropertiesView::SetScriptSystem(shine::script::ScriptSystem* scriptSystem)
    {
        scriptSystem_ = scriptSystem;
    }

    void PropertiesView::RenderObjectProperties(shine::gameplay::SObject* obj)
    {
        if (obj == nullptr)
            return;

        // Basic Info (Name/Active/Visible)
        // Ideally, SObject itself should be reflected so we can just call DrawInspector(obj).
        // For now, keep this manual part or assume SObject fields are not reflected yet.
        
        char nameBuffer[256];
        strncpy_s(nameBuffer, obj->getName().c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        if (ImGui::InputText("名称", nameBuffer, sizeof(nameBuffer))) {
            obj->setName(nameBuffer);
        }

        bool active = obj->isActive();
        if (ImGui::Checkbox("激活", &active)) {
            obj->setActive(active);
        }

        bool visible = obj->isVisible();
        if (ImGui::Checkbox("可见", &visible)) {
            obj->setVisible(visible);
        }

        ImGui::Separator();

        RenderComponentProperties(obj);
        ImGui::Separator();
        RenderScriptProperties(obj);
    }

    void PropertiesView::RenderComponentProperties(shine::gameplay::SObject* obj)
    {
        if (ImGui::CollapsingHeader("组件", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto& components = obj->getComponents();
            if (components.empty())
            {
                ImGui::Text("无组件");
            }
            else
            {
                for (size_t i = 0; i < components.size(); ++i)
                {
                    auto* comp = components[i].get();
                    if (!comp) continue;

                    // Use RTTI or Reflection to get the component name
                    // Assuming we have TypeId for components via Reflection
                    // If Component is Reflected, we can find its TypeInfo.
                    
                    // Fallback to RTTI name if no TypeInfo found (though InspectorBuilder requires TypeInfo)
                    // Let's try to find TypeInfo using a helper (requires Component to have GetTypeId or similar)
                    // For now, use typeid name for header, and assume we can't draw fields unless reflected.
                    
                    std::string compName = typeid(*comp).name(); 
                    // Clean up "class " prefix if present (MSVC)
                    if (compName.starts_with("class ")) compName = compName.substr(6);
                    if (compName.starts_with("struct ")) compName = compName.substr(7);

                    if (ImGui::TreeNode((void*)comp, "%s", compName.c_str())) {
                        
                        // Try to find TypeInfo via TypeRegistry
                        // This relies on component classes having registered reflection with matching names or IDs
                        // Since we don't have the compile-time type T here, we need runtime lookup.
                        // Assuming we can get TypeId from instance or name.
                        // If components don't have virtual GetTypeId(), we are stuck unless we use RTTI hash map.
                        
                        // Workaround: We can't easily get TypeInfo from base pointer without virtual GetTypeId().
                        // Let's assume for this task that we just print a placeholder or try to use a hypothetical lookup.
                        // ImGui::Text("Inspector logic here...");
                        
                        // FUTURE: util::InspectorBuilder::DrawInspector(comp, typeInfo);
                        
                        ImGui::TreePop();
                    }
                }
            }
        }
    }

    void PropertiesView::RenderScriptProperties(shine::gameplay::SObject* obj)
    {
        if (!ImGui::CollapsingHeader("脚本变量", ImGuiTreeNodeFlags_DefaultOpen))
        {
            return;
        }

        auto* scriptComponent = obj->getComponent<shine::gameplay::component::ScriptComponent>();
        if (!scriptComponent)
        {
            ImGui::Text("无脚本组件");
            return;
        }
        if (!scriptSystem_)
        {
            scriptSystem_ = EngineContext::Get().GetSystem<shine::script::ScriptSystem>();
        }
        if (!scriptSystem_)
        {
            ImGui::Text("ScriptSystem 未就绪");
            return;
        }

        const auto handle = scriptComponent->getScriptHandle();
        if (!handle.IsValid())
        {
            ImGui::Text("脚本未加载");
            return;
        }

        struct CachedScriptProperty
        {
            shine::SString name;
            shine::SString type;
            shine::SString access;
            std::string group;
        };
        struct ScriptLayoutCache
        {
            shine::gameplay::SObject* object = nullptr;
            uint32_t handleId = 0;
            uint64_t layoutVersion = 0;
            bool valid = false;
            std::vector<CachedScriptProperty> properties;
            std::vector<std::string> groupOrder;
            std::unordered_map<std::string, std::vector<size_t>> groupedIndices;
        };
        static ScriptLayoutCache cache{};

        const uint64_t currentLayoutVersion = scriptSystem_->GetScriptPropertyLayoutVersion(handle);
        const bool cacheDirty = !cache.valid ||
            cache.object != obj ||
            cache.handleId != handle.id ||
            cache.layoutVersion != currentLayoutVersion;
        if (cacheDirty)
        {
            std::vector<shine::script::ScriptSystem::ScriptPropertyInspectorInfo> inspectorProperties;
            if (!scriptSystem_->GetScriptPropertyInfos(handle, inspectorProperties) || inspectorProperties.empty())
            {
                ImGui::Text("无可编辑脚本变量");
                return;
            }

            cache.object = obj;
            cache.handleId = handle.id;
            cache.layoutVersion = currentLayoutVersion;
            cache.valid = true;
            cache.properties.clear();
            cache.groupOrder.clear();
            cache.groupedIndices.clear();
            cache.properties.reserve(inspectorProperties.size());
            cache.groupOrder.reserve(inspectorProperties.size());
            cache.groupedIndices.reserve(inspectorProperties.size());

            size_t visiblePropertyCount = 0;
            for (size_t i = 0; i < inspectorProperties.size(); ++i)
            {
                const auto& property = inspectorProperties[i];
                if (!property.visible)
                {
                    continue;
                }

                cache.properties.push_back(CachedScriptProperty{
                    .name = property.name,
                    .type = property.type,
                    .access = property.access,
                    .group = property.group.empty() ? "默认分组" : property.group.to_string()
                });

                const size_t cachedIndex = cache.properties.size() - 1;
                auto [it, inserted] = cache.groupedIndices.try_emplace(cache.properties.back().group, std::vector<size_t>{});
                if (inserted)
                {
                    cache.groupOrder.push_back(cache.properties.back().group);
                }
                it->second.push_back(cachedIndex);
                ++visiblePropertyCount;
            }

            if (visiblePropertyCount == 0)
            {
                ImGui::Text("无可显示脚本变量");
                return;
            }
        }

        auto renderPropertyField = [&](size_t index)
        {
            const auto& property = cache.properties[index];
            reflection::ScriptValue value;
            if (!scriptSystem_->GetScriptPropertyValue(handle, property.name.view(), value))
            {
                return;
            }

            const bool isReadOnly = property.access.sv() == "ReadOnly";
            const std::string nameText = property.name.to_string();
            const std::string valueLabel = fmt::format("##ScriptProp_{}", index);
            bool changed = false;
            reflection::ScriptValue newValue;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(nameText.c_str());

            ImGui::TableSetColumnIndex(1);
            if (isReadOnly)
            {
                ImGui::BeginDisabled();
            }
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (property.type.sv() == "bool")
            {
                bool boolValue = std::holds_alternative<bool>(value.data) ? std::get<bool>(value.data) : false;
                changed = ImGui::Checkbox(valueLabel.c_str(), &boolValue);
                if (changed)
                {
                    newValue = reflection::ScriptValue(boolValue);
                }
            }
            else if (property.type.sv() == "int")
            {
                int intValue = 0;
                if (std::holds_alternative<int>(value.data))
                {
                    intValue = std::get<int>(value.data);
                }
                else if (std::holds_alternative<float>(value.data))
                {
                    intValue = static_cast<int>(std::get<float>(value.data));
                }
                else if (std::holds_alternative<double>(value.data))
                {
                    intValue = static_cast<int>(std::get<double>(value.data));
                }
                changed = ImGui::InputInt(valueLabel.c_str(), &intValue);
                if (changed)
                {
                    newValue = reflection::ScriptValue(intValue);
                }
            }
            else if (property.type.sv() == "float")
            {
                float floatValue = 0.0f;
                if (std::holds_alternative<int>(value.data))
                {
                    floatValue = static_cast<float>(std::get<int>(value.data));
                }
                else if (std::holds_alternative<float>(value.data))
                {
                    floatValue = std::get<float>(value.data);
                }
                else if (std::holds_alternative<double>(value.data))
                {
                    floatValue = static_cast<float>(std::get<double>(value.data));
                }
                changed = ImGui::DragFloat(valueLabel.c_str(), &floatValue, 0.01f);
                if (changed)
                {
                    newValue = reflection::ScriptValue(floatValue);
                }
            }
            else
            {
                char textBuffer[256]{};
                if (std::holds_alternative<std::string>(value.data))
                {
                    const auto& text = std::get<std::string>(value.data);
                    strncpy_s(textBuffer, text.c_str(), sizeof(textBuffer) - 1);
                    textBuffer[sizeof(textBuffer) - 1] = '\0';
                }
                changed = ImGui::InputText(valueLabel.c_str(), textBuffer, sizeof(textBuffer));
                if (changed)
                {
                    newValue = reflection::ScriptValue(std::string(textBuffer));
                }
            }

            if (isReadOnly)
            {
                ImGui::EndDisabled();
            }

            if (changed && !isReadOnly)
            {
                scriptSystem_->SetScriptPropertyValue(handle, property.name.view(), newValue);
            }
        };

        for (size_t groupIndex = 0; groupIndex < cache.groupOrder.size(); ++groupIndex)
        {
            const auto& groupName = cache.groupOrder[groupIndex];
            const std::string groupLabel = fmt::format("{}##ScriptGroup_{}", groupName, groupIndex);
            const bool opened = ImGui::CollapsingHeader(
                groupLabel.c_str(),
                ImGuiTreeNodeFlags_DefaultOpen
            );
            if (!opened)
            {
                continue;
            }

            const auto it = cache.groupedIndices.find(groupName);
            if (it == cache.groupedIndices.end())
            {
                continue;
            }
            const std::string tableId = fmt::format("ScriptGroupTable_{}", groupIndex);
            if (!ImGui::BeginTable(
                tableId.c_str(),
                2,
                ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp
            ))
            {
                continue;
            }
            ImGui::TableSetupColumn("变量", ImGuiTableColumnFlags_WidthStretch, 0.38f);
            ImGui::TableSetupColumn("值", ImGuiTableColumnFlags_WidthStretch, 0.62f);
            for (size_t index : it->second)
            {
                renderPropertyField(index);
            }
            ImGui::EndTable();
        }
    }
}
