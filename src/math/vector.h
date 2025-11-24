#pragma once

#include <memory>
#include "mathDef.h"

namespace shine::math
{

    template <MathPoint T>
    struct TVector
    {

    public:
        T X;
        T Y;
        T Z;

        static constexpr TVector<T> Zero() { return TVector<T>(0, 0, 0); }
        static constexpr TVector<T> One() { return TVector<T>(1, 1, 1); }
        static constexpr TVector<T> Up() { return TVector<T>(0, 0, 1); }
        static constexpr TVector<T> Down() { return TVector<T>(0, 0, -1); }
        static constexpr TVector<T> Left() { return TVector<T>(0, -1, 0); }
        static constexpr TVector<T> Right() { return TVector<T>(0, 1, 0); }
        static constexpr TVector<T> Forward() { return TVector<T>(1, 0, 0); }
        static constexpr TVector<T> Back() { return TVector<T>(-1, 0, 0); }
        static constexpr TVector<T> XAxis() { return TVector<T>(1, 0, 0); }
        static constexpr TVector<T> YAxis() { return TVector<T>(0, 1, 0); }
        static constexpr TVector<T> ZAxis() { return TVector<T>(0, 0, 1); }

        constexpr TVector(const TVector<T>& other) noexcept = default;
        constexpr TVector<T>& operator=(const TVector<T>& other) noexcept = default;
        constexpr TVector<T>& operator=(TVector<T>&& Other) noexcept = default;

        constexpr TVector() noexcept: X(0), Y(0), Z(0)  {}
        constexpr TVector(T _x) noexcept: X(_x), Y(_x), Z(_x)  {}
        constexpr TVector(T _x, T _y, T _z) noexcept: X(_x), Y(_y), Z(_z)  {}
		constexpr TVector(T _x, T _y) noexcept: X(_x), Y(_y), Z(0)  {}


        constexpr TVector(TVector<T> && Other) noexcept:
            X(std::move(Other.X)),
            Y(std::move(Other.Y)),
            Z(std::move(Other.Z))
        {

        }


        constexpr TVector<T> operator+(const TVector<T> &V)  const noexcept
        {
            return TVector<T>(X + V.X, Y + V.Y, Z + V.Z);
        }

        constexpr TVector<T> operator-(const TVector<T> &V)  const noexcept
        {
            return TVector<T>(X - V.X, Y - V.Y, Z - V.Z);
        }

        constexpr TVector<T> operator*(const TVector<T> &V)  const noexcept
        {
            return TVector<T>(X * V.X, Y * V.Y, Z * V.Z);
        }

        constexpr TVector<T> operator/(const TVector<T> &V)  const noexcept
        {
            return TVector<T>(X / V.X, Y / V.Y, Z / V.Z);
        }

        constexpr TVector<T> operator^(const TVector<T> &V)  const noexcept
        {
            return TVector<T>(
                Y * V.Z - Z * V.Y,
                Z * V.X - X * V.Z,
                X * V.Y - Y * V.X);
        }

        constexpr TVector operator-()  const noexcept// 移除多余的template <typename T>
        {
            return TVector(-X, -Y, -Z); // 使用当前类的T类型
        }

        constexpr TVector<T> &operator+=(const TVector<T> &V) noexcept
        {
            X += V.X;
            Y += V.Y;
            Z += V.Z;
            return *this;
        }

        constexpr TVector<T> &operator-=(const TVector<T> &V) noexcept
        {
            X -= V.X;
            Y -= V.Y;
            Z -= V.Z;
            return *this;
        }

        constexpr TVector<T> &operator*=(const TVector<T> &V) noexcept
        {
            X *= V.X;
            Y *= V.Y;
            Z *= V.Z;
            return *this;
        }

        constexpr TVector<T> &operator/=(const TVector<T> &V) noexcept
        {
            X /= V.X;
            Y /= V.Y;
            Z /= V.Z;
            return *this;
        }

        template <MathPoint U>
        TVector<T> operator+(U value)  const noexcept
        {
            return TVector<T>(X + (T)value, Y + (T)value, Z + (T)value);
        }

        template <MathPoint U>
        TVector<T> operator-(U value)  const noexcept
        {
            return TVector<T>(X - (T)value, Y - (T)value, Z - (T)value);
        }

        template <MathPoint U>
        TVector<T> operator*(U value) const noexcept
        {
            return TVector<T>(X * (T)value, Y * (T)value, Z * (T)value);
        }

        template <MathPoint U>
        TVector<T> operator/(U value)   const noexcept
        {
            const T RScale = T(1) / value;
            return TVector<T>(X * RScale, Y * RScale, Z * RScale);
        }

        template <MathPoint U>
        TVector<T> &operator+=(U value) noexcept
        {
            X += value;
            Y += value;
            Z += value;
            return *this;
        }

        template <MathPoint U>
        TVector<T> &operator-=(U value) noexcept
        {
            X -= value;
            Y -= value;
            Z -= value;
            return *this;
        }

        template <MathPoint U>
        TVector<T> &operator*=(U value) noexcept
        {
            X *= value;
            Y *= value;
            Z *= value;
            return *this;
        }

        template <MathPoint U>
        TVector<T> &operator/=(U value) noexcept
        {
            const T RScale = T(1) / value;
            X *= RScale;
            Y *= RScale;
            Z *= RScale;
            return *this;
        }

        constexpr bool operator==(const TVector<T> &V) const  noexcept
        {
            return X == V.X && Y == V.Y && Z == V.Z;
        }

        constexpr bool operator!=(const TVector<T> &V) const  noexcept
        {
            return X != V.X || Y != V.Y || Z != V.Z;
        }

        constexpr T operator|(const TVector<T> &V) const noexcept
        {
            return X * V.X + Y * V.Y + Z * V.Z;
        }

        constexpr TVector<T> Cross(const TVector<T> &V) const noexcept
        {
            return *this ^ V;
        }

        constexpr T Dot(const TVector<T> &V) const noexcept
        {
            return *this | V;
        }

        bool Equals(const TVector<T> &V, T Tolerance) const
        {
            return Abs(X - V.X) <= Tolerance &&
                   Abs(Y - V.Y) <= Tolerance &&
                   Abs(Z - V.Z) <= Tolerance;
        }

        constexpr static TVector<T> CrossProduct(const TVector<T> &A, const TVector<T> &B) noexcept
        {
            return A ^ B;
        }

        inline bool Normalize(T Tolerance = SMALL_NUMBER) noexcept
        {
            T SquareSum = X * X + Y * Y + Z * Z;
            if (SquareSum > Tolerance)
            {
                T Scale = InvSqrt(SquareSum);
                X *= Scale;
                Y *= Scale;
                Z *= Scale;
                return true;
            }

            return false;
        }

    public:
        constexpr void set(T _x, T _y, T _z) noexcept
        {
            X = _x;
            Y = _y;
            Z = _z;
        }

        T getMax()  const noexcept { return Max(Max(X, Y), Z); }

        T getAbsMax()  const  noexcept{ return Max(Max(Abs(X),Abs(Y)), Abs(Z)); }
    };



    using FVector = TVector<int>;
    using FVector3f = TVector<float>;
    using FVector3d = TVector<double>;

}

