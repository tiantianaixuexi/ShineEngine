#pragma once

#ifndef SHINE_PLATFORM_WASM
#include <string>
#endif

#include "math/mathDef.h"
#include "math/mathUtil.h"



namespace shine::math
{
    template<MathPoint T>
    class vector2
    {

    public:
        T X;
        T Y;

        
#ifndef SHINE_PLATFORM_WASM

        [[nodiscard]] std::string ToString() const
        {
            return "(" + std::to_string(X) + ", " + std::to_string(Y) + ")";
        }

#endif

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


        // 限制向量长度
        [[nodiscard]] vector2<T> ClampLength(T maxLength) const noexcept
        {
            T lengthSq = LengthSquared();
            if (lengthSq <= maxLength * maxLength) {
                return *this;
            }
            return GetNormalized() * maxLength;
        }

        // 限制向量长度（最小和最大）
        [[nodiscard]] vector2<T> ClampLength(T minLength, T maxLength) const noexcept
        {
            T length = Length();
            if (length < minLength) {
                return GetNormalized() * minLength;
            }
            if (length > maxLength) {
                return GetNormalized() * maxLength;
            }
            return *this;
        }

        // 检查是否接近零向量
        [[nodiscard]] constexpr bool IsNearlyZero(T tolerance = SMALL_NUMBER) const noexcept
        {
            return Abs(X) <= tolerance && Abs(Y) <= tolerance;
        }

        // 检查是否为单位向量
        [[nodiscard]] bool IsUnit(T tolerance = KINDA_SMALL_NUMBER) const noexcept
        {
            return Abs(LengthSquared() - static_cast<T>(1)) <= tolerance;
        }

        // 计算两个向量之间的角度（弧度）
        [[nodiscard]] static T Angle(const vector2<T>& a, const vector2<T>& b) noexcept
        {
            T dot = a.Dot(b);
            T lenA = a.Length();
            T lenB = b.Length();
            
            if (lenA < SMALL_NUMBER || lenB < SMALL_NUMBER) {
                return static_cast<T>(0);
            }
            
            dot = Clamp(dot / (lenA * lenB), static_cast<T>(-1), static_cast<T>(1));
            return std::acos(dot);
        }

        // 计算两个向量之间的角度（度）
        [[nodiscard]] static T AngleDegrees(const vector2<T>& a, const vector2<T>& b) noexcept
        {
            return Angle(a, b) * static_cast<T>(57.295779513082320876); // 180/PI
        }

        // 投影向量（投影到另一个向量上）
        [[nodiscard]] vector2<T> Project(const vector2<T>& target) const noexcept
        {
            T targetLengthSq = target.LengthSquared();
            if (targetLengthSq < SMALL_NUMBER) {
                return vector2<T>::Zero();
            }
            return target * (Dot(target) / targetLengthSq);
        }

        // 拒绝向量（垂直于另一个向量）
        [[nodiscard]] vector2<T> Reject(const vector2<T>& target) const noexcept
        {
            return *this - Project(target);
        }

        // 旋转向量（逆时针旋转指定角度，弧度）
        [[nodiscard]] vector2<T> Rotate(T angleRad) const noexcept
        {
            T cosA = std::cos(angleRad);
            T sinA = std::sin(angleRad);
            return vector2<T>(
                X * cosA - Y * sinA,
                X * sinA + Y * cosA
            );
        }

        // 旋转向量（逆时针旋转指定角度，度）
        [[nodiscard]] vector2<T> RotateDegrees(T angleDeg) const noexcept
        {
            return Rotate(angleDeg * static_cast<T>(0.017453292519943295769)); // PI/180
        }

        // 获取角度（相对于X轴，弧度）
        [[nodiscard]] T GetAngle() const noexcept
        {
            return std::atan2(Y, X);
        }

        // 获取角度（相对于X轴，度）
        [[nodiscard]] T GetAngleDegrees() const noexcept
        {
            return GetAngle() * static_cast<T>(57.295779513082320876);
        }

        // 从角度创建向量（弧度）
        [[nodiscard]] static vector2<T> FromAngle(T angleRad) noexcept
        {
            return vector2<T>(std::cos(angleRad), std::sin(angleRad));
        }

        // 从角度创建向量（度）
        [[nodiscard]] static vector2<T> FromAngleDegrees(T angleDeg) noexcept
        {
            return FromAngle(angleDeg * static_cast<T>(0.017453292519943295769));
        }
    };

    using FVector2 = vector2<int>;
    using FVector2f = vector2<float>;
    using FVector2d = vector2<double>;
};

