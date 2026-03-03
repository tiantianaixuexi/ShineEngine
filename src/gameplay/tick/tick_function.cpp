#include "tick_function.h"

#include "EngineCore/engine_context.h"
#include "tickManager.h"

namespace shine::gameplay::tick
{
    TickFunction::~TickFunction()
    {
        if (!_registered)
        {
            return;
        }

        if (!EngineContext::IsInitialized())
        {
            return;
        }

        if (auto* tickManager = EngineContext::Get().GetSystem<TickManager>())
        {
            tickManager->Unregister(this);
        }
    }
}
