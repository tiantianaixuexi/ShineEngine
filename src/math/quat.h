#pragma once

#include <cmath>
#include <array>


#include "math/rotator.h"
#include "math/mathDef.h"



namespace shine::math
{

    template<FloatingPoint T>
    class TQuat
    {
    public:

        T w{}, x{}, y{}, z{};

        constexpr TQuat() = default;
        constexpr TQuat(T w, T x, T y, T z) : w(w), x(x), y(y), z(z) {}
        constexpr TQuat(T w, std::array<T, 3> v) : w(w), x(v[0]), y(v[1]), z(v[2]) {}

        // 拷贝/移动构造函数
        constexpr TQuat(const TQuat&) = default;
        constexpr TQuat(TQuat&&) = default;
        constexpr TQuat& operator=(const TQuat&) = default;
        constexpr TQuat& operator=(TQuat&&) = default;

        // 加法
        constexpr TQuat operator+(const TQuat& rhs) const noexcept {
            return TQuat(w + rhs.w, x + rhs.x, y + rhs.y, z + rhs.z);
        }

        // 减法
        constexpr TQuat operator-(const TQuat& rhs) const noexcept  {
            return TQuat(w - rhs.w, x - rhs.x, y - rhs.y, z - rhs.z);
        }

        // 乘法
        constexpr TQuat operator*(const TQuat& rhs) const noexcept  {
            return TQuat(
                w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
                w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
                w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
                w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w
            );
        }

        // 标量乘法
        constexpr TQuat operator*(T scalar) const  noexcept {
            return TQuat(w * scalar, x * scalar, y * scalar, z * scalar);
        }

        // 标量除法
        constexpr TQuat operator/(T scalar) const noexcept  {
            return TQuat(w / scalar, x / scalar, y / scalar, z / scalar);
        }

        // 共轭
        constexpr TQuat conjugate() const noexcept {
            return TQuat(w, -x, -y, -z);
        }

        // 模长
        constexpr T norm() const  noexcept {
            return std::sqrt(w * w + x * x + y * y + z * z);
        }

        // 归一化
        constexpr TQuat<T> normalized() const noexcept;

        // 取反
        constexpr TQuat operator-() const noexcept {
            return TQuat(-w, -x, -y, -z);
        }

        // 等于
        constexpr bool operator==(const TQuat& rhs) const = default;

        // 四元数转欧拉角（roll, pitch, yaw），单位：弧度
        constexpr TRotator<T> eulerAngles() const noexcept;

        // 用Rotator(角度为单位) 构造四元数
        static constexpr TQuat fromRotatorDegrees(const TRotator<T>& rotDeg) noexcept;
        // 从四元数生成 Rotator(角度为单位)
        constexpr TRotator<T> toRotatorDegrees() const noexcept;

        // 单位四元数
        static consteval TQuat identity() noexcept {
            return TQuat{T(1), T(0), T(0), T(0)};
        }

        // 零四元数
        static consteval TQuat zero() noexcept {
            return TQuat{T(0), T(0), T(0), T(0)};
        }

        // 从轴角构造
        static constexpr TQuat fromAxisAngle(const std::array<T, 3>& axis, T angle) noexcept
        {
            T half = angle / T(2);
            T s = std::sin(half);
            return TQuat(std::cos(half), axis[0] * s, axis[1] * s, axis[2] * s);
        }

        // 从欧拉角构造（roll, pitch, yaw）
        static constexpr TQuat fromEulerAngles(const std::array<T, 3>& euler) noexcept;

        // 点积（内积）
        constexpr T dot(const TQuat& rhs) const noexcept {
            return w * rhs.w + x * rhs.x + y * rhs.y + z * rhs.z;
        }

        // 球面线性插值（slerp）
        static constexpr TQuat slerp(const TQuat& a, const TQuat& b, T t) noexcept {
            T d = a.dot(b);
            TQuat b2 = b;
            if (d < T(0)) { d = -d; b2 = -b; }
            if (d > T(0.9995)) {
                // 线性插值
                return (a * (T(1) - t) + b2 * t).normalized();
            }
            T theta = std::acos(d);
            T s0 = std::sin((T(1) - t) * theta);
            T s1 = std::sin(t * theta);
            T s = std::sin(theta);
            return (a * (s0 / s) + b2 * (s1 / s)).normalized();
        }

        // 线性插值（lerp）
        static constexpr TQuat lerp(const TQuat& a, const TQuat& b, T t) noexcept {
            return (a * (T(1) - t) + b * t).normalized();
        }

        // 旋转向量
        constexpr std::array<T, 3> rotate(const std::array<T, 3>& v) const noexcept {
            TQuat<T> qv{T(0), v[0], v[1], v[2]};
            TQuat<T> res = (*this) * qv * this->conjugate();
            return {res.x, res.y, res.z};
        }

        // 是否为单位四元数
        constexpr bool isUnit(T eps = T(1e-6)) const noexcept {
            return std::abs(norm() - T(1)) < eps;
        }

        // 逆
        constexpr TQuat inverse() const noexcept {
            T n2 = w * w + x * x + y * y + z * z;
            if (n2 == T(0)) return *this;
            return conjugate() / n2;
        }
    };


    using FQuatf = TQuat<float>;
    using FQuatd = TQuat<double>;
}

