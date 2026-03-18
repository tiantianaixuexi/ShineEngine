#include "PropertyDrawer.h"
#include "InspectorBuilder.h"
#include "imgui/imgui.h"
#include "string/shine_string.h"
#include <variant>
#include <algorithm>

namespace shine::editor::util {

    using namespace reflection;

    // Internal Visitor
    struct FieldRendererVisitor {
        void* instance;
        const reflection::FieldInfo& field;
        const reflection::TypeInfo* ownerType;

        // 1. None / Auto-Deduce — automatically select UI control based on C++ type
        //    Like UE5 UPROPERTY: no explicit UI schema needed; type determines the widget.
        //    If .Range() metadata is present, numeric types use Slider; otherwise DragFloat/DragInt.
        void operator()(const reflection::UI::None&) {

            // --- bool → Checkbox ---
            if (field.typeId == GetTypeId<bool>()) {
                bool val;
                field.Get(instance, &val);
                bool oldVal = val;
                if (ImGui::Checkbox(field.name.data(), &val)) {
                    field.Set(instance, &val);
                    field.OnChange(instance, &oldVal);
                }
                return;
            }

            // --- float → SliderFloat (if Range) or DragFloat ---
            if (field.typeId == GetTypeId<float>()) {
                const auto* metaMin = field.GetMeta(MetaKeys::Min);
                const auto* metaMax = field.GetMeta(MetaKeys::Max);
                bool hasRange = metaMin && metaMax;

                float val;
                field.Get(instance, &val);
                float oldVal = val;

                if (hasRange) {
                    float min = 0.f, max = 100.f;
                    if (std::holds_alternative<float>(*metaMin)) min = std::get<float>(*metaMin);
                    if (std::holds_alternative<float>(*metaMax)) max = std::get<float>(*metaMax);
                    if (ImGui::SliderFloat(field.name.data(), &val, min, max)) {
                        field.Set(instance, &val);
                        field.OnChange(instance, &oldVal);
                    }
                } else {
                    if (ImGui::DragFloat(field.name.data(), &val)) {
                        field.Set(instance, &val);
                        field.OnChange(instance, &oldVal);
                    }
                }
                return;
            }

            // --- int → SliderInt (if Range) or DragInt ---
            if (field.typeId == GetTypeId<int>()) {
                const auto* metaMin = field.GetMeta(MetaKeys::Min);
                const auto* metaMax = field.GetMeta(MetaKeys::Max);
                bool hasRange = metaMin && metaMax;

                int val;
                field.Get(instance, &val);
                int oldVal = val;

                if (hasRange) {
                    int minI = 0, maxI = 100;
                    if (std::holds_alternative<float>(*metaMin)) minI = (int)std::get<float>(*metaMin);
                    else if (std::holds_alternative<int>(*metaMin)) minI = std::get<int>(*metaMin);
                    if (std::holds_alternative<float>(*metaMax)) maxI = (int)std::get<float>(*metaMax);
                    else if (std::holds_alternative<int>(*metaMax)) maxI = std::get<int>(*metaMax);
                    if (ImGui::SliderInt(field.name.data(), &val, minI, maxI)) {
                        field.Set(instance, &val);
                        field.OnChange(instance, &oldVal);
                    }
                } else {
                    if (ImGui::DragInt(field.name.data(), &val)) {
                        field.Set(instance, &val);
                        field.OnChange(instance, &oldVal);
                    }
                }
                return;
            }

            // --- double → DragScalar ---
            if (field.typeId == GetTypeId<double>()) {
                double val;
                field.Get(instance, &val);
                double oldVal = val;
                float speed = 0.1f;
                if (ImGui::DragScalar(field.name.data(), ImGuiDataType_Double, &val, speed)) {
                    field.Set(instance, &val);
                    field.OnChange(instance, &oldVal);
                }
                return;
            }

            // --- SString → TextInput ---
            if (field.typeId == GetTypeId<shine::SString>()) {
                shine::SString val;
                field.Get(instance, &val);
                shine::SString oldVal = val;

                char buffer[256];
                strncpy_s(buffer, val.c_str(), sizeof(buffer) - 1);

                if (ImGui::InputText(field.name.data(), buffer, sizeof(buffer))) {
                    val = shine::SString(buffer);
                    field.Set(instance, &val);
                    field.OnChange(instance, &oldVal);
                }
                return;
            }

            // --- Enum → Dropdown (Combo) ---
            const reflection::TypeInfo* fieldTypeInfo = TypeRegistry::Get().FindFast(field.typeId);
            if (fieldTypeInfo && fieldTypeInfo->isEnum) {
                void* fieldPtr = static_cast<char*>(instance) + field.offset;
                int64_t currentVal = 0;

                if (field.size == 4) currentVal = (int64_t)*(int32_t*)fieldPtr;
                else if (field.size == 1) currentVal = (int64_t)*(int8_t*)fieldPtr;
                else if (field.size == 2) currentVal = (int64_t)*(int16_t*)fieldPtr;
                else if (field.size == 8) currentVal = *(int64_t*)fieldPtr;

                const char* currentName = "Unknown";
                for (const auto& e : fieldTypeInfo->enumEntries) {
                    if (e.value == currentVal) { currentName = e.name.data(); break; }
                }

                if (ImGui::BeginCombo(field.name.data(), currentName)) {
                    for (const auto& e : fieldTypeInfo->enumEntries) {
                        bool isSelected = (currentVal == e.value);
                        if (ImGui::Selectable(e.name.data(), isSelected)) {
                            if (field.size == 4) *(int32_t*)fieldPtr = (int32_t)e.value;
                            else if (field.size == 1) *(int8_t*)fieldPtr = (int8_t)e.value;
                            else if (field.size == 2) *(int16_t*)fieldPtr = (int16_t)e.value;
                            else if (field.size == 8) *(int64_t*)fieldPtr = e.value;

                            field.OnChange(instance, &currentVal);
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                return;
            }

            // --- Struct (recursive) ---
            if (fieldTypeInfo && !fieldTypeInfo->fields.empty()) {
                if (ImGui::TreeNode(field.name.data())) {
                    void* fieldInstance = static_cast<char*>(instance) + field.offset;
                    InspectorBuilder::DrawInspector(fieldInstance, fieldTypeInfo);
                    ImGui::TreePop();
                }
                return;
            }

            // --- Array/Vector ---
            if (field.containerType == ContainerType::Sequence) {
                const auto* trait = static_cast<const reflection::SequenceTrait*>(field.containerTrait);
                if (trait && ImGui::TreeNode(field.name.data())) {
                    void* arrayPtr = static_cast<char*>(instance) + field.offset;
                    size_t size = trait->GetSize(arrayPtr);

                    ImGui::Text("Size: %zu", size);

                    if (ImGui::Button("+")) {
                        trait->Resize(arrayPtr, size + 1);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("-") && size > 0) {
                        trait->Resize(arrayPtr, size - 1);
                    }

                    for (size_t i = 0; i < size; ++i) {
                        ImGui::PushID((int)i);
                        ImGui::Text("Element %zu", i);
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                return;
            }

            // --- Final fallback ---
            ImGui::Text("%s", field.name.data());
            ImGui::SameLine();
            ImGui::TextDisabled("(Unknown Type)");
        }

        // 2. Slider
        void operator()(const reflection::UI::Slider& slider) {
            float min = slider.min;
            float max = slider.max;

            const auto* metaMin = field.GetMeta(MetaKeys::Min);
            const auto* metaMax = field.GetMeta(MetaKeys::Max);

            // Float Slider
            if (field.typeId == GetTypeId<float>()) {
                if (metaMin && std::holds_alternative<float>(*metaMin)) min = std::get<float>(*metaMin);
                if (metaMax && std::holds_alternative<float>(*metaMax)) max = std::get<float>(*metaMax);

                float val;
                field.Get(instance, &val);
                float oldVal = val;
                if (ImGui::SliderFloat(field.name.data(), &val, min, max)) {
                    field.Set(instance, &val);
                    field.OnChange(instance, &oldVal);
                }
            }
            // Int Slider
            else if (field.typeId == GetTypeId<int>()) {
                int minI = (int)min;
                int maxI = (int)max;

                if (metaMin) {
                    if (std::holds_alternative<int>(*metaMin)) minI = std::get<int>(*metaMin);
                    else if (std::holds_alternative<float>(*metaMin)) minI = (int)std::get<float>(*metaMin);
                }
                if (metaMax) {
                    if (std::holds_alternative<int>(*metaMax)) maxI = std::get<int>(*metaMax);
                    else if (std::holds_alternative<float>(*metaMax)) maxI = (int)std::get<float>(*metaMax);
                }

                int val;
                field.Get(instance, &val);
                int oldVal = val;
                if (ImGui::SliderInt(field.name.data(), &val, minI, maxI)) {
                    field.Set(instance, &val);
                    field.OnChange(instance, &oldVal);
                }
            }
        }

        // 3. Checkbox
        void operator()(const reflection::UI::Checkbox&) {
            if (field.typeId == GetTypeId<bool>()) {
                bool val;
                field.Get(instance, &val);
                bool oldVal = val;
                if (ImGui::Checkbox(field.name.data(), &val)) {
                    field.Set(instance, &val);
                    field.OnChange(instance, &oldVal);
                }
            }
        }

        // 4. TextInput (InputText alias)
        void operator()(const reflection::UI::TextInput&) {
            if (field.typeId == GetTypeId<shine::SString>()) {
                shine::SString val;
                field.Get(instance, &val);
                shine::SString oldVal = val;
                
                char buffer[256];
                strncpy_s(buffer, val.c_str(), sizeof(buffer) - 1);
                
                if (ImGui::InputText(field.name.data(), buffer, sizeof(buffer))) {
                    val = shine::SString(buffer);
                    field.Set(instance, &val);
                    field.OnChange(instance, &oldVal);
                }
            }
        }

        // 5. ColorPicker (Color alias)
        void operator()(const reflection::UI::ColorPicker&) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Color Not Implemented");
        }

        // 6. Function Selector
        void operator()(const reflection::UI::FunctionSelector& selector) {
            shine::SString currentFunc;
            field.Get(instance, &currentFunc);
            
            if (ImGui::BeginCombo(field.name.data(), currentFunc.c_str())) {
                if (ownerType) {
                    for (const auto& method : ownerType->methods) {
                        bool show = !selector.onlyScriptCallable;
                        
                        if (selector.onlyScriptCallable) {
                            if (reflection::HasFlag(method.flags, FunctionFlags::ScriptCallable)) show = true;
                            
                            const auto* bpMeta = method.GetMeta(MetaKeys::BlueprintFunction);
                            if (bpMeta) show = true;
                        }
                        
                        if (show) {
                            bool isSelected = (currentFunc == method.name);
                            if (ImGui::Selectable(method.name.data(), isSelected)) {
                                shine::SString oldVal = currentFunc;
                                currentFunc = shine::SString(method.name);
                                field.Set(instance, &currentFunc);
                                field.OnChange(instance, &oldVal);
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                    }
                } else {
                    ImGui::TextDisabled("Owner Type Unknown");
                }
                ImGui::EndCombo();
            }
        }

        // 7. NumberInput
        void operator()(const reflection::UI::NumberInput&) {
            if (field.typeId == GetTypeId<float>()) {
                float val;
                field.Get(instance, &val);
                float oldVal = val;
                if (ImGui::DragFloat(field.name.data(), &val)) {
                    field.Set(instance, &val);
                    field.OnChange(instance, &oldVal);
                }
            } else if (field.typeId == GetTypeId<int>()) {
                int val;
                field.Get(instance, &val);
                int oldVal = val;
                if (ImGui::DragInt(field.name.data(), &val)) {
                    field.Set(instance, &val);
                    field.OnChange(instance, &oldVal);
                }
            }
        }

        // 8. Dropdown
        void operator()(const reflection::UI::Dropdown&) {
            ImGui::TextDisabled("Dropdown Not Implemented");
        }

        // 9. FilePicker
        void operator()(const reflection::UI::FilePicker&) {
            ImGui::TextDisabled("FilePicker Not Implemented");
        }

        // 10. VectorEditor
        void operator()(const reflection::UI::VectorEditor&) {
            ImGui::TextDisabled("VectorEditor Not Implemented");
        }

        // 11. MatrixEditor
        void operator()(const reflection::UI::MatrixEditor&) {
            ImGui::TextDisabled("MatrixEditor Not Implemented");
        }
    };

    void PropertyDrawer::DrawField(void* instance, const reflection::FieldInfo& field, const reflection::TypeInfo* ownerType) {
        std::visit(FieldRendererVisitor{ instance, field, ownerType }, field.uiSchema);
    }

    bool PropertyDrawer::DrawFloat(const char* label, float& value, float min, float max) {
        if (min == 0.0f && max == 0.0f) {
            return ImGui::DragFloat(label, &value);
        }
        return ImGui::SliderFloat(label, &value, min, max);
    }

    bool PropertyDrawer::DrawInt(const char* label, int& value, int min, int max) {
        if (min == 0 && max == 0) {
            return ImGui::DragInt(label, &value);
        }
        return ImGui::SliderInt(label, &value, min, max);
    }

    bool PropertyDrawer::DrawBool(const char* label, bool& value) {
        return ImGui::Checkbox(label, &value);
    }

    bool PropertyDrawer::DrawString(const char* label, shine::SString& value) {
        char buffer[256];
        strncpy_s(buffer, value.c_str(), sizeof(buffer) - 1);
        if (ImGui::InputText(label, buffer, sizeof(buffer))) {
            value = shine::SString(buffer);
            return true;
        }
        return false;
    }

}
