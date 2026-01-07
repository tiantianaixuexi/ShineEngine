#include "InspectorBuilder.h"
#include "PropertyDrawer.h"
#include "imgui/imgui.h"
#include <string>
#include <variant>

namespace shine::editor::util {

    void InspectorBuilder::DrawInspector(void* instance, const Shine::Reflection::TypeInfo* typeInfo) {
        if (!instance || !typeInfo) {
            ImGui::TextDisabled("Invalid Instance or TypeInfo");
            return;
        }

        Shine::Reflection::InspectorView view;
        view.typeInfo = typeInfo;

        std::string currentCategory = "";

        for (auto& field : view) {
            // Category Grouping Logic
            auto* meta = field.GetMeta(Shine::Reflection::Hash("Category"));
            if (meta) {
                std::string_view cat = std::get<std::string_view>(*meta);
                if (cat != currentCategory) {
                    currentCategory = cat;
                    ImGui::Separator();
                    // Using a nice header color for categories
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", currentCategory.data());
                    ImGui::Spacing();
                }
            }

            // Draw individual field
            PropertyDrawer::DrawField(instance, field);
        }
    }

}
