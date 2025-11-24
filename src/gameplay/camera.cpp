#include "Camera.h"

#include <GL/glew.h>
#include <cmath>

#include "math/mathUtil.h"


namespace shine::gameplay
{
    using namespace shine::math;

    void Camera::SetRotationFromEuler(FRotator3f rotator)
    {
        // 将欧拉角转换为弧度
        float yawRad = math::radians(rotator.Yaw);
        float pitchRad = math::radians(rotator.Pitch);
        float rollRad = math::radians(rotator.Roll);

        // 计算各个欧拉角的三角函数值
        float cy = std::cos(yawRad * 0.5f);
        float sy = std::sin(yawRad * 0.5f);
        float cp = std::cos(pitchRad * 0.5f);
        float sp = std::sin(pitchRad * 0.5f);
        float cr = std::cos(rollRad * 0.5f);
        float sr = std::sin(rollRad * 0.5f);

        // 计算四元数分量(w, x, y, z)
        quaternion.w = cr * cp * cy + sr * sp * sy;  // w
        quaternion.x = sr * cp * cy - cr * sp * sy;  // x
        quaternion.y = cr * sp * cy + sr * cp * sy;  // y
        quaternion.z = cr * cp * sy - sr * sp * cy;  // z

        // 确保四元数规范化
        quaternion = quaternion.normalized();

        // 更新相机方向向量
        UpdateCameraVectors();
    }

    void Camera::SetRotationFromEuler(float yaw, float pitch, float roll)
    {
            // 将欧拉角转换为弧度
            float yawRad = math::radians(yaw);
            float pitchRad = math::radians(pitch);
            float rollRad = math::radians(roll);

            // 计算各个欧拉角的三角函数值
            float cy = std::cos(yawRad * 0.5f);
            float sy = std::sin(yawRad * 0.5f);
            float cp = std::cos(pitchRad * 0.5f);
            float sp = std::sin(pitchRad * 0.5f);
            float cr = std::cos(rollRad * 0.5f);
            float sr = std::sin(rollRad * 0.5f);

            // 计算四元数分量(w, x, y, z)
            quaternion.w = cr * cp * cy + sr * sp * sy;  // w
            quaternion.x = sr * cp * cy - cr * sp * sy;  // x
            quaternion.y = cr * sp * cy + sr * cp * sy;  // y
            quaternion.z = cr * cp * sy - sr * sp * cy;  // z

            // 确保四元数规范化
            quaternion = quaternion.normalized();

            // 更新相机方向向量
            UpdateCameraVectors();
    }

    void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
    {
        // 灵敏度修正
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        // 使用向量四元数方法：
        // - 水平旋转围绕世界向上旋转（类似 UE 视角控制）
        // - 垂直旋转围绕局部右旋转（鼠标上下移动）
        const float yawRad   = math::radians(xoffset);
        const float pitchRad = math::radians(-yoffset);

        // 先做世界 Up 轴的 Yaw
        const FQuatf yawQ = FQuatf::fromAxisAngle({0.0f, 1.0f, 0.0f}, yawRad);
        FQuatf tmpQ = yawQ * quaternion;

        // 基于临时结果计算右旋转（用四元数旋转单位右向量）
        const auto r = tmpQ.rotate({1.0f, 0.0f, 0.0f});
        const FQuatf pitchQ = FQuatf::fromAxisAngle({r[0], r[1], r[2]}, pitchRad);
        FQuatf newQ = pitchQ * tmpQ;

        if (constrainPitch) {
            auto rot = newQ.toRotatorDegrees();
            if (rot.Pitch > 89.0f || rot.Pitch < -89.0f) {
                // 超出俯仰限制则只应用 Yaw
                newQ = tmpQ;
            }
        }

        quaternion = newQ.normalized();
        UpdateCameraVectors();
    }

    void Camera::UpdateCameraVectors()
    {
        // 通过旋转单位向量，构建标准正交的局部坐标系
        const auto f = quaternion.rotate({0.0f, 0.0f, -1.0f});
        const auto r = quaternion.rotate({1.0f, 0.0f,  0.0f});

        front = { static_cast<double>(f[0]), static_cast<double>(f[1]), static_cast<double>(f[2]) };
        right = { static_cast<double>(r[0]), static_cast<double>(r[1]), static_cast<double>(r[2]) };

        front.Normalize();
        right.Normalize();
        // 使用固定世界向上进行 Gram-Schmidt，确保与世界向上不共线
        FVector3d worldUpD = worldUp;
        if (std::abs(front.Dot(worldUpD)) > 0.999) {
            worldUpD = {1.0, 0.0, 0.0};
        }
        // 确保右向量与前向量、上向量构成右手系
        right = front.Cross(worldUpD);
        right.Normalize();
        up = right.Cross(front);
        up.Normalize();
    }

    void Camera::Apply()
    {
            // 设置投影矩阵
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            FMatrix4d P = GetProjectionMatrixM();
            glLoadMatrixd(P.data());

            // 设置模型视图矩阵
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            FMatrix4d V = GetViewMatrixM();
            glLoadMatrixd(V.data());
    }

    FMatrix4d Camera::GetProjectionMatrixM() const noexcept
    {
        return isPerspective
            ? math::Perspective<double>(fov, aspectRatio, nearPlane, farPlane)
            : math::Ortho<double>(leftBound, rightBound, bottomBound, topBound, nearPlane, farPlane);
    }

    FMatrix4d Camera::GetViewMatrixM() const noexcept
    {
        double centerX = position.X + front.X;
        double centerY = position.Y + front.Y;
        double centerZ = position.Z + front.Z;
        return math::LookAt<double>(position, {centerX, centerY, centerZ}, up);
    }

    FMatrix4d Camera::GetViewProjectionMatrixM() const noexcept
    {
        FMatrix4d P = GetProjectionMatrixM();
        FMatrix4d V = GetViewMatrixM();
        return P * V;
    }
}


