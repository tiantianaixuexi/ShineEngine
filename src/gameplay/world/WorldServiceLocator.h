#pragma once

namespace shine
{
    class EngineContext;
}

namespace shine::gameplay::world
{
    class IWorldActorPlacementService;
    class IWorldActorHierarchyService;

    IWorldActorPlacementService* ResolveWorldPlacementService(EngineContext& ctx);
    IWorldActorHierarchyService* ResolveWorldHierarchyService(EngineContext& ctx);
}
