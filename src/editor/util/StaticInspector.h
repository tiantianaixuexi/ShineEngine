#pragma once

#include "InspectorBuilder.h"
#include "imgui/imgui.h"

namespace shine::editor::util {

    template<typename T>
    struct StaticInspectorBuilder {
        using ObjectType = T;

        template<typename U>
        static void Draw(U* instance) {
            if (!instance) {
                return;
            }

            const auto* typeInfo = shine::reflection::TypeRegistry::Get().FindFast(
                shine::reflection::GetTypeId<T>());
            if (!typeInfo) {
                ImGui::BeginDisabled();
                ImGui::TextUnformatted("TypeInfo Not Found");
                ImGui::EndDisabled();
                return;
            }

            InspectorBuilder::DrawInspector(static_cast<void*>(instance), typeInfo);
        }
    };

}
