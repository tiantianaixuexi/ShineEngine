#include "matrix.h"

#include "math/mathDef.h"

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

    // 显式实例化，以便 MSVC/Clang 构建模块
    template class Matrix4<float>;
    template class Matrix4<double>;
}


