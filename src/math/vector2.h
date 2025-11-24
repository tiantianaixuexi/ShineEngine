#pragma once

#include <string>



#include "math/mathDef.h"

namespace shine::math
{
    template<MathPoint T>
    class vector2
    {

    public:
        T X;
        T Y;

      
        static constexpr vector2<T> Zero() { return vector2<T>(0, 0); }
        static constexpr vector2<T> One() { return vector2<T>(1, 1); }
        static constexpr vector2<T> UnitX() { return vector2<T>(1, 0); }
        static constexpr vector2<T> UnitY() { return vector2<T>(0, 1); }
        static constexpr vector2<T> Left() { return vector2<T>(-1, 0); }
        static constexpr vector2<T> Right() { return vector2<T>(1, 0); }
        static constexpr vector2<T> Up() { return vector2<T>(0, 1); }
        static constexpr vector2<T> Down() { return vector2<T>(0, -1); }

      
        constexpr vector2() noexcept : X(0), Y(0) {}
        constexpr vector2(T scalar) noexcept : X(scalar), Y(scalar) {}
        constexpr vector2(T x, T y) noexcept : X(x), Y(y) {}


        constexpr vector2(const vector2<T>& other) noexcept = default;
        constexpr vector2<T>& operator=(const vector2<T>& other) noexcept = default;
        constexpr vector2(vector2<T>&& other) noexcept = default;
        constexpr vector2<T>& operator=(vector2<T>&& other) noexcept = default;

       
        [[nodiscard]] constexpr vector2<T> operator+(const vector2<T>& v) const noexcept
        {
            return vector2<T>(X + v.X, Y + v.Y);
        }

        [[nodiscard]] constexpr vector2<T> operator-(const vector2<T>& v) const noexcept
        {
            return vector2<T>(X - v.X, Y - v.Y);
        }

        [[nodiscard]] constexpr vector2<T> operator*(const vector2<T>& v) const noexcept
        {
            return vector2<T>(X * v.X, Y * v.Y);
        }

        [[nodiscard]] constexpr vector2<T> operator/(const vector2<T>& v) const noexcept
        {
            return vector2<T>(X / v.X, Y / v.Y);
        }

        [[nodiscard]] constexpr vector2<T> operator-() const noexcept
        {
            return vector2<T>(-X, -Y);
        }

        constexpr vector2<T>& operator+=(const vector2<T>& v) noexcept
        {
            X += v.X;
            Y += v.Y;
            return *this;
        }

        constexpr vector2<T>& operator-=(const vector2<T>& v) noexcept
        {
            X -= v.X;
            Y -= v.Y;
            return *this;
        }

        constexpr vector2<T>& operator*=(const vector2<T>& v) noexcept
        {
            X *= v.X;
            Y *= v.Y;
            return *this;
        }

        constexpr vector2<T>& operator/=(const vector2<T>& v) noexcept
        {
            X /= v.X;
            Y /= v.Y;
            return *this;
        }

    
        template <MathPoint U>
        [[nodiscard]] constexpr vector2<T> operator+(U value) const noexcept
        {
            return vector2<T>(X + static_cast<T>(value), Y + static_cast<T>(value));
        }

        template <MathPoint U>
        [[nodiscard]] constexpr vector2<T> operator-(U value) const noexcept
        {
            return vector2<T>(X - static_cast<T>(value), Y - static_cast<T>(value));
        }

        template <MathPoint U>
        [[nodiscard]] constexpr vector2<T> operator*(U value) const noexcept
        {
            return vector2<T>(X * static_cast<T>(value), Y * static_cast<T>(value));
        }

        template <MathPoint U>
        [[nodiscard]] constexpr vector2<T> operator/(U value) const noexcept
        {
            const T rScale = static_cast<T>(1) / static_cast<T>(value);
            return vector2<T>(X * rScale, Y * rScale);
        }

        template <MathPoint U>
        constexpr vector2<T>& operator+=(U value) noexcept
        {
            X += static_cast<T>(value);
            Y += static_cast<T>(value);
            return *this;
        }

        template <MathPoint U>
        constexpr vector2<T>& operator-=(U value) noexcept
        {
            X -= static_cast<T>(value);
            Y -= static_cast<T>(value);
            return *this;
        }

