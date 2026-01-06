#pragma once

#include "gameplay/tick/tick_function.h"
#include "gameplay/tick/tickManager.h"

namespace shine::gameplay
{
    template<typename Derived>
    class TickableComponent {
    public:
        TickableComponent() = default;
        virtual ~TickableComponent() = default;

        void OnRegister() {
            static_cast<Derived*>(this)->RegisterTicks();
        }

        void OnUnregister() {
            static_cast<Derived*>(this)->UnregisterTicks();
        }

        virtual bool ShouldTick() const { return true; }

    protected:
        void RegisterTick(tick::TickFunction& fn) {
            if (EngineContext::IsInitialized()) {
                EngineContext::Get().GetSystem<tick::TickManager>()->Register(&fn);
            }
        }

        void UnregisterTick(tick::TickFunction& fn) {
            if (EngineContext::IsInitialized()) {
                EngineContext::Get().GetSystem<tick::TickManager>()->Unregister(&fn);
            }
        }
    };

}
