#pragma once

#include <memory>
#include "mathDef.h"
#include "mathUtil.h"

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

        // C++23 特性：显式对象参数 (Explicit Object Parameter) - CWG 2586
        //constexpr bool operator==(this TVector<T> const& self, TVector<T> const& V) const noexcept = default;
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

        [[nodiscard]] constexpr T getMax() const noexcept { return Max(Max(X, Y), Z); }

        [[nodiscard]] constexpr T getAbsMax() const noexcept { return Max(Max(Abs(X), Abs(Y)), Abs(Z)); }

        [[nodiscard]] constexpr T getMin() const noexcept { return Min(Min(X, Y), Z); }

        [[nodiscard]] constexpr T getAbsMin() const noexcept { return Min(Min(Abs(X), Abs(Y)), Abs(Z)); }

        // 长度平方（避免开方，性能更好）
        [[nodiscard]] constexpr T LengthSquared() const noexcept
        {
            return X * X + Y * Y + Z * Z;
        }

        // 长度
        [[nodiscard]] T Length() const noexcept
        {
            return Sqrt(LengthSquared());
        }

        // 获取归一化向量（不修改自身）
        [[nodiscard]] TVector<T> GetNormalized(T Tolerance = SMALL_NUMBER) const noexcept
        {
            TVector<T> result(*this);
            result.Normalize(Tolerance);
            return result;
        }

        // 距离
        [[nodiscard]] static T Distance(const TVector<T>& V1, const TVector<T>& V2) noexcept
        {
            return (V2 - V1).Length();
        }

        // 距离平方
        [[nodiscard]] static constexpr T DistanceSquared(const TVector<T>& V1, const TVector<T>& V2) noexcept
        {
            return (V2 - V1).LengthSquared();
        }

        // 线性插值
        [[nodiscard]] static constexpr TVector<T> Lerp(const TVector<T>& A, const TVector<T>& B, T Alpha) noexcept
        {
            Alpha = Clamp(Alpha, static_cast<T>(0), static_cast<T>(1));
            return A + (B - A) * Alpha;
        }

        // 球面线性插值（用于方向向量）
        [[nodiscard]] static TVector<T> Slerp(const TVector<T>& A, const TVector<T>& B, T Alpha) noexcept
        {
            Alpha = Clamp(Alpha, static_cast<T>(0), static_cast<T>(1));
            T dot = A.Dot(B);
            dot = Clamp(dot, static_cast<T>(-1), static_cast<T>(1));
            
            T theta = std::acos(dot);
            if (Abs(theta) < SMALL_NUMBER) {
                return Lerp(A, B, Alpha);
            }
            
            T sinTheta = std::sin(theta);
            T w1 = std::sin((static_cast<T>(1) - Alpha) * theta) / sinTheta;
            T w2 = std::sin(Alpha * theta) / sinTheta;
            
            return A * w1 + B * w2;
        }

        // 反射向量
        [[nodiscard]] TVector<T> Reflect(const TVector<T>& Normal) const noexcept
        {
            return *this - Normal * (static_cast<T>(2) * Dot(Normal));
        }

        // 投影向量（投影到另一个向量上）
        [[nodiscard]] TVector<T> Project(const TVector<T>& Target) const noexcept
        {
            T targetLengthSq = Target.LengthSquared();
            if (targetLengthSq < SMALL_NUMBER) {
                return TVector<T>::Zero();
            }
            return Target * (Dot(Target) / targetLengthSq);
        }

        // 拒绝向量（垂直于另一个向量）
        [[nodiscard]] TVector<T> Reject(const TVector<T>& Target) const noexcept
        {
            return *this - Project(Target);
        }

        // 计算两个向量之间的角度（弧度）
        [[nodiscard]] static T Angle(const TVector<T>& A, const TVector<T>& B) noexcept
        {
            T dot = A.Dot(B);
            T lenA = A.Length();
            T lenB = B.Length();
            
            if (lenA < SMALL_NUMBER || lenB < SMALL_NUMBER) {
                return static_cast<T>(0);
            }
            
            dot = Clamp(dot / (lenA * lenB), static_cast<T>(-1), static_cast<T>(1));
            return std::acos(dot);
        }

        // 计算两个向量之间的角度（度）
        [[nodiscard]] static T AngleDegrees(const TVector<T>& A, const TVector<T>& B) noexcept
        {
            return Angle(A, B) * static_cast<T>(57.295779513082320876); // 180/PI
        }

        // 限制向量长度
        [[nodiscard]] TVector<T> ClampLength(T MaxLength) const noexcept
        {
            T lengthSq = LengthSquared();
            if (lengthSq <= MaxLength * MaxLength) {
                return *this;
            }
            return GetNormalized() * MaxLength;
        }

        // 限制向量长度（最小和最大）
        [[nodiscard]] TVector<T> ClampLength(T MinLength, T MaxLength) const noexcept
        {
            T length = Length();
            if (length < MinLength) {
                return GetNormalized() * MinLength;
            }
            if (length > MaxLength) {
                return GetNormalized() * MaxLength;
            }
            return *this;
        }

        // 检查是否接近零向量
        [[nodiscard]] constexpr bool IsNearlyZero(T Tolerance = SMALL_NUMBER) const noexcept
        {
            return Abs(X) <= Tolerance && Abs(Y) <= Tolerance && Abs(Z) <= Tolerance;
        }

        // 检查是否为单位向量
        [[nodiscard]] bool IsUnit(T Tolerance = KINDA_SMALL_NUMBER) const noexcept
        {
            return Abs(LengthSquared() - static_cast<T>(1)) <= Tolerance;
        }

        // 数组访问操作符
        [[nodiscard]] constexpr T& operator[](int Index) noexcept
        {
            return (Index == 0) ? X : (Index == 1) ? Y : Z;
        }

        [[nodiscard]] constexpr const T& operator[](int Index) const noexcept
        {
            return (Index == 0) ? X : (Index == 1) ? Y : Z;
        }
    };



    using FVector = TVector<int>;
    using FVector3f = TVector<float>;
    using FVector3d = TVector<double>;

}

