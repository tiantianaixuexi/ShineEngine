#ifdef SHINE_USE_MODULE

module;

export module shine.math.matrix4;

import <array>;
import shine.math.mathDef;
import shine.math.quat;
import shine.math.vector;


#else

#pragma once
#include <array>
#include "mathDef.h"
#include "quat.h"
#include "vector.ixx"

#endif





namespace shine::math
{

    template<FloatingPoint T>
    class TQuat;

    template<IsNumber T>
    class TVector;


    // 4x4 matrix, column-major (OpenGL convention)
    template<FloatingPoint T>
    class Matrix4
    {
    public:
        constexpr Matrix4() noexcept
        {
            m_data.fill(T(0));
            m_data[0] = m_data[5] = m_data[10] = m_data[15] = T(1);
        }

        constexpr explicit Matrix4(T diag) noexcept 
        {
            m_data.fill(T(0));
            m_data[0] = m_data[5] = m_data[10] = m_data[15] = diag;
        }

        constexpr Matrix4(const std::array<T, 16>& values) noexcept
            : m_data(values)
        {}


        const T* data() const noexcept { return m_data.data(); }
        T* data() noexcept { return m_data.data(); }


        T get(int row, int col) const noexcept;
        void set(int row, int col, T v) noexcept;


        Matrix4 operator*(const Matrix4& rhs) const noexcept;
        Matrix4 operator+(const Matrix4& rhs) const noexcept;
        Matrix4 operator-(const Matrix4& rhs) const noexcept;
        Matrix4 operator*(T scalar) const noexcept;
        Matrix4 operator/(T scalar) const noexcept;
        Matrix4& operator*=(const Matrix4& rhs) noexcept;
        Matrix4& operator+=(const Matrix4& rhs) noexcept;
        Matrix4& operator-=(const Matrix4& rhs) noexcept;
        Matrix4& operator*=(T scalar) noexcept;
        Matrix4& operator/=(T scalar) noexcept;

        Matrix4 transposed() const noexcept;

        // 向量变换
        TVector<T> transformVector(const TVector<T>& v) const noexcept;
        TVector<T> transformPoint(const TVector<T>& p) const noexcept;

        // 矩阵运算
        Matrix4 inverse() const noexcept;
        T determinant() const noexcept;

        // 静态工厂方法
        static constexpr Matrix4 identity() noexcept
        {
            return Matrix4();
        }

        static constexpr Matrix4 zero() noexcept
        {
            Matrix4 m;
            m.m_data.fill(T(0));
            return m;
        }
        static Matrix4 translate(const TVector<T>& translation) noexcept;
        static Matrix4 rotate(const TQuat<T>& rotation) noexcept;
        static Matrix4 rotateX(T angleRad) noexcept;
        static Matrix4 rotateY(T angleRad) noexcept;
        static Matrix4 rotateZ(T angleRad) noexcept;
        static Matrix4 scale(const TVector<T>& scale) noexcept;
        static Matrix4 scale(T uniformScale) noexcept;
        static Matrix4 TRS(const TVector<T>& translation, const TQuat<T>& rotation, const TVector<T>& scale) noexcept;

        // 获取变换分量
        TVector<T> getTranslation() const noexcept;
        TQuat<T> getRotation() const noexcept;
        TVector<T> getScale() const noexcept;

        std::array<T, 16> m_data{};
    };

	SHINE_MODULE_EXPORT using FMatrix4f = Matrix4<float>;
	SHINE_MODULE_EXPORT using FMatrix4d = Matrix4<double>;
}

