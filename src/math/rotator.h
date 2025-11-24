#pragma once


#include "mathDef.h"
#include "mathUtil.h"


namespace shine::math
{

    template <FloatingPoint T>
    struct TRotator
    {
    public:
        T Pitch; // 
        T Yaw;   // 
        T Roll;  // 

        constexpr TRotator() noexcept : Pitch(0), Yaw(0), Roll(0) {}
        constexpr TRotator(T pitch) noexcept : Pitch(pitch), Yaw(pitch), Roll(pitch) {}
        constexpr TRotator(T pitch, T yaw, T roll) noexcept : Pitch(pitch), Yaw(yaw), Roll(roll) {}
        
        constexpr TRotator(const TRotator&) noexcept = default;
        constexpr TRotator(TRotator&&) noexcept = default;
        constexpr TRotator& operator=(const TRotator&) noexcept = default;
        constexpr TRotator& operator=(TRotator&&) noexcept = default;

    public:
        [[nodiscard]] static constexpr TRotator<T> ZeroRotator() noexcept {
            return TRotator<T>(0, 0, 0);
        }

        [[nodiscard]] constexpr TRotator<T> operator+(const TRotator<T>& R) const noexcept {
            return TRotator<T>(Pitch + R.Pitch, Yaw + R.Yaw, Roll + R.Roll);
        }

        [[nodiscard]] constexpr TRotator<T> operator-(const TRotator<T>& R) const noexcept {
            return TRotator<T>(Pitch - R.Pitch, Yaw - R.Yaw, Roll - R.Roll);
        }

        [[nodiscard]] constexpr TRotator<T> operator*(T scalar) const noexcept {
            return TRotator<T>(Pitch * scalar, Yaw * scalar, Roll * scalar);
        }

        [[nodiscard]] constexpr TRotator<T> operator/(T scalar) const noexcept {
            T invScalar = T(1) / scalar;
            return *this * invScalar;
        }

        [[nodiscard]] constexpr TRotator<T> operator-() const noexcept {
            return TRotator<T>(-Pitch, -Yaw, -Roll);
        }

        [[nodiscard]] constexpr bool operator==(const TRotator<T>& R) const noexcept {
            return Pitch == R.Pitch && Yaw == R.Yaw && Roll == R.Roll;
        }

        [[nodiscard]] constexpr bool operator!=(const TRotator<T>& R) const noexcept {
            return Pitch != R.Pitch || Yaw != R.Yaw || Roll != R.Roll;
        }

        constexpr TRotator<T>& operator+=(const TRotator<T>& R) noexcept {
            Pitch += R.Pitch;
            Yaw += R.Yaw;
            Roll += R.Roll;
            return *this;
        }

        constexpr TRotator<T>& operator-=(const TRotator<T>& R) noexcept {
            Pitch -= R.Pitch;
            Yaw -= R.Yaw;
            Roll -= R.Roll;
            return *this;
        }

        constexpr TRotator<T>& operator*=(T scalar) noexcept {
            Pitch *= scalar;
            Yaw *= scalar;
            Roll *= scalar;
            return *this;
        }

        constexpr TRotator<T>& operator/=(T scalar) noexcept {
            T invScalar = T(1) / scalar;
            return *this *= invScalar;
        }

        [[nodiscard]] bool IsNearlyZero(T Tolerance = KINDA_SMALL_NUMBER) const noexcept;
        [[nodiscard]] bool IsZero() const noexcept;
        [[nodiscard]] bool Equals(const TRotator& R, T Tolerance = KINDA_SMALL_NUMBER) const noexcept;
        [[nodiscard]] static constexpr T ClampAxis(T Angle) noexcept;
        [[nodiscard]] static constexpr T NormalizeAxis(T Angle) noexcept;

        [[nodiscard]] constexpr TRotator<T> Add(T DeltaPitch, T DeltaYaw, T DeltaRoll) const noexcept;
        
        // 归一化所有轴
        [[nodiscard]] TRotator<T> GetNormalized() const noexcept;
        
        // 归一化自身
        void Normalize() noexcept;
        
        // 线性插值
        [[nodiscard]] static TRotator<T> Lerp(const TRotator<T>& A, const TRotator<T>& B, T Alpha) noexcept;
    };

    template <FloatingPoint T>
    inline bool TRotator<T>::IsNearlyZero(T Tolerance) const noexcept
    {
        return Abs(NormalizeAxis(Pitch)) <= Tolerance &&
               Abs(NormalizeAxis(Yaw)) <= Tolerance &&
               Abs(NormalizeAxis(Roll)) <= Tolerance;
    }

    template <FloatingPoint T>
    inline bool TRotator<T>::IsZero() const noexcept
    {
        return (ClampAxis(Pitch) == T(0)) &&
               (ClampAxis(Yaw) == T(0)) &&
               (ClampAxis(Roll) == T(0));
    }

    template <FloatingPoint T>
    inline bool TRotator<T>::Equals(const TRotator& R, T Tolerance) const noexcept
    {
        return Abs(NormalizeAxis(Pitch - R.Pitch)) <= Tolerance &&
               Abs(NormalizeAxis(Yaw - R.Yaw)) <= Tolerance &&
               Abs(NormalizeAxis(Roll - R.Roll)) <= Tolerance;
    }



    template <FloatingPoint T>
    inline constexpr T TRotator<T>::ClampAxis(T Angle) noexcept
    {
        Angle = Fmod(Angle, T(360));
        if (Angle < T(0)) {
            Angle += T(360);
        }
        return Angle;
    }

    template <FloatingPoint T>
    inline constexpr T TRotator<T>::NormalizeAxis(T Angle) noexcept
    {
        Angle = ClampAxis(Angle);
        if (Angle > T(180)) {
            Angle -= T(360);
        }
        if (Angle < T(-180)) {
            Angle += T(360);
        }
        return Angle;
    }

    template <FloatingPoint T>
    constexpr TRotator<T> TRotator<T>::Add(T DeltaPitch, T DeltaYaw, T DeltaRoll) const noexcept
    {
        return TRotator<T>(Pitch + DeltaPitch, Yaw + DeltaYaw, Roll + DeltaRoll);
    }

    template <FloatingPoint T>
    TRotator<T> TRotator<T>::GetNormalized() const noexcept
    {
        TRotator<T> result(*this);
        result.Normalize();
        return result;
    }

    template <FloatingPoint T>
    void TRotator<T>::Normalize() noexcept
    {
        Pitch = NormalizeAxis(Pitch);
        Yaw = NormalizeAxis(Yaw);
        Roll = NormalizeAxis(Roll);
    }

    template <FloatingPoint T>
    TRotator<T> TRotator<T>::Lerp(const TRotator<T>& A, const TRotator<T>& B, T Alpha) noexcept
    {
        Alpha = Clamp(Alpha, T(0), T(1));
        return TRotator<T>(
            A.Pitch + (B.Pitch - A.Pitch) * Alpha,
            A.Yaw + (B.Yaw - A.Yaw) * Alpha,
            A.Roll + (B.Roll - A.Roll) * Alpha
        );
    }



    using FRotator3f = TRotator<float>;
    using FRotator3d = TRotator<double>;
}
