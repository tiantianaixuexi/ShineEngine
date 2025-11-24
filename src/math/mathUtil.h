#pragma once



#include "math/mathDef.h"
#include "math/vector.h"
#include "math/matrix.h"


namespace shine::math
{
    template<FloatingPoint T>
    class Matrix4;

    template<class T>
    T Abs(const T& A)
    {
        return ( A < (T)0 ) ?  -A : A;
    }

    template<class T>
    constexpr T Max(const T &A , const T &B)
    {
        return A > B ? A : B;
    }

    template<class T>
    constexpr T Min(const T &A , const T &B)
    {
        return A < B ? A : B;
    }

    template<class T>
    constexpr T Clamp(const T &A , const T &Min , const T &Max)
    {
        return A < Min ? Min : ( A > Max ? Max : A );
    }

    template<class T>
    constexpr T Sign(const T &A)
    {
        return ( A > static_cast<T>(0) ) ? static_cast<T>(1) : ( A < static_cast<T>(0) ? static_cast<T>(-1) : static_cast<T>(0) );
    }

    inline float Fmod(float A, float B)
    {
        const float absY = Abs(B);
        if (absY < SMALL_NUMBER) {
            return 0.0f; // 避免除以零
        }
        return fmodf(A, B);
    }

    inline double Fmod(double A, double B){
        const double absY = Abs(B);
        if (absY < DOUBLE_KINDA_SMALL_NUMBER) {
            return 0.0; // 避免除以零
        }
        return fmod(A, B);
    }


    // 角度转弧度
    [[nodiscard]] constexpr float radians(float degrees) noexcept {
        return degrees * 0.01745329251f;
    }

    [[nodiscard]] constexpr double radians(double degrees) noexcept {
        return degrees * 0.017453292519943295769;
    }

    // 弧度转角度
    [[nodiscard]] constexpr float degrees(float radians) noexcept {
        return radians * 57.295779513082320876f;
    }

    [[nodiscard]] constexpr double degrees(double radians) noexcept {
        return radians * 57.295779513082320876;
    }

    // 线性插值
    template<FloatingPoint T>
    [[nodiscard]] constexpr T lerp(T a, T b, T t) noexcept {
        t = Clamp(t, static_cast<T>(0), static_cast<T>(1));
        return a + (b - a) * t;
    }

    // 平滑插值（smoothstep）
    template<FloatingPoint T>
    [[nodiscard]] constexpr T smoothstep(T edge0, T edge1, T x) noexcept {
        x = Clamp((x - edge0) / (edge1 - edge0), static_cast<T>(0), static_cast<T>(1));
        return x * x * (static_cast<T>(3) - static_cast<T>(2) * x);
    }

    // 更平滑的插值（smootherstep）
    template<FloatingPoint T>
    [[nodiscard]] constexpr T smootherstep(T edge0, T edge1, T x) noexcept {
        x = Clamp((x - edge0) / (edge1 - edge0), static_cast<T>(0), static_cast<T>(1));
        return x * x * x * (x * (x * static_cast<T>(6) - static_cast<T>(15)) + static_cast<T>(10));
    }

    // 重映射值
    template<FloatingPoint T>
    [[nodiscard]] constexpr T remap(T value, T inMin, T inMax, T outMin, T outMax) noexcept {
        return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
    }

    // 检查值是否在范围内
    template<Arithmetic T>
    [[nodiscard]] constexpr bool isInRange(T value, T min, T max) noexcept {
        return value >= min && value <= max;
    }

    // 将角度限制在 [0, 360) 度
    template<FloatingPoint T>
    [[nodiscard]] constexpr T wrapAngleDegrees(T angle) noexcept {
        angle = Fmod(angle, static_cast<T>(360));
        if (angle < static_cast<T>(0)) {
            angle += static_cast<T>(360);
        }
        return angle;
    }

    // 将角度限制在 [-180, 180) 度
    template<FloatingPoint T>
    [[nodiscard]] constexpr T normalizeAngleDegrees(T angle) noexcept {
        angle = wrapAngleDegrees(angle);
        if (angle > static_cast<T>(180)) {
            angle -= static_cast<T>(360);
        }
        return angle;
    }

