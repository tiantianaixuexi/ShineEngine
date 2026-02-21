#pragma once

#include "gameplay/component/component.h"
#include "math/transform.h"

namespace shine::gameplay::component
{
    class TransformComponent : public UComponent
    {
    public:
        TransformComponent() = default;
        ~TransformComponent() override = default;

        void setPosition(const math::FVector3f& pos) { m_Transform.Position = pos; m_isDirty = true; }
        void setRotation(const math::FRotator3f& rot) { m_Transform.Rotation = rot; m_isDirty = true; }
        void setScale(const math::FVector3f& scale) { m_Transform.Scale = scale; m_isDirty = true; }

        [[nodiscard]] const math::FVector3f& getPosition() const { return m_Transform.Position; }
        [[nodiscard]] const math::FRotator3f& getRotation() const { return m_Transform.Rotation; }
        [[nodiscard]] const math::FVector3f& getScale() const { return m_Transform.Scale; }
        
        [[nodiscard]] const math::FTransform3f& getTransform() const { return m_Transform; }
        void setTransform(const math::FTransform3f& transform) { m_Transform = transform; m_isDirty = true; }

        const math::FMatrix4f& getModelMatrix() const
        {
            if (m_isDirty)
            {
                m_ModelMatrix = m_Transform.ToMatrixWithScale();
                m_isDirty = false;
            }
            return m_ModelMatrix;
        }

    private:
        math::FTransform3f m_Transform;

        mutable math::FMatrix4f m_ModelMatrix{ math::FMatrix4f::identity() };
        mutable bool m_isDirty{ true };
    };
}
