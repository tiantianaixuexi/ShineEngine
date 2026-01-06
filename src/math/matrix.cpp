#ifdef SHINE_USE_MODULE

module;


module shine.math.matrix4;

#else

#include "matrix.ixx"
#include "math/mathDef.h"

#endif
namespace shine::math
{
    template<FloatingPoint T>
    Matrix4<T>::Matrix4()
    {
        m_data.fill(T(0));
        // identity
        m_data[0] = m_data[5] = m_data[10] = m_data[15] = T(1);
    }

    template<FloatingPoint T>
    Matrix4<T>::Matrix4(T diag)
    {
        m_data.fill(T(0));
        m_data[0] = m_data[5] = m_data[10] = m_data[15] = diag;
    }

    template<FloatingPoint T>
    Matrix4<T>::Matrix4(const std::array<T, 16>& values)
        : m_data(values)
    {}

    template<FloatingPoint T>
    T Matrix4<T>::get(int row, int col) const noexcept
    {
        return m_data[col * 4 + row];
    }

    template<FloatingPoint T>
    void Matrix4<T>::set(int row, int col, T v) noexcept
    {
        m_data[col * 4 + row] = v;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::operator*(const Matrix4<T>& rhs) const noexcept
    {
        Matrix4<T> out{}; out.m_data.fill(T(0));
     
        for (int col = 0; col < 4; ++col)
        {
            for (int row = 0; row < 4; ++row)
            {
                out.m_data[col * 4 + row] =
                    m_data[0 * 4 + row] * rhs.m_data[col * 4 + 0] +
                    m_data[1 * 4 + row] * rhs.m_data[col * 4 + 1] +
                    m_data[2 * 4 + row] * rhs.m_data[col * 4 + 2] +
                    m_data[3 * 4 + row] * rhs.m_data[col * 4 + 3];
            }
        }
        return out;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::transposed() const noexcept
    {
        Matrix4<T> t{}; t.m_data.fill(T(0));
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                t.m_data[c * 4 + r] = m_data[r * 4 + c];
        return t;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::operator+(const Matrix4<T>& rhs) const noexcept
    {
        Matrix4<T> result;
        for (int i = 0; i < 16; ++i) {
            result.m_data[i] = m_data[i] + rhs.m_data[i];
        }
        return result;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::operator-(const Matrix4<T>& rhs) const noexcept
    {
        Matrix4<T> result;
        for (int i = 0; i < 16; ++i) {
            result.m_data[i] = m_data[i] - rhs.m_data[i];
        }
        return result;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::operator*(T scalar) const noexcept
    {
        Matrix4<T> result;
        for (int i = 0; i < 16; ++i) {
            result.m_data[i] = m_data[i] * scalar;
        }
        return result;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::operator/(T scalar) const noexcept
    {
        T invScalar = T(1) / scalar;
        return *this * invScalar;
    }

    template<FloatingPoint T>
    Matrix4<T>& Matrix4<T>::operator*=(const Matrix4<T>& rhs) noexcept
    {
        *this = *this * rhs;
        return *this;
    }

    template<FloatingPoint T>
    Matrix4<T>& Matrix4<T>::operator+=(const Matrix4<T>& rhs) noexcept
    {
        for (int i = 0; i < 16; ++i) {
            m_data[i] += rhs.m_data[i];
        }
        return *this;
    }

    template<FloatingPoint T>
    Matrix4<T>& Matrix4<T>::operator-=(const Matrix4<T>& rhs) noexcept
    {
        for (int i = 0; i < 16; ++i) {
            m_data[i] -= rhs.m_data[i];
        }
        return *this;
    }

    template<FloatingPoint T>
    Matrix4<T>& Matrix4<T>::operator*=(T scalar) noexcept
    {
        for (int i = 0; i < 16; ++i) {
            m_data[i] *= scalar;
        }
        return *this;
    }

    template<FloatingPoint T>
    Matrix4<T>& Matrix4<T>::operator/=(T scalar) noexcept
    {
        T invScalar = T(1) / scalar;
        return *this *= invScalar;
    }

    template<FloatingPoint T>
    TVector<T> Matrix4<T>::transformVector(const TVector<T>& v) const noexcept
    {
        return TVector<T>(
            m_data[0] * v.X + m_data[4] * v.Y + m_data[8] * v.Z,
            m_data[1] * v.X + m_data[5] * v.Y + m_data[9] * v.Z,
            m_data[2] * v.X + m_data[6] * v.Y + m_data[10] * v.Z
        );
    }

    template<FloatingPoint T>
    TVector<T> Matrix4<T>::transformPoint(const TVector<T>& p) const noexcept
    {
        T w = m_data[3] * p.X + m_data[7] * p.Y + m_data[11] * p.Z + m_data[15];
        if (Abs(w) < SMALL_NUMBER) {
            w = T(1);
        }
        T invW = T(1) / w;
        return TVector<T>(
            (m_data[0] * p.X + m_data[4] * p.Y + m_data[8] * p.Z + m_data[12]) * invW,
            (m_data[1] * p.X + m_data[5] * p.Y + m_data[9] * p.Z + m_data[13]) * invW,
            (m_data[2] * p.X + m_data[6] * p.Y + m_data[10] * p.Z + m_data[14]) * invW
        );
    }

    template<FloatingPoint T>
    T Matrix4<T>::determinant() const noexcept
    {
        T a = m_data[0], b = m_data[4], c = m_data[8], d = m_data[12];
        T e = m_data[1], f = m_data[5], g = m_data[9], h = m_data[13];
        T i = m_data[2], j = m_data[6], k = m_data[10], l = m_data[14];
        T m = m_data[3], n = m_data[7], o = m_data[11], p = m_data[15];

        T det = a * (f * (k * p - l * o) - g * (j * p - l * n) + h * (j * o - k * n))
              - b * (e * (k * p - l * o) - g * (i * p - l * m) + h * (i * o - k * m))
              + c * (e * (j * p - l * n) - f * (i * p - l * m) + h * (i * n - j * m))
              - d * (e * (j * o - k * n) - f * (i * o - k * m) + g * (i * n - j * m));
        return det;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::inverse() const noexcept
    {
        T det = determinant();
        if (Abs(det) < SMALL_NUMBER) {
            return Matrix4<T>::identity();
        }

        T invDet = T(1) / det;
        Matrix4<T> result;

        result.m_data[0] = (m_data[5] * (m_data[10] * m_data[15] - m_data[11] * m_data[14]) -
                           m_data[6] * (m_data[9] * m_data[15] - m_data[11] * m_data[13]) +
                           m_data[7] * (m_data[9] * m_data[14] - m_data[10] * m_data[13])) * invDet;
        result.m_data[1] = -(m_data[1] * (m_data[10] * m_data[15] - m_data[11] * m_data[14]) -
                            m_data[2] * (m_data[9] * m_data[15] - m_data[11] * m_data[13]) +
                            m_data[3] * (m_data[9] * m_data[14] - m_data[10] * m_data[13])) * invDet;
        result.m_data[2] = (m_data[1] * (m_data[6] * m_data[15] - m_data[7] * m_data[14]) -
                           m_data[2] * (m_data[5] * m_data[15] - m_data[7] * m_data[13]) +
                           m_data[3] * (m_data[5] * m_data[14] - m_data[6] * m_data[13])) * invDet;
        result.m_data[3] = -(m_data[1] * (m_data[6] * m_data[11] - m_data[7] * m_data[10]) -
                            m_data[2] * (m_data[5] * m_data[11] - m_data[7] * m_data[9]) +
                            m_data[3] * (m_data[5] * m_data[10] - m_data[6] * m_data[9])) * invDet;

        result.m_data[4] = -(m_data[4] * (m_data[10] * m_data[15] - m_data[11] * m_data[14]) -
                            m_data[6] * (m_data[8] * m_data[15] - m_data[11] * m_data[12]) +
                            m_data[7] * (m_data[8] * m_data[14] - m_data[10] * m_data[12])) * invDet;
        result.m_data[5] = (m_data[0] * (m_data[10] * m_data[15] - m_data[11] * m_data[14]) -
                           m_data[2] * (m_data[8] * m_data[15] - m_data[11] * m_data[12]) +
                           m_data[3] * (m_data[8] * m_data[14] - m_data[10] * m_data[12])) * invDet;
        result.m_data[6] = -(m_data[0] * (m_data[6] * m_data[15] - m_data[7] * m_data[14]) -
                            m_data[2] * (m_data[4] * m_data[15] - m_data[7] * m_data[12]) +
                            m_data[3] * (m_data[4] * m_data[14] - m_data[6] * m_data[12])) * invDet;
        result.m_data[7] = (m_data[0] * (m_data[6] * m_data[11] - m_data[7] * m_data[10]) -
                           m_data[2] * (m_data[4] * m_data[11] - m_data[7] * m_data[8]) +
                           m_data[3] * (m_data[4] * m_data[10] - m_data[6] * m_data[8])) * invDet;

        result.m_data[8] = (m_data[4] * (m_data[9] * m_data[15] - m_data[11] * m_data[13]) -
                           m_data[5] * (m_data[8] * m_data[15] - m_data[11] * m_data[12]) +
                           m_data[7] * (m_data[8] * m_data[13] - m_data[9] * m_data[12])) * invDet;
        result.m_data[9] = -(m_data[0] * (m_data[9] * m_data[15] - m_data[11] * m_data[13]) -
                            m_data[1] * (m_data[8] * m_data[15] - m_data[11] * m_data[12]) +
                            m_data[3] * (m_data[8] * m_data[13] - m_data[9] * m_data[12])) * invDet;
        result.m_data[10] = (m_data[0] * (m_data[5] * m_data[15] - m_data[7] * m_data[13]) -
                            m_data[1] * (m_data[4] * m_data[15] - m_data[7] * m_data[12]) +
                            m_data[3] * (m_data[4] * m_data[13] - m_data[5] * m_data[12])) * invDet;
        result.m_data[11] = -(m_data[0] * (m_data[5] * m_data[11] - m_data[7] * m_data[9]) -
                             m_data[1] * (m_data[4] * m_data[11] - m_data[7] * m_data[8]) +
                             m_data[3] * (m_data[4] * m_data[9] - m_data[5] * m_data[8])) * invDet;

        result.m_data[12] = -(m_data[4] * (m_data[9] * m_data[14] - m_data[10] * m_data[13]) -
                             m_data[5] * (m_data[8] * m_data[14] - m_data[10] * m_data[12]) +
                             m_data[6] * (m_data[8] * m_data[13] - m_data[9] * m_data[12])) * invDet;
        result.m_data[13] = (m_data[0] * (m_data[9] * m_data[14] - m_data[10] * m_data[13]) -
                            m_data[1] * (m_data[8] * m_data[14] - m_data[10] * m_data[12]) +
                            m_data[2] * (m_data[8] * m_data[13] - m_data[9] * m_data[12])) * invDet;
        result.m_data[14] = -(m_data[0] * (m_data[5] * m_data[14] - m_data[6] * m_data[13]) -
                             m_data[1] * (m_data[4] * m_data[14] - m_data[6] * m_data[12]) +
                             m_data[2] * (m_data[4] * m_data[13] - m_data[5] * m_data[12])) * invDet;
        result.m_data[15] = (m_data[0] * (m_data[5] * m_data[10] - m_data[6] * m_data[9]) -
                            m_data[1] * (m_data[4] * m_data[10] - m_data[6] * m_data[8]) +
                            m_data[2] * (m_data[4] * m_data[9] - m_data[5] * m_data[8])) * invDet;

        return result;
    }

    template<FloatingPoint T>
    constexpr Matrix4<T> Matrix4<T>::identity() noexcept
    {
        Matrix4<T> m;
        m.m_data.fill(T(0));
        m.m_data[0] = m.m_data[5] = m.m_data[10] = m.m_data[15] = T(1);
        return m;
    }

    template<FloatingPoint T>
    constexpr Matrix4<T> Matrix4<T>::zero() noexcept
    {
        Matrix4<T> m;
        m.m_data.fill(T(0));
        return m;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::translate(const TVector<T>& translation) noexcept
    {
        Matrix4<T> m = identity();
        m.m_data[12] = translation.X;
        m.m_data[13] = translation.Y;
        m.m_data[14] = translation.Z;
        return m;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::rotate(const TQuat<T>& rotation) noexcept
    {
        T w = rotation.w, x = rotation.x, y = rotation.y, z = rotation.z;
        Matrix4<T> m = identity();

        m.m_data[0] = T(1) - T(2) * (y * y + z * z);
        m.m_data[1] = T(2) * (x * y + w * z);
        m.m_data[2] = T(2) * (x * z - w * y);

        m.m_data[4] = T(2) * (x * y - w * z);
        m.m_data[5] = T(1) - T(2) * (x * x + z * z);
        m.m_data[6] = T(2) * (y * z + w * x);

        m.m_data[8] = T(2) * (x * z + w * y);
        m.m_data[9] = T(2) * (y * z - w * x);
        m.m_data[10] = T(1) - T(2) * (x * x + y * y);

        return m;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::rotateX(T angleRad) noexcept
    {
        T c = std::cos(angleRad);
        T s = std::sin(angleRad);
        Matrix4<T> m = identity();
        m.m_data[5] = c;
        m.m_data[6] = s;
        m.m_data[9] = -s;
        m.m_data[10] = c;
        return m;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::rotateY(T angleRad) noexcept
    {
        T c = std::cos(angleRad);
        T s = std::sin(angleRad);
        Matrix4<T> m = identity();
        m.m_data[0] = c;
        m.m_data[2] = -s;
        m.m_data[8] = s;
        m.m_data[10] = c;
        return m;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::rotateZ(T angleRad) noexcept
    {
        T c = std::cos(angleRad);
        T s = std::sin(angleRad);
        Matrix4<T> m = identity();
        m.m_data[0] = c;
        m.m_data[1] = s;
        m.m_data[4] = -s;
        m.m_data[5] = c;
        return m;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::scale(const TVector<T>& scale) noexcept
    {
        Matrix4<T> m = identity();
        m.m_data[0] = scale.X;
        m.m_data[5] = scale.Y;
        m.m_data[10] = scale.Z;
        return m;
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::scale(T uniformScale) noexcept
    {
        return scale(TVector<T>(uniformScale));
    }

    template<FloatingPoint T>
    Matrix4<T> Matrix4<T>::TRS(const TVector<T>& translation, const TQuat<T>& rotation, const TVector<T>& scale) noexcept
    {
        return translate(translation) * rotate(rotation) * Matrix4<T>::scale(scale);
    }

    template<FloatingPoint T>
    TVector<T> Matrix4<T>::getTranslation() const noexcept
    {
        return TVector<T>(m_data[12], m_data[13], m_data[14]);
    }

    template<FloatingPoint T>
    TQuat<T> Matrix4<T>::getRotation() const noexcept
    {
        T trace = m_data[0] + m_data[5] + m_data[10];
        T w, x, y, z;

        if (trace > T(0)) {
            T s = std::sqrt(trace + T(1)) * T(2);
            w = s * T(0.25);
            x = (m_data[6] - m_data[9]) / s;
            y = (m_data[8] - m_data[2]) / s;
            z = (m_data[1] - m_data[4]) / s;
        } else if (m_data[0] > m_data[5] && m_data[0] > m_data[10]) {
            T s = std::sqrt(T(1) + m_data[0] - m_data[5] - m_data[10]) * T(2);
            w = (m_data[6] - m_data[9]) / s;
            x = s * T(0.25);
            y = (m_data[1] + m_data[4]) / s;
            z = (m_data[8] + m_data[2]) / s;
        } else if (m_data[5] > m_data[10]) {
            T s = std::sqrt(T(1) + m_data[5] - m_data[0] - m_data[10]) * T(2);
            w = (m_data[8] - m_data[2]) / s;
            x = (m_data[1] + m_data[4]) / s;
            y = s * T(0.25);
            z = (m_data[6] + m_data[9]) / s;
        } else {
            T s = std::sqrt(T(1) + m_data[10] - m_data[0] - m_data[5]) * T(2);
            w = (m_data[1] - m_data[4]) / s;
            x = (m_data[8] + m_data[2]) / s;
            y = (m_data[6] + m_data[9]) / s;
            z = s * T(0.25);
        }

        return TQuat<T>(w, x, y, z).normalized();
    }

    template<FloatingPoint T>
    TVector<T> Matrix4<T>::getScale() const noexcept
    {
        TVector<T> xAxis(m_data[0], m_data[1], m_data[2]);
        TVector<T> yAxis(m_data[4], m_data[5], m_data[6]);
        TVector<T> zAxis(m_data[8], m_data[9], m_data[10]);
        return TVector<T>(xAxis.Length(), yAxis.Length(), zAxis.Length());
    }

    // 显式实例化，以便 MSVC/Clang 构建模块
    template class Matrix4<float>;
    template class Matrix4<double>;
}


