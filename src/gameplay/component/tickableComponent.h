#pragma once

#include "EngineCore/engine_context.h"
#include "gameplay/component/component.h"
#include "gameplay/tick/tickManager.h"

namespace shine::gameplay::component
{
    class TickableComponent : public UComponent
    {
    public:
        TickableComponent()
        {
            m_TickFunction.fn = &TickableComponent::DispatchTick;
            m_TickFunction.owner = this;
            m_TickFunction.enable = &m_EnableState;
        }

        ~TickableComponent() override
        {
            UnregisterTick();
        }

        void onAttached() override
        {
            RegisterTick();
        }

        void onDetached() override
        {
            UnregisterTick();
        }

        void setTickGroup(ETickGroup group)
        {
            const bool wasRegistered = m_TickFunction._registered;
            if (wasRegistered)
            {
                UnregisterTick();
            }
            m_TickFunction.group = group;
            if (wasRegistered)
            {
                RegisterTick();
            }
        }

        void setTickInterval(float interval)
        {
            m_TickFunction.interval = interval;
        }

        void setTickEnabled(bool enabled)
        {
            m_EnableState.enabled = enabled;
        }

        [[nodiscard]] bool isTickEnabled() const
        {
            return m_EnableState.enabled;
        }

        [[nodiscard]] bool isTickRegistered() const
        {
            return m_TickFunction._registered;
        }

    protected:
        virtual void onTick(float deltaTime)
        {
        }

        virtual bool shouldTick() const
        {
            return true;
        }

        void RegisterTick()
        {
            if (m_TickFunction._registered)
            {
                return;
            }

            if (!EngineContext::IsInitialized())
            {
                return;
            }

            if (auto* tickManager = EngineContext::Get().GetSystem<tick::TickManager>())
            {
                tickManager->Register(&m_TickFunction);
            }
        }

        void UnregisterTick()
        {
            if (!m_TickFunction._registered)
            {
                return;
            }

            if (!EngineContext::IsInitialized())
            {
                return;
            }

            if (auto* tickManager = EngineContext::Get().GetSystem<tick::TickManager>())
            {
                tickManager->Unregister(&m_TickFunction);
            }
        }

    private:
        static void DispatchTick(UComponent* owner, float deltaTime)
        {
            if (!owner)
            {
                return;
            }

            auto* self = static_cast<TickableComponent*>(owner);
            if (!self->shouldTick())
            {
                return;
            }

            self->onTick(deltaTime);
        }

    private:
        tick::TickEnableState m_EnableState;
        tick::TickFunction m_TickFunction;
    };
}
