#pragma once

#include <array>

#include "mathDef.h"
#include "quat.h"
#include "vector.h"






namespace shine::math
{
    template<MathPoint T>
    struct TVector;

	template<FloatingPoint T>
    class TQuat;

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
        static constexpr Matrix4 identity() noexcept;
        static constexpr Matrix4 zero() noexcept;
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

	using FMatrix4f = Matrix4<float>;
	using FMatrix4d = Matrix4<double>;
}

