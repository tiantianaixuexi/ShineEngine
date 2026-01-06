#pragma once



#include "math/mathDef.h"
#include "math/vector.ixx"
#include "math/matrix.ixx"


namespace shine::math
{
    template<FloatingPoint T>
    class Matrix4;

    // Basic math functions moved to mathDef.h to avoid circular dependency



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

