#include "PropertiesView.h"

#include <cfloat>
#include <cstdint>
#include <string>



#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"


#include "script/ScriptSystem.h"

#include "EngineCore/engine_context.h"
#include "editor/util/ImGuiIdScope.h"
#include "gameplay/component/ScriptComponent.h"

namespace shine::editor::views
{
    namespace {
        constexpr const char* kScriptValueLabel = "##ScriptValue";
        constexpr const char* kArrayElementLabel = "##ArrayElement";
        constexpr const char* kMapValueLabel = "##MapValue";
        constexpr const char* kGroupTableId = "ScriptGroupTable";
    }


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
            shine::SString nameText;
            shine::SString group;
        };
        struct ScriptLayoutCache
        {
            shine::gameplay::SObject* object = nullptr;
            uint32_t handleId = 0;
            uint64_t layoutVersion = 0;
            bool valid = false;
            std::vector<CachedScriptProperty> properties;
            std::vector<shine::SString> groupOrder;
            std::unordered_map<shine::SString,
                               std::vector<size_t>,
                               shine::SStringTransparentHash,
                               shine::SStringTransparentEqual> groupedIndices;
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
                    .nameText = property.name,
                    .group = property.group.empty() ? shine::SString("默认分组") : property.group
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
            const char* const propertyNameText = property.nameText.c_str();
            bool changed = false;
            reflection::ScriptValue newValue;
            const auto isStringValue = [](const reflection::ScriptValue& scriptValue)
            {
                return std::holds_alternative<shine::SString>(scriptValue.data)
                    || std::holds_alternative<shine::STextView>(scriptValue.data);
            };
            const auto toEditableString = [](const reflection::ScriptValue& scriptValue) -> shine::SString
            {
                if (std::holds_alternative<shine::SString>(scriptValue.data))
                {
                    return std::get<shine::SString>(scriptValue.data);
                }
                if (std::holds_alternative<shine::STextView>(scriptValue.data))
                {
                    return shine::SString(std::get<shine::STextView>(scriptValue.data));
                }
                return {};
            };

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(propertyNameText);

            ImGui::TableSetColumnIndex(1);
            const shine::editor::util::ScopedImGuiID valueId(static_cast<int>(index));
            if (isReadOnly)
            {
                ImGui::BeginDisabled();
            }
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (property.type == "bool")
            {
                bool boolValue = std::holds_alternative<bool>(value.data) ? std::get<bool>(value.data) : false;
                changed = ImGui::Checkbox(kScriptValueLabel, &boolValue);
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
                changed = ImGui::InputInt(kScriptValueLabel, &intValue);
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
                changed = ImGui::DragFloat(kScriptValueLabel, &floatValue, 0.01f);
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

                    if (ImGui::TreeNodeEx("ScriptArray", ImGuiTreeNodeFlags_None, "[%zu] %s", arr->elements.size(), propertyNameText))
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
                                    arr->elements.emplace_back(shine::SString(""));
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
                            const shine::editor::util::ScopedImGuiID elementId(static_cast<int>(i));
                            auto& elem = arr->elements[i];

                            if (isStringValue(elem))
                            {
                                shine::SString text = toEditableString(elem);
                                if (ImGui::InputText(kArrayElementLabel, &text, isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0))
                                {
                                    elem = reflection::ScriptValue(text);
                                    changed = true;
                                }
                            }
                            else if (std::holds_alternative<int>(elem.data))
                            {
                                int intVal = std::get<int>(elem.data);
                                if (ImGui::InputInt(kArrayElementLabel, &intVal, 1, 10, isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0))
                                {
                                    elem = reflection::ScriptValue(intVal);
                                    changed = true;
                                }
                            }
                            else if (std::holds_alternative<float>(elem.data))
                            {
                                float floatVal = std::get<float>(elem.data);
                                if (ImGui::DragFloat(kArrayElementLabel, &floatVal, 0.01f, 0, 0, "%.3f", isReadOnly ? ImGuiSliderFlags_NoInput : 0))
                                {
                                    elem = reflection::ScriptValue(floatVal);
                                    changed = true;
                                }
                            }
                            else if (std::holds_alternative<bool>(elem.data))
                            {
                                bool boolVal = std::get<bool>(elem.data);
                                if (ImGui::Checkbox(kArrayElementLabel, &boolVal))
                                {
                                    elem = reflection::ScriptValue(boolVal);
                                    changed = true;
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled("[%zu]: (unsupported type)", i);
                            }
                        }

                        if (changed)
                        {
                            newValue = reflection::ScriptValue(arrWrapper);
                        }
                        ImGui::TreePop();
                    }
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

                    if (ImGui::TreeNodeEx("ScriptMap", ImGuiTreeNodeFlags_None, "{%zu} %s", map->elements.size(), propertyNameText))
                    {
                        // 添加新键值对
                        static shine::SString newKeyBuffer{};
                        if (!isReadOnly)
                        {
                            ImGui::PushID("new_key_input");
                            ImGui::SetNextItemWidth(100);
                            ImGui::InputText("##newKey", &newKeyBuffer);
                            ImGui::SameLine();
                            if (ImGui::Button("+ Add Key"))
                            {
                                if (!newKeyBuffer.empty())
                                {
                                    if (map->elements.find(newKeyBuffer) == map->elements.end())
                                    {
                                        map->elements[newKeyBuffer] = reflection::ScriptValue(shine::SString(""));
                                        changed = true;
                                        newKeyBuffer.clear();
                                    }
                                }
                            }
                            ImGui::PopID();
                        }

                        // 编辑每个键值对
                        std::vector<shine::SString> keysToRemove;
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
                            if (isStringValue(val))
                            {
                                shine::SString text = toEditableString(val);
                                if (ImGui::InputText(kMapValueLabel, &text, isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0))
                                {
                                    val = reflection::ScriptValue(text);
                                    changed = true;
                                }
                            }
                            else if (std::holds_alternative<int>(val.data))
                            {
                                int intVal = std::get<int>(val.data);
                                if (ImGui::InputInt(kMapValueLabel, &intVal, 1, 10, isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0))
                                {
                                    val = reflection::ScriptValue(intVal);
                                    changed = true;
                                }

                            }
                            else if (std::holds_alternative<float>(val.data))
                            {
                                float floatVal = std::get<float>(val.data);
                                if (ImGui::DragFloat(kMapValueLabel, &floatVal, 0.01f, 0, 0, "%.3f", isReadOnly ? ImGuiSliderFlags_NoInput : 0))
                                {
                                    val = reflection::ScriptValue(floatVal);
                                    changed = true;
                                }
                            }
                            else if (std::holds_alternative<bool>(val.data))
                            {
                                bool boolVal = std::get<bool>(val.data);
                                if (ImGui::Checkbox(kMapValueLabel, &boolVal))
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
                shine::SString text = toEditableString(value);
                changed = ImGui::InputText(kScriptValueLabel, &text, isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0);
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
            const shine::editor::util::ScopedImGuiID groupId(static_cast<int>(groupIndex));
            const bool opened = ImGui::CollapsingHeader(
                groupName.c_str(),
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
            if (!ImGui::BeginTable(
                kGroupTableId,
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
