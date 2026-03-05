#pragma once

namespace shine::editor::views
{
    enum class EPlacementItemType : int
    {
        EmptyActor = 0,
        CubeActor = 1
    };

    struct PlacementItemPayload
    {
        EPlacementItemType type = EPlacementItemType::EmptyActor;
        float scale = 1.0f;
        char label[64]{};
    };
}
