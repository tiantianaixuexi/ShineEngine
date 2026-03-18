#include "InspectorBuilder.h"
#include "PropertyDrawer.h"
#include "imgui/imgui.h"
#include <variant>

namespace shine::editor::util {

    void InspectorBuilder::DrawInspector(void* instance, const reflection::TypeInfo* typeInfo) {
        if (!instance || !typeInfo) {
            ImGui::TextDisabled("Invalid Instance or TypeInfo");
            return;
        }

        reflection::InspectorView view;
        view.typeInfo = typeInfo;

        shine::STextView currentCategory;

        for (auto& field : view) {
            // Category Grouping Logic
            if (field.HasCategory()) {
                shine::STextView cat = field.GetCategoryView();
                if (cat != currentCategory) {
                    currentCategory = cat;
                    ImGui::Separator();
                    // Using a nice header color for categories
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", currentCategory.data());
                    ImGui::Spacing();
                }
            }

            // Draw individual field
            PropertyDrawer::DrawField(instance, field, typeInfo);
        }
    }

}
