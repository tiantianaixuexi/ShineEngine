#pragma once

#include <type_traits>



#include "shine_define.h"
#include "math/mathDef.h"
#include "math/mathUtil.h"


namespace shine::math
{

    template <typename T>
    struct TRotator
    {
        static_assert(std::is_floating_point_v<T>, "T must be floating point");

    public:
        T Pitch; // 
        T Yaw;   //
        T Roll;  //

        TRotator() : Pitch(0), Yaw(0), Roll(0) {}
        TRotator(T pitch) : Pitch(pitch), Yaw(pitch), Roll(pitch) {}

        TRotator(T pitch, T yaw, T roll) : Pitch(pitch), Yaw(yaw), Roll(roll) {}

    public:
        static const TRotator<T> ZeroRotator()
        {
            return TRotator<T>(0, 0, 0);
        };

        TRotator<T> operator+(const TRotator<T> &R) const
        {
            return TRotator<T>(Pitch + R.Pitch, Yaw + R.Yaw, Roll + R.Roll);
        }

        TRotator<T> operator-(const TRotator<T> &R) const
        {
            return TRotator<T>(Pitch - R.Pitch, Yaw - R.Yaw, Roll - R.Roll);
        }

        bool operator==(const TRotator<T> &R) const
        {
            return Pitch == R.Pitch && Yaw == R.Yaw && Roll == R.Roll;
        }

        bool operator!=(const TRotator<T> &R) const
        {
            return Pitch != R.Pitch || Yaw != R.Yaw || Roll != R.Roll;
        }

        TRotator<T> operator+=(const TRotator<T> &R)
        {
            Pitch += R.Pitch;
            Yaw += R.Yaw;
            Roll += R.Roll;
            return *this;
        }

        TRotator<T> operator-=(const TRotator<T> &R)
        {
            Pitch -= R.Pitch;
            Yaw -= R.Yaw;
            Roll -= R.Roll;
            return *this;
        }

        bool IsNearlyZero(T Tolerance = KINDA_SMALL_NUMBER) const;
        bool IsZero() const;
        bool Equlas(const TRotator &R, T Tolerance = KINDA_SMALL_NUMBER) const;
        T ClampAxis(T Angle);
        T NormalizeAxis(T Angle);

        TRotator<T> Add(T DeltaPitch, T DeltaYaw, T DeltaRoll) const;
    };

    // TODO : SSE向量优化
    template <typename T>
    inline bool TRotator<T>::IsNearlyZero(T Tolerance) const
    {
        return Abs(NormalizeAxis(Pitch)) <= Tolerance &&
               Abs(NormalizeAxis(Yaw)) <= Tolerance &&
              Abs(NormalizeAxis(Roll)) <= Tolerance;
    }

    template <typename T>
    inline bool TRotator<T>::IsZero() const
    {
        return (ClampAxis(Pitch) == 0.0f) &&
               (ClampAxis(Yaw) == 0.0f) &&
               (ClampAxis(Roll) == 0.0f);
    }

    template <typename T>
    inline bool TRotator<T>::Equlas(const TRotator &R, T Tolerance) const
    {

        return Abs(NormalizeAxis(Pitch - R.Pitch)) <= Tolerance &&
               Abs(NormalizeAxis(Yaw - R.Yaw)) <= Tolerance &&
              Abs(NormalizeAxis(Roll - R.Roll)) <= Tolerance;
    }

    template <typename T>
    inline T TRotator<T>::ClampAxis(T Angle)
    {
        Angle = Fmod(Angle, T(360));

        if (Angle < (T)0.0)
        {
            Angle += (T)360.0;
        }

        return Angle;
    }

    template <typename T>
    inline T TRotator<T>::NormalizeAxis(T Angle)
    {
        Angle = ClampAxis(Angle);

        if (Angle > 180.0f)
            Angle -= 360.0f;
        if (Angle < -180.0f)
            Angle += 360.0f;
        return Angle;
    }

    template <typename T>
    TRotator<T> TRotator<T>::Add(T DeltaPitch, T DeltaYaw, T DeltaRoll) const
    {
        Pitch += DeltaPitch;
        Yaw += DeltaYaw;
        Roll += DeltaRoll;
        return *this;
    }



    using FRotator3f = TRotator<float>;
    using FRotator3d = TRotator<double>;
}
