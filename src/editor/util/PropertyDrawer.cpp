#include "PropertyDrawer.h"
#include "InspectorBuilder.h"
#include "imgui/imgui.h"
#include <variant>
#include <string>
#include <algorithm>

namespace shine::editor::util {


    using namespace reflection;

    // Internal Visitor
    struct FieldRendererVisitor {
        void* instance;
        const reflection::FieldInfo& field;
        const reflection::TypeInfo* ownerType;

        // 1. None / Default Fallback
        void operator()(const reflection::UI::None&) {


            // --- Recursive Struct Drawing ---
            // If field is a POD or Reflectable Struct (but not a basic type we handled elsewhere), try to draw its inspector
            // Check if it has TypeInfo registered
            const reflection::TypeInfo* fieldTypeInfo = TypeRegistry::Get().Find(field.typeId);
            if (fieldTypeInfo) {
                // Enum Handling
                if (fieldTypeInfo->isEnum) {
                    void* fieldPtr = static_cast<char*>(instance) + field.offset;
                    int64_t currentVal = 0;
                    
                    // Simple support for common enum sizes (assuming int/enum class : int)
                    // TODO: Handle other sizes properly based on field.size
                    if (field.size == 4) currentVal = (int64_t)*(int32_t*)fieldPtr;
                    else if (field.size == 1) currentVal = (int64_t)*(int8_t*)fieldPtr;
                    else if (field.size == 2) currentVal = (int64_t)*(int16_t*)fieldPtr;
                    else if (field.size == 8) currentVal = *(int64_t*)fieldPtr;

                    const char* currentName = "Unknown";
                    for(const auto& e : fieldTypeInfo->enumEntries) {
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
                                
                                if (field.onChange) field.onChange(instance, &currentVal); // Pass OLD value
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    return;
                }
            } else {
                // If TypeInfo is missing but it's an enum (checked via other means? No field info doesn't tell us if it's enum unless we have TypeInfo)
                // But wait, field.typeId IS the enum type id.
                // If we can't find TypeInfo, we can't draw enum names.
                // We should fallback to int editor if we suspect it's an enum or just int.
                // But PropertyDrawer doesn't know if it is an enum without TypeInfo.
            }

            if (fieldTypeInfo && !fieldTypeInfo->fields.empty()) {
                    if (ImGui::TreeNode(field.name.data())) {
                        // Calculate pointer to field instance
                        // Note: This requires the field getter to support getting a pointer, or we compute offset manually
                        // Since we are inside the object, instance + offset is the field address.
                        void* fieldInstance = static_cast<char*>(instance) + field.offset;
                        
                        InspectorBuilder::DrawInspector(fieldInstance, fieldTypeInfo);
                        
                        ImGui::TreePop();
                    }
                    return;
            } else {
                // TypeInfo Missing Fallbacks
            }

            // Fallback for int-like types if no schema provided (and no TypeInfo found above)
            // This handles Enums that are not registered (TypeInfo not found) but are fundamentally ints.
            // Or just plain ints that didn't use .UI(Slider)
            if (field.typeId == GetTypeId<int>() || field.size == sizeof(int)) {
                // WARNING: field.size == 4 is a heuristic. It could be float, int, uint, or Enum.
                // If it was float, GetTypeId<float> check in Slider/etc usually handles it, but here we are in None schema.
                
                // Let's try to be smart. If GetTypeId matches int exactly:
                if (field.typeId == GetTypeId<int>()) {
                    int val;
                    field.Get(instance, &val);
                    int oldVal = val;
                    if (ImGui::InputInt(field.name.data(), &val)) {
                        field.Set(instance, &val);
                        if (field.onChange) field.onChange(instance, &oldVal);
                    }
                    return;
                }
                
                // If it is an Enum (but missing info), it likely has a unique TypeId but size 4.
                // We can't distinguish it from float easily without TypeInfo or extra metadata.
                // But we can try to render it as int if we are desperate.
                // Let's NOT do dangerous casts unless we are sure.
            }

            // --- Array/Vector Drawing ---
            if (field.containerType == ContainerType::Sequence) {
                const auto *trait = static_cast<const reflection::ArrayTrait *>(field.containerTrait);
                if (ImGui::TreeNode(field.name.data())) {
                    void* arrayPtr = static_cast<char*>(instance) + field.offset;
                    size_t size = trait->getSize(arrayPtr);
                    
                    // Simple Size Display
                    ImGui::Text("Size: %zu", size);

                    // Add Button (Simple resize + 1)
                    if (ImGui::Button("+")) {
                        trait->resize(arrayPtr, size + 1);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("-") && size > 0) {
                        trait->resize(arrayPtr, size - 1);
                    }

                    // Elements
                    for (size_t i = 0; i < size; ++i) {
                        ImGui::PushID((int)i);
                        void* elemPtr = trait->getElement(arrayPtr, i);
                        
                        // Try to find TypeInfo for element
                        // Note: field.typeId is vector<T>, we need T. 
                        // Current Reflection system might not expose T directly in FieldInfo easily without parsing name
                        // BUT, usually we register vectors as fields. 
                        // For this prototype, let's assume we can't easily deep-draw generic T without more info in FieldInfo.
                        // Wait! FieldInfo doesn't store ElementTypeId. 
                        // Fix: We need to enhance Reflection to store elementTypeId for arrays.
                        // For now, print placeholder.
                        ImGui::Text("Element %zu", i);
                        
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                return;
            }

            // Fallback: If it's an Enum-like type (size 1/2/4/8) but we missed TypeInfo,
            // we might want to display it as int with a warning.
            // But checking size is risky (could be float).
            
            // Check for float specifically to avoid confusion
            if (field.typeId == GetTypeId<float>()) {
                float val;
                field.Get(instance, &val);
                float oldVal = val;
                if (ImGui::DragFloat(field.name.data(), &val)) {
                    field.Set(instance, &val);
                    if (field.onChange) field.onChange(instance, &oldVal);
                }
                return;
            }

            ImGui::Text("%s", field.name.data());
            ImGui::SameLine();
            ImGui::TextDisabled("(No UI Schema)");
            // Debug Hint
            // ImGui::SameLine(); ImGui::TextDisabled("[%s]", field.typeId == GetTypeId<int>() ? "int" : "unknown");
        }

        // 2. Slider
        void operator()(const reflection::UI::Slider& slider) {
    

            // Get Min/Max from Metadata (Range) or fallback to Schema defaults
            float min = slider.min;
            float max = slider.max;

            auto* metaMin = field.GetMeta(Hash("Min"));
            auto* metaMax = field.GetMeta(Hash("Max"));

            // Float Slider
            if (field.typeId == GetTypeId<float>()) {
                if (metaMin) min = std::get<float>(*metaMin);
                if (metaMax) max = std::get<float>(*metaMax);

                float val;
                field.Get(instance, &val);
                float oldVal = val;
                if (ImGui::SliderFloat(field.name.data(), &val, min, max)) {
                    field.Set(instance, &val);
                    if (field.onChange) field.onChange(instance, &oldVal);
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
                    if (field.onChange) field.onChange(instance, &oldVal);
                }
            }
        }

        // 3. Checkbox
        void operator()(const reflection::UI::Checkbox& cb) {


            if (field.typeId == GetTypeId<bool>()) {
                bool val;
                field.Get(instance, &val);
                bool oldVal = val;
                if (ImGui::Checkbox(field.name.data(), &val)) {
                    field.Set(instance, &val);
                    if (field.onChange) field.onChange(instance, &oldVal);
                }
            }
        }

        // 4. InputText
        void operator()(const reflection::UI::InputText& text) {
   
            if (field.typeId == GetTypeId<std::string>()) {
                std::string val;
                field.Get(instance, &val); 
                std::string oldVal = val;
                
                char buffer[256];
                strncpy_s(buffer, val.c_str(), sizeof(buffer) - 1);
                
                if (ImGui::InputText(field.name.data(), buffer, sizeof(buffer))) {
                    val = buffer;
                    field.Set(instance, &val); 
                    if (field.onChange) field.onChange(instance, &oldVal);
                }
            }
            else if (field.typeId == GetTypeId<shine::SString>()) {
                shine::SString val;
                field.Get(instance, &val);
                // SString is move-only, so careful with oldVal if we want to support onChange
                // For now, skip oldVal for SString or implement manual clone
                
                std::string utf8 = val.to_utf8();
                char buffer[256];
                strncpy_s(buffer, utf8.c_str(), sizeof(buffer) - 1);
                
                if (ImGui::InputText(field.name.data(), buffer, sizeof(buffer))) {
                    // Reconstruct SString from UTF8
                    // We need to move it back
                    shine::SString newVal = shine::SString::from_utf8(buffer);
                    field.Set(instance, &newVal);
                    
                    // Trigger onChange without oldVal for now to avoid copy cost/complexity
                    if (field.onChange) field.onChange(instance, nullptr); 
                }
            }
        }

        // 5. Color
        void operator()(const reflection::UI::Color& color) {
             ImGui::TextColored(ImVec4(1, 0, 0, 1), "Color Not Implemented");
        }

        // 6. Function Selector
        void operator()(const reflection::UI::FunctionSelector& selector) {

            // Assume field is std::string
            std::string currentFunc;
            field.Get(instance, &currentFunc);
            
            if (ImGui::BeginCombo(field.name.data(), currentFunc.c_str())) {
                if (ownerType) {
                    for (const auto& method : ownerType->methods) {
                        bool show = !selector.onlyScriptCallable;
                        
                        // Check ScriptCallable flag
                        if (selector.onlyScriptCallable) {
                            if (HasFlag(method.flags, FunctionFlags::ScriptCallable)) show = true;
                            
                            // Also check for "BlueprintFunction" metadata
                            // Manual lookup since MethodInfo might not have GetMeta helper
                            TypeId bpKey = Hash("BlueprintFunction");
                            auto it = std::lower_bound(method.metadata.begin(), method.metadata.end(), bpKey, 
                                [](const auto& pair, TypeId k) { return pair.first < k; });
                            if (it != method.metadata.end() && it->first == bpKey) show = true;
                        }
                        
                        if (show) {
                            bool isSelected = (currentFunc == method.name);
                            if (ImGui::Selectable(method.name.data(), isSelected)) {
                                std::string oldVal = currentFunc;
                                currentFunc = method.name;
                                field.Set(instance, &currentFunc);
                                if (field.onChange) field.onChange(instance, &oldVal);
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

    bool PropertyDrawer::DrawString(const char* label, std::string& value) {
        char buffer[256];
        strncpy_s(buffer, value.c_str(), sizeof(buffer) - 1);
        if (ImGui::InputText(label, buffer, sizeof(buffer))) {
            value = buffer;
            return true;
        }
        return false;
    }

}
