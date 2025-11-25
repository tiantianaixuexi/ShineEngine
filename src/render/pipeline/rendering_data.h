#pragma once

#include "shine_define.h"
#include <vector>
#include <memory>
#include <array>

namespace shine::gameplay
{
    class Camera;
    class SObject;
}

namespace shine::manager
{
    class LightManager;
}

namespace shine::render
{
    /**
     * @brief 渲染数据（类似 Unity RenderingData）
     * 包含渲染所需的所有数据：相机、光源、场景对象等
     */
    struct RenderingData
    {
        // 相机列表（支持多相机）
        std::vector<shine::gameplay::Camera*> cameras;

        // 主相机
        shine::gameplay::Camera* mainCamera = nullptr;

        // 光源管理器
        shine::manager::LightManager* lightManager = nullptr;

        // 场景对象列表
        std::vector<shine::gameplay::SObject*> sceneObjects;

        // 视口信息
        struct Viewport
        {
            s32 handle = 0;
            s32 width = 0;
            s32 height = 0;
        } viewport;

        // 渲染时间
        float deltaTime = 0.016f;

        // 清除颜色
        std::array<float, 4> clearColor = { 0.2f, 0.3f, 0.4f, 1.0f };
    };
}