    // 将角度限制在 [0, 2π) 弧度
    template<FloatingPoint T>
    [[nodiscard]] constexpr T wrapAngleRadians(T angle) noexcept {
        constexpr T twoPi = constants::two_pi<T>;
        angle = Fmod(angle, twoPi);
        if (angle < static_cast<T>(0)) {
            angle += twoPi;
        }
        return angle;
    }

    // 将角度限制在 [-π, π) 弧度
    template<FloatingPoint T>
    [[nodiscard]] constexpr T normalizeAngleRadians(T angle) noexcept {
        constexpr T pi = constants::pi<T>;
        angle = wrapAngleRadians(angle);
        if (angle > pi) {
            angle -= constants::two_pi<T>;
        }
        return angle;
    }

    // 检查浮点数是否接近零
    template<FloatingPoint T>
    [[nodiscard]] constexpr bool isNearlyZero(T value, T tolerance = SMALL_NUMBER) noexcept {
        return Abs(value) <= tolerance;
    }

    // 检查两个浮点数是否接近相等
    template<FloatingPoint T>
    [[nodiscard]] constexpr bool isNearlyEqual(T a, T b, T tolerance = KINDA_SMALL_NUMBER) noexcept {
        return Abs(a - b) <= tolerance;
    }


    template<FloatingPoint T>
    constexpr  Matrix4<T> Perspective(float fovDeg, float aspect, float znear, float zfar) noexcept
    {
        Matrix4<T> m{}; // identity by default
        m.m_data.fill(0.0f);
        const float f = 1.0f / std::tan(math::radians(fovDeg) * 0.5f);
        m.m_data[0]  = f / aspect;
        m.m_data[5]  = f;
        m.m_data[10] = (zfar + znear) / (znear - zfar);
        m.m_data[11] = -1.0f;
        m.m_data[14] = (2.0f * zfar * znear) / (znear - zfar);
        // m[15] stays 0
        return m;
    }

    template<FloatingPoint T>
    constexpr  Matrix4<T> Ortho(float left, float right, float bottom, float top, float znear, float zfar) noexcept
    {
        Matrix4<T> m{}; m.m_data.fill(0.0f);
        const T rl = right - left;
        const T tb = top - bottom;
        const T fn = zfar - znear;
        m.m_data[0] = static_cast<T>(2) / rl;
        m.m_data[5] = static_cast<T>(2) / tb;
        m.m_data[10] = -static_cast<T>(2) / fn;
        m.m_data[12] = -(right + left) / rl;
        m.m_data[13] = -(top + bottom) / tb;
        m.m_data[14] = -(zfar + znear) / fn;
        m.m_data[15] = static_cast<T>(1);
        return m;
    }

    template<FloatingPoint T>
    constexpr  Matrix4<T> LookAt(const TVector<T>& eye, const TVector<T>& center, const TVector<T>& up) noexcept
    {
        TVector<T> f = TVector<T>(center.X - eye.X, center.Y - eye.Y, center.Z - eye.Z);
        f.Normalize();
        TVector<T> s = TVector<T>::CrossProduct(f, up);
        s.Normalize();
        TVector<T> u = TVector<T>::CrossProduct(s, f);

        Matrix4<T> m{}; m.m_data.fill(0.0f);
        // first column
        m.m_data[0] = s.X;
        m.m_data[1] = u.X;
        m.m_data[2] = -f.X;
        m.m_data[3] = 0.0f;
        // second column
        m.m_data[4] = s.Y;
        m.m_data[5] = u.Y;
        m.m_data[6] = -f.Y;
        m.m_data[7] = 0.0f;
        // third column
        m.m_data[8]  = s.Z;
        m.m_data[9]  = u.Z;
        m.m_data[10] = -f.Z;
        m.m_data[11] = 0.0f;
        // fourth column (translation)
        m.m_data[12] = -(s.X * eye.X + s.Y * eye.Y + s.Z * eye.Z);
        m.m_data[13] = -(u.X * eye.X + u.Y * eye.Y + u.Z * eye.Z);
        m.m_data[14] =  (f.X * eye.X + f.Y * eye.Y + f.Z * eye.Z);
        m.m_data[15] = 1.0f;
        return m;
    }

}