        template <MathPoint U>
        constexpr vector2<T>& operator*=(U value) noexcept
        {
            X *= static_cast<T>(value);
            Y *= static_cast<T>(value);
            return *this;
        }

        template <MathPoint U>
        constexpr vector2<T>& operator/=(U value) noexcept
        {
            const T rScale = static_cast<T>(1) / static_cast<T>(value);
            X *= rScale;
            Y *= rScale;
            return *this;
        }

       
        [[nodiscard]] constexpr bool operator==(const vector2<T>& v) const noexcept
        {
            return X == v.X && Y == v.Y;
        }

        [[nodiscard]] constexpr bool operator!=(const vector2<T>& v) const noexcept
        {
            return X != v.X || Y != v.Y;
        }

        // 鐐圭Н
        [[nodiscard]] constexpr T operator|(const vector2<T>& v) const noexcept
        {
            return X * v.X + Y * v.Y;
        }

        [[nodiscard]] constexpr T Dot(const vector2<T>& v) const noexcept
        {
            return *this | v;
        }

        
        [[nodiscard]] constexpr T Cross(const vector2<T>& v) const noexcept
        {
            return X * v.Y - Y * v.X;
        }

        [[nodiscard]] static constexpr T CrossProduct(const vector2<T>& a, const vector2<T>& b) noexcept
        {
            return a.Cross(b);
        }

 
        [[nodiscard]] bool Equals(const vector2<T>& v, T tolerance) const noexcept
        {
            return Abs(X - v.X) <= tolerance && Abs(Y - v.Y) <= tolerance;
        }

   
        [[nodiscard]] constexpr T LengthSquared() const noexcept
        {
            return X * X + Y * Y;
        }

        [[nodiscard]] T Length() const noexcept
        {
            return Sqrt(LengthSquared());
        }

       
        bool Normalize(T tolerance = SMALL_NUMBER) noexcept
        {
            const T squareSum = LengthSquared();
            if (squareSum > tolerance)
            {
                const T scale = InvSqrt(squareSum);
                X *= scale;
                Y *= scale;
                return true;
            }
            return false;
        }

        [[nodiscard]] vector2<T> GetNormalized(T tolerance = SMALL_NUMBER) const noexcept
        {
            vector2<T> result(*this);
            result.Normalize(tolerance);
            return result;
        }

        // 实用函数
        constexpr void Set(T x, T y) noexcept
        {
            X = x;
            Y = y;
        }

        [[nodiscard]] constexpr T GetMax() const noexcept
        {
            return Max(X, Y);
        }

        [[nodiscard]] constexpr T GetAbsMax() const noexcept
        {
            return Max(Abs(X), Abs(Y));
        }

        [[nodiscard]] constexpr T GetMin() const noexcept
        {
            return Min(X, Y);
        }

        [[nodiscard]] constexpr T GetAbsMin() const noexcept
        {
            return Min(Abs(X), Abs(Y));
        }

   
        [[nodiscard]] static T Distance(const vector2<T>& v1, const vector2<T>& v2) noexcept
        {
            return (v2 - v1).Length();
        }

        [[nodiscard]] static constexpr T DistanceSquared(const vector2<T>& v1, const vector2<T>& v2) noexcept
        {
            return (v2 - v1).LengthSquared();
        }

      
        [[nodiscard]] static constexpr vector2<T> Lerp(const vector2<T>& a, const vector2<T>& b, T t) noexcept
        {
            t = Clamp(t, static_cast<T>(0), static_cast<T>(1));
            return a + (b - a) * t;
        }

     
        [[nodiscard]] constexpr vector2<T> GetPerpendicular() const noexcept
        {
            return vector2<T>(-Y, X);
        }

      
        [[nodiscard]] vector2<T> Reflect(const vector2<T>& normal) const noexcept
        {
            return *this - normal * (static_cast<T>(2) * Dot(normal));
        }

     
        [[nodiscard]] constexpr T& operator[](int index) noexcept
        {
            return (index == 0) ? X : Y;
        }

        [[nodiscard]] constexpr const T& operator[](int index) const noexcept
        {
            return (index == 0) ? X : Y;
        }

      
        [[nodiscard]] std::string ToString() const
        {
            return "(" + std::to_string(X) + ", " + std::to_string(Y) + ")";
        }
    };

    using FVector2 = vector2<int>;
    using FVector2f = vector2<float>;
    using FVector2d = vector2<double>;
};

