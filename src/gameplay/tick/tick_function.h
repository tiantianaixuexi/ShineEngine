#pragma once


#include <vector>

#include "tick_types.h"


namespace shine::gameplay
{
    using TickFn = void(*)(void* userdata, float dt);

    namespace component
    {
        class UComponent;
    }

    namespace tick
    {
        class TickManager;

        struct TickEnableState {
            bool enabled = true;
        };

        struct TickFunction {
            TickFn      fn = nullptr;
            void* userdata = nullptr;
            ETickGroup   group = ETickGroup::PrePhysics;

            float       interval = 0.f;
            float       accTime = 0.f;

            TickEnableState* enable = nullptr;

            u32 execIndex = 0;
            u32 execOrder = 0;

            std::vector<TickFunction*> dependencies;

            bool _registered = false;

            ~TickFunction() {
                if (_registered) {
                    TickManager::get().Unregister(this);
                }
            }

            void SetRegistered(bool registered) {
                _registered = registered;
            }
        };
    }

}