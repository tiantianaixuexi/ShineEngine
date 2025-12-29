#pragma once

#include <functional>

#include "gameplay/tick/tick_function.h"
#include "gameplay/tick/tick_types.h"
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
            tick::TickManager::get().Register(&fn);
        }

        void UnregisterTick(tick::TickFunction& fn) {
            tick::TickManager::get().Unregister(&fn);
        }
    };

}
