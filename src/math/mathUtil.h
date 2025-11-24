#pragma once



#include "math/mathDef.h"
#include "math/vector.h"
#include "math/matrix.h"


namespace shine::math
{

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


    // 辅助函数，角度转弧度
    constexpr float radians(float degrees)  noexcept{    return degrees * 0.01745329251f; };


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

