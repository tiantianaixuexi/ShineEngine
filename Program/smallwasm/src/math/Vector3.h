#pragma once


namespace shine::math {

    class Vector3 {
    public:
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

   
        constexpr Vector3() noexcept = default;
        constexpr Vector3(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}

        constexpr Vector3(const Vector3&) noexcept = default;
        constexpr Vector3(Vector3&&) noexcept = default;
        constexpr Vector3& operator=(const Vector3&) noexcept = default;
        constexpr Vector3& operator=(Vector3&&) noexcept = default;

        static Vector3 Set(float x_, float y_, float z_) noexcept { return Vector3(x_, y_, z_); }

        Vector3 Clone() const noexcept { return Vector3(x, y, z); }

        void SetScalar(float scalar) noexcept { x = scalar; y = scalar; z = scalar; }
        void Add(const Vector3& other) noexcept { x += other.x; y += other.y; z += other.z; }
        void Subtract(const Vector3& other) noexcept { x -= other.x; y -= other.y; z -= other.z; }
        void Multiply(const Vector3& other) noexcept { x *= other.x; y *= other.y; z *= other.z; }
        void Divide(const Vector3& other) noexcept { x /= other.x; y /= other.y; z /= other.z; }
        void Divide(float scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; }

    };

    inline constinit Vector3 Zero{0.0f, 0.0f, 0.0f};
    inline constinit Vector3 One{1.0f, 1.0f, 1.0f};
    inline constinit Vector3 Up{0.0f, 1.0f, 0.0f};
    inline constinit Vector3 Down{0.0f, -1.0f, 0.0f};
    inline constinit Vector3 Left{-1.0f, 0.0f, 0.0f};
    inline constinit Vector3 Right{1.0f, 0.0f, 0.0f};
    inline constinit Vector3 Forward{0.0f, 0.0f, 1.0f};
    inline constinit Vector3 Back{0.0f, 0.0f, -1.0f};

}