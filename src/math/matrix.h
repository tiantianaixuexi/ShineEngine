#pragma once

#include "math/mathDef.h"

#include <array>





namespace shine::math
{

    // 4x4 matrix, column-major (OpenGL convention)
    template<FloatingPoint T>
    class Matrix4
    {
    public:
        Matrix4();
        explicit Matrix4(T diag);
        Matrix4(const std::array<T, 16>& values);


        const T* data() const noexcept { return m_data.data(); }
        T* data() noexcept { return m_data.data(); }


        T get(int row, int col) const noexcept;
        void set(int row, int col, T v) noexcept;


        Matrix4 operator*(const Matrix4& rhs) const noexcept;

        Matrix4 transposed() const noexcept;


        std::array<T, 16> m_data{};
    };

	using FMatrix4f = Matrix4<float>;
	using FMatrix4d = Matrix4<double>;
}

