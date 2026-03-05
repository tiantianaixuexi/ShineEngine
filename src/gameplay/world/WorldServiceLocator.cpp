#include "gameplay/world/WorldServiceLocator.h"

#include "EngineCore/engine_context.h"
#include "gameplay/world/world_service.h"

namespace shine::gameplay::world
{
    IWorldActorPlacementService* ResolveWorldPlacementService(EngineContext& ctx)
    {
        auto* worldService = ctx.GetSystem<WorldService>();
        return static_cast<IWorldActorPlacementService*>(worldService);
    }

    IWorldActorHierarchyService* ResolveWorldHierarchyService(EngineContext& ctx)
    {
        auto* worldService = ctx.GetSystem<WorldService>();
        return static_cast<IWorldActorHierarchyService*>(worldService);
    }
}
