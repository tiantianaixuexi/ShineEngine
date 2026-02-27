#pragma once

namespace shine::math {

    class Vector4 {
    public:
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;

        constexpr Vector4() noexcept = default;
        constexpr Vector4(float x_, float y_, float z_, float w_) noexcept : x(x_), y(y_), z(z_), w(w_) {}

        constexpr Vector4(const Vector4&) noexcept = default;
        constexpr Vector4(Vector4&&) noexcept = default;
        constexpr Vector4& operator=(const Vector4&) noexcept = default;
        constexpr Vector4& operator=(Vector4&&) noexcept = default;

        static Vector4 Set(float x_, float y_, float z_, float w_) noexcept { return Vector4(x_, y_, z_, w_); }

        Vector4 Clone() const noexcept { return Vector4(x, y, z, w); }

        void SetScalar(float scalar) noexcept { x = scalar; y = scalar; z = scalar; w = scalar; }
        void Add(const Vector4& other) noexcept { x += other.x; y += other.y; z += other.z; w += other.w; }
        void Subtract(const Vector4& other) noexcept { x -= other.x; y -= other.y; z -= other.z; w -= other.w; }
        void Multiply(const Vector4& other) noexcept { x *= other.x; y *= other.y; z *= other.z; w *= other.w; }
        void Divide(const Vector4& other) noexcept { x /= other.x; y /= other.y; z /= other.z; w /= other.w; }
        void Divide(float scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; w /= scalar; }
    };

    using Vec4 = Vector4;

} // namespace shine::math
