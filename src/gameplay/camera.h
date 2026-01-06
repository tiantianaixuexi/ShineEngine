#pragma once

#include <string>


#include "math/rotator.h"
#include "math/matrix.ixx"
#include "math/quat.h"

namespace shine::gameplay
{
	using namespace shine::math;

    enum class CameraMovement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    class Camera
    {
    public:

    private:

        std::string name;

    public:

        Camera(const std::string& cameraName = "默认相机")
            : name(cameraName),
            position{0.0f, 0.0f, 5.0f},
            front{0.0f, 0.0f, -1.0f},
            up{0.0f, 1.0f, 0.0f},
            right{1.0f, 0.0f, 0.0f},
            worldUp{0.0f, 1.0f, 0.0f},
            quaternion{1.0f, 0.0f, 0.0f, 0.0f}, // 初始单位四元数
            movementSpeed(2.5f),
            mouseSensitivity(0.05f),
            fov(45.0f),
            aspectRatio(16.0f/9.0f),
            nearPlane(0.1f),
            farPlane(100.0f),
            isPerspective(true)
        {
            // 设置初始旋转，相当于原来的yaw=-90, pitch=0, roll=0
            SetRotationFromEuler(FRotator3f::ZeroRotator());
        }

        // 设置相机位置
        constexpr void SetPosition(float x, float y, float z) noexcept
        {
            position.X = x;
            position.Y = y;
            position.Z = z;
        }

        // 获取相机位置
        FVector3d GetPosition() const noexcept { return position; }

        // 设置透视投影
        constexpr void SetPerspective(float fieldOfView, float aspect, float nearClip, float farClip) noexcept
        {
            fov = fieldOfView;
            aspectRatio = aspect;
            nearPlane = nearClip;
            farPlane = farClip;
            isPerspective = true;
        }

        // 设置正交投影
        constexpr void SetOrthographic(float _leftBound, float _rightBound, float _bottomBound, float _topBound, float _nearClip, float _farClip) noexcept
        {
            this->leftBound = _leftBound;
            this->rightBound = _rightBound;
            this->bottomBound = _bottomBound;
            this->topBound = _topBound;
            nearPlane = _nearClip;
            farPlane = _farClip;
            isPerspective = false;
        }

        // 应用相机变换和投影到OpenGL矩阵堆栈
        void Apply();


        // 处理键盘移动
        void ProcessKeyboard(CameraMovement direction, float deltaTime)
        {
            float velocity = movementSpeed * deltaTime;

            if (direction == CameraMovement::FORWARD)
                MoveForward(velocity);
            if (direction == CameraMovement::BACKWARD)
                MoveForward(-velocity);
            if (direction == CameraMovement::LEFT)
                MoveRight(-velocity);
            if (direction == CameraMovement::RIGHT)
                MoveRight(velocity);
            if (direction == CameraMovement::UP)
                MoveUp(velocity);
            if (direction == CameraMovement::DOWN)
                MoveUp(-velocity);
        }

        // 处理鼠标移动，实现见 .cpp
        void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

        // 处理鼠标滚轮缩放
        void ProcessMouseScroll(float yoffset)
        {
            fov -= yoffset;
            if (fov < 1.0f)
                fov = 1.0f;
            if (fov > 45.0f)
                fov = 45.0f;
        }

        // 设置旋转，从欧拉角
        void SetRotationFromEuler(FRotator3f rotator);

        void SetRotationFromEuler(float yaw, float pitch, float roll);


        //返回投影矩阵
        FMatrix4d GetProjectionMatrixM() const noexcept;

        // 接口：返回视图矩阵
        FMatrix4d GetViewMatrixM() const noexcept;

        // 接口：返回视图投影矩阵
        FMatrix4d GetViewProjectionMatrixM() const noexcept;

    private:


        // 更新相机的前、右、上向量，实现见 .cpp
        void UpdateCameraVectors();

        // 向前移动
        void MoveForward(float distance)
        {
            position += front * distance;
        }

        // 向右移动
        void MoveRight(float distance)
        {
            position += right * distance;
        }

        // 向上移动
        void MoveUp(float distance)
        {
            position += up * distance;
        }

public:

        // 相机属性
        FVector3d position;     // 位置
        FVector3d front;        // 前向量
        FVector3d up;           // 上向量
        FVector3d right;        // 右向量
        FVector3d worldUp;      // 世界向上向量

        // 四元数旋转表示(w,x,y,z)
        FQuatf quaternion;

        // 相机选项
        float movementSpeed;
        float mouseSensitivity;

        // 投影参数
        float fov;
        float aspectRatio;
        float nearPlane;
        float farPlane;
        // 不知道为什么要再加这三个参数，不是已经有 quaternion 了吗
        // 正交投影参数
        float leftBound, rightBound, bottomBound, topBound;

        bool isPerspective;
    };
}

