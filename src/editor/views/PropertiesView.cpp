#include "PropertiesView.h"

#include <cfloat>
#include <cstdint>
#include <string>



#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"


#include "fmt/format.h"


#include "script/ScriptSystem.h"

#include "EngineCore/engine_context.h"
#include "gameplay/component/ScriptComponent.h"

namespace shine::editor::views
{

    void PropertiesView::onShutDown() {
    }
    void PropertiesView::onInit() {
        SetName("属性编辑器");
    }
    void PropertiesView::onRender()
    {
        if(ImGui::Begin(name.c_str(), &isOpen))
        {
            auto* selectedObject = worldHierarchyService_ ? worldHierarchyService_->getSelectedObject() : nullptr;
            if (selectedObject == nullptr)
            {
                ImGui::Text("未选择任何对象");
            }
            else
            {
                RenderObjectProperties(selectedObject);
            }
        }

        ImGui::End();
    }

    void PropertiesView::SetWorldHierarchyService(shine::gameplay::world::IWorldActorHierarchyService* worldHierarchyService)
    {
        worldHierarchyService_ = worldHierarchyService;
    }

    void PropertiesView::SetScriptSystem(shine::script::ScriptSystem* scriptSystem)
    {
        scriptSystem_ = scriptSystem;
    }

    void PropertiesView::RenderObjectProperties(shine::gameplay::SObject* obj)
    {
        if (obj == nullptr)
            return;

        if (ImGui::InputText("名称", obj->getRefName().data(), obj->getRefName().size())) {
            obj->setName(obj->getRefName());
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
                for(auto& comp :components){

                    std::string compName = typeid(*comp).name();
                    // Clean up "class " prefix if present (MSVC)
                    if (compName.starts_with("class ")) compName = compName.substr(6);
                    if (compName.starts_with("struct ")) compName = compName.substr(7);

                    if (ImGui::TreeNode((void*)comp.get(), "%s", compName.c_str())) {

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

            const bool isReadOnly = property.access == "ReadOnly";
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
            if (property.type == "bool")
            {
                bool boolValue = std::holds_alternative<bool>(value.data) ? std::get<bool>(value.data) : false;
                changed = ImGui::Checkbox(valueLabel.c_str(), &boolValue);
                if (changed)
                {
                    newValue = reflection::ScriptValue(boolValue);
                }
            }
            else if (property.type == "int")
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
            else if (property.type == "float")
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
            else if (property.type == "array")
            {
                // 数组类型：可展开编辑
                if (std::holds_alternative<reflection::ScriptValue::ArrayWrapper>(value.data))
                {
                    auto arrWrapper = std::get<reflection::ScriptValue::ArrayWrapper>(value.data);
                    auto& arr = arrWrapper.ptr;
                    if (!arr) arr = std::make_shared<reflection::ScriptArray>();

                    const std::string treeNodeLabel = fmt::format("[{}] {}", arr->elements.size(), nameText);
                    ImGui::PushID(nameText.c_str());
                    if (ImGui::TreeNode(treeNodeLabel.c_str()))
                    {
                        // 添加/删除按钮
                        if (!isReadOnly)
                        {
                            if (ImGui::Button("+ Add"))
                            {
                                // Initialize with a default value based on the first element if it exists, otherwise default to string
                                if (!arr->elements.empty())
                                {
                                    arr->elements.push_back(arr->elements.back());
                                }
                                else
                                {
                                    arr->elements.emplace_back(std::string(""));
                                }
                                changed = true;
                            }
                            ImGui::SameLine();
                            if (arr->elements.size() > 0 && ImGui::Button("- Remove Last"))
                            {
                                arr->elements.pop_back();
                                changed = true;
                            }
                        }

                        // 编辑每个元素
                        for (size_t i = 0; i < arr->elements.size(); ++i)
                        {
                            const std::string elemLabel = fmt::format("[{}]##{}_{}", i, nameText, i);
                            auto& elem = arr->elements[i];

                            if (std::holds_alternative<std::string>(elem.data))
                            {
                                std::string text = std::get<std::string>(elem.data);
                                if (ImGui::InputText(elemLabel.c_str(), &text, isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0))
                                {
                                    elem = reflection::ScriptValue(text);
                                    changed = true;
                                }
                            }
                            else if (std::holds_alternative<int>(elem.data))
                            {
                                int intVal = std::get<int>(elem.data);
                                if (ImGui::InputInt(elemLabel.c_str(), &intVal, 1, 10, isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0))
                                {
                                    elem = reflection::ScriptValue(intVal);
                                    changed = true;
                                }
                            }
                            else if (std::holds_alternative<float>(elem.data))
                            {
                                float floatVal = std::get<float>(elem.data);
                                if (ImGui::DragFloat(elemLabel.c_str(), &floatVal, 0.01f, 0, 0, "%.3f", isReadOnly ? ImGuiSliderFlags_NoInput : 0))
                                {
                                    elem = reflection::ScriptValue(floatVal);
                                    changed = true;
                                }
                            }
                            else if (std::holds_alternative<bool>(elem.data))
                            {
                                bool boolVal = std::get<bool>(elem.data);
                                if (ImGui::Checkbox(elemLabel.c_str(), &boolVal))
                                {
                                    elem = reflection::ScriptValue(boolVal);
                                    changed = true;
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled("%s: (unsupported type)", elemLabel.c_str());
                            }
                        }

                        if (changed)
                        {
                            newValue = reflection::ScriptValue(arrWrapper);
                        }
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                    // 未展开时显示大小
                    ImGui::SameLine();
                    ImGui::TextDisabled("[%zu]", arr->elements.size());
                }
                else
                {
                    ImGui::TextDisabled("[]");
                }
            }
            else if (property.type == "map")
            {
                // Map类型：可展开编辑
                if (std::holds_alternative<reflection::ScriptValue::MapWrapper>(value.data))
                {
                    auto mapWrapper = std::get<reflection::ScriptValue::MapWrapper>(value.data);
                    auto& map = mapWrapper.ptr;
                    if (!map) map = std::make_shared<reflection::ScriptMap>();

                    const std::string treeNodeLabel = fmt::format("{{{}}} {}", map->elements.size(), nameText);
                    ImGui::PushID(nameText.c_str());
                    if (ImGui::TreeNode(treeNodeLabel.c_str()))
                    {
                        // 添加新键值对
                        static std::string newKeyBuffer{};
                        if (!isReadOnly)
                        {
                            ImGui::PushID("new_key_input");
                            ImGui::SetNextItemWidth(100);
                            ImGui::InputText("##newKey", &newKeyBuffer);
                            ImGui::SameLine();
                            if (ImGui::Button("+ Add Key"))
                            {
                                if (newKeyBuffer[0] != '\0')
                                {
                                    if (map->elements.find(newKeyBuffer) == map->elements.end())
                                    {
                                        map->elements[newKeyBuffer] = reflection::ScriptValue(std::string(""));
                                        changed = true;
                                        newKeyBuffer.clear();
                                    }
                                }
                            }
                            ImGui::PopID();
                        }

                        // 编辑每个键值对
                        std::vector<std::string> keysToRemove;
                        for (auto& [key, val] : map->elements)
                        {
                            ImGui::PushID(key.c_str());

                            // 键名 + 删除按钮
                            ImGui::Text("%s", key.c_str());
                            if (!isReadOnly)
                            {
                                ImGui::SameLine();
                                if (ImGui::SmallButton("X"))
                                {
                                    keysToRemove.push_back(key);
                                }
                            }
                            ImGui::SameLine();

                            // 值编辑
                            const std::string valLabel = fmt::format("##val_{}", key);
                            if (std::holds_alternative<std::string>(val.data))
                            {
                                char buffer[256];
                                strncpy_s(buffer, std::get<std::string>(val.data).c_str(), sizeof(buffer) - 1);
                                buffer[sizeof(buffer) - 1] = '\0';
                                if (ImGui::InputText(valLabel.c_str(), buffer, sizeof(buffer), isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0))
                                {
                                    val = reflection::ScriptValue(std::string(buffer));
                                    changed = true;
                                }
                            }
                            else if (std::holds_alternative<int>(val.data))
                            {
                                int intVal = std::get<int>(val.data);
                                if (ImGui::InputInt(valLabel.c_str(), &intVal, 1, 10, isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0))
                                {
                                    val = reflection::ScriptValue(intVal);
                                    changed = true;
                                }

                            }
                            else if (std::holds_alternative<float>(val.data))
                            {
                                float floatVal = std::get<float>(val.data);
                                if (ImGui::DragFloat(valLabel.c_str(), &floatVal, 0.01f, 0, 0, "%.3f", isReadOnly ? ImGuiSliderFlags_NoInput : 0))
                                {
                                    val = reflection::ScriptValue(floatVal);
                                    changed = true;
                                }
                            }
                            else if (std::holds_alternative<bool>(val.data))
                            {
                                bool boolVal = std::get<bool>(val.data);
                                if (ImGui::Checkbox(valLabel.c_str(), &boolVal))
                                {
                                    val = reflection::ScriptValue(boolVal);
                                    changed = true;
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled("(unsupported type)");
                            }

                            ImGui::PopID();
                        }

                        // 删除标记的键
                        for (const auto& keyToRemove : keysToRemove)
                        {
                            map->elements.erase(keyToRemove);
                            changed = true;
                        }

                        if (changed)
                        {
                            newValue = reflection::ScriptValue(mapWrapper);
                        }
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                    // 未展开时显示大小
                    ImGui::SameLine();
                    ImGui::TextDisabled("{%zu}", map->elements.size());
                }
                else
                {
                    ImGui::TextDisabled("{}");
                }
            }
            else
            {
                static std::string text{};
                if (std::holds_alternative<std::string>(value.data))
                {
                    text = std::get<std::string>(value.data);
                }
                changed = ImGui::InputText(valueLabel.c_str(), text.data(), text.length());
                if (changed)
                {
                    newValue = reflection::ScriptValue(text);
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
