#include "PropertyDrawer.h"
#include "InspectorBuilder.h"
#include "imgui/imgui.h"
#include <variant>
#include <string>

namespace shine::editor::util {

    // Internal Visitor
    struct FieldRendererVisitor {
        void* instance;
        const Shine::Reflection::FieldInfo& field;

        // 1. None / Default Fallback
        void operator()(const Shine::Reflection::UI::None&) {
            using namespace Shine::Reflection;

            // --- Recursive Struct Drawing ---
            // If field is a POD or Reflectable Struct (but not a basic type we handled elsewhere), try to draw its inspector
            // Check if it has TypeInfo registered
            const TypeInfo* fieldTypeInfo = TypeRegistry::Get().Find(field.typeId);
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
            }

            // --- Array/Vector Drawing ---
            if (field.arrayTrait) {
                if (ImGui::TreeNode(field.name.data())) {
                    void* arrayPtr = static_cast<char*>(instance) + field.offset;
                    size_t size = field.arrayTrait->getSize(arrayPtr);
                    
                    // Simple Size Display
                    ImGui::Text("Size: %zu", size);

                    // Add Button (Simple resize + 1)
                    if (ImGui::Button("+")) {
                        field.arrayTrait->resize(arrayPtr, size + 1);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("-") && size > 0) {
                        field.arrayTrait->resize(arrayPtr, size - 1);
                    }

                    // Elements
                    for (size_t i = 0; i < size; ++i) {
                        ImGui::PushID((int)i);
                        void* elemPtr = field.arrayTrait->getElement(arrayPtr, i);
                        
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

            ImGui::Text("%s", field.name.data());
            ImGui::SameLine();
            ImGui::TextDisabled("(No UI Schema)");
        }

        // 2. Slider
        void operator()(const Shine::Reflection::UI::Slider& slider) {
            using namespace Shine::Reflection;

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
                if (ImGui::SliderFloat(field.name.data(), &val, min, max)) {
                    field.Set(instance, &val);
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
                if (ImGui::SliderInt(field.name.data(), &val, minI, maxI)) {
                    field.Set(instance, &val);
                }
            }
        }

        // 3. Checkbox
        void operator()(const Shine::Reflection::UI::Checkbox& cb) {
            using namespace Shine::Reflection;
            if (field.typeId == GetTypeId<bool>()) {
                bool val;
                field.Get(instance, &val);
                if (ImGui::Checkbox(field.name.data(), &val)) {
                    field.Set(instance, &val);
                }
            }
        }

        // 4. InputText
        void operator()(const Shine::Reflection::UI::InputText& text) {
            using namespace Shine::Reflection;
            // Simplified string handling (assumes std::string layout matches simple pointer logic or manual copy)
            // Warning: This part is tricky without exact TypeId<std::string>. 
            // We assume the field is std::string if it uses InputText UI.
            
            std::string val;
            field.Get(instance, &val); // This copies string content to local 'val' via generic getter
            
            char buffer[256];
            strncpy_s(buffer, val.c_str(), sizeof(buffer) - 1);
            
            if (ImGui::InputText(field.name.data(), buffer, sizeof(buffer))) {
                val = buffer;
                field.Set(instance, &val); // Copies back
            }
        }

        // 5. Color
        void operator()(const Shine::Reflection::UI::Color& color) {
             ImGui::TextColored(ImVec4(1, 0, 0, 1), "Color Not Implemented");
        }
    };

    void PropertyDrawer::DrawField(void* instance, const Shine::Reflection::FieldInfo& field) {
        std::visit(FieldRendererVisitor{ instance, field }, field.uiSchema);
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
