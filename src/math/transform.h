#pragma once

#include "math/vector.ixx"
#include "math/rotator.h"
#include "math/matrix.ixx"
#include "math/mathUtil.h"

namespace shine::math
{
    template<FloatingPoint T>
    struct TRotator;

    template<FloatingPoint T>
    class Matrix4;

    template<FloatingPoint T>
    struct TTransform
    {
        TVector<T> Position;
        TRotator<T> Rotation;
        TVector<T> Scale;

        constexpr TTransform() noexcept 
            : Position(TVector<T>::Zero())
            , Rotation(TRotator<T>::ZeroRotator())
            , Scale(TVector<T>::One())
        {}

        constexpr TTransform(const TVector<T>& pos, const TRotator<T>& rot, const TVector<T>& scale) noexcept
            : Position(pos), Rotation(rot), Scale(scale)
        {}

        [[nodiscard]] Matrix4<T> ToMatrixWithScale() const
        {
            return MakeMatrix4(Position, Rotation, Scale);
        }

        [[nodiscard]] static TTransform<T> Identity()
        {
            return TTransform<T>();
        }
    };

    SHINE_MODULE_EXPORT using FTransform3f = TTransform<float>;
    SHINE_MODULE_EXPORT using FTransform3d = TTransform<double>;
}
