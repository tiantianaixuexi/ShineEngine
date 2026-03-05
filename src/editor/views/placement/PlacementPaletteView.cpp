#include "editor/views/placement/PlacementPaletteView.h"

#include <array>
#include <cstring>

#include "imgui/imgui.h"
#include "editor/views/placement/PlacementPayload.h"

namespace shine::editor::views
{
    namespace
    {
        struct PlacementItemConfig
        {
            EPlacementItemType type;
            const char* label;
            float scale;
        };
    }

    void PlacementPaletteView::onInit()
    {
        SetName("物品放置栏");
    }

    void PlacementPaletteView::onShutDown()
    {
    }

    void PlacementPaletteView::onRender()
    {
        if (!ImGui::Begin(name.c_str(), &isOpen))
        {
            ImGui::End();
            return;
        }

        static constexpr std::array<PlacementItemConfig, 2> items{{
            { EPlacementItemType::CubeActor, "立方体 Actor", 0.35f },
            { EPlacementItemType::EmptyActor, "空 Actor", 1.0f }
        }};

        for (const auto& item : items)
        {
            ImGui::Selectable(item.label);
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                PlacementItemPayload payload;
                payload.type = item.type;
                payload.scale = item.scale;
                std::strncpy(payload.label, item.label, sizeof(payload.label) - 1);
                ImGui::SetDragDropPayload("SHINE_PLACE_ITEM", &payload, sizeof(payload));
                ImGui::Text("%s", item.label);
                ImGui::EndDragDropSource();
            }
        }

        ImGui::End();
    }
}
