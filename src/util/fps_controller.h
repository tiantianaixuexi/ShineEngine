#pragma once

#include "EngineCore/subsystem.h"
#include "EngineCore/engine_context.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif


namespace shine::util {

    // FPS控制器类
    class FPSController : public Subsystem
	{

    private:
        double m_targetFPS;
        double m_targetFrameTime; // 目标帧时间（毫秒）
        double m_lastFrameTime;
        double m_deltaTime;
        double m_frameTimeAccumulator;
        int m_frameCount;
        double m_actualFPS;
        bool m_enabled;

        // 使用Windows高精度计时器
        LARGE_INTEGER m_frequency;
        LARGE_INTEGER m_lastTime;

    public:

        static FPSController& get() { return *EngineContext::Get().GetSystem<FPSController>(); }

    	void Shutdown(EngineContext& ctx) override
        {
	        
        }


        FPSController(double targetFPS = 60.0)
            : m_targetFPS(targetFPS)
            , m_targetFrameTime(1000.0 / targetFPS)
            , m_lastFrameTime(0.0)
            , m_deltaTime(0.0)
            , m_frameTimeAccumulator(0.0)
            , m_frameCount(0)
            , m_actualFPS(0.0)
            , m_enabled(true) {

            // 初始化高精度计时器
            QueryPerformanceFrequency(&m_frequency);
            QueryPerformanceCounter(&m_lastTime);
        }

        // 设置目标FPS
        void SetTargetFPS(double fps) noexcept {
            m_targetFPS = fps;
            m_targetFrameTime = 1000.0 / fps;
        }

        // 获取目标FPS
        double GetTargetFPS() const noexcept {
            return m_targetFPS;
        }

        // 获取实际FPS
        double GetActualFPS() const noexcept {
            return m_actualFPS;
        }

        // 获取帧时间（毫秒）
        double GetDeltaTime() const  noexcept{
            return m_deltaTime;
        }

        // 启用/禁用FPS控制
        void SetEnabled(bool enabled)noexcept {
            m_enabled = enabled;
        }

        bool IsEnabled() const noexcept {
            return m_enabled;
        }

        // 帧开始时调用
        void BeginFrame() {
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);

            // 计算帧时间（毫秒）
            m_deltaTime = static_cast<double>(currentTime.QuadPart - m_lastTime.QuadPart) * 1000.0 / m_frequency.QuadPart;
            m_lastTime = currentTime;

            // 累积帧时间用于计算实际FPS
            m_frameTimeAccumulator += m_deltaTime;
            m_frameCount++;

            // 每秒更新一次实际FPS
            if (m_frameTimeAccumulator >= 1000.0) {
                m_actualFPS = m_frameCount * 1000.0 / m_frameTimeAccumulator;
                m_frameTimeAccumulator = 0.0;
                m_frameCount = 0;
            }
        }

        // 帧结束时调用，进行FPS控制
        void EndFrame() {
            if (!m_enabled) {
                return;
            }

            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            double currentFrameTime = static_cast<double>(currentTime.QuadPart - m_lastTime.QuadPart) * 1000.0 / m_frequency.QuadPart;

            if (currentFrameTime < m_targetFrameTime) {
                // 如果当前帧时间小于目标帧时间，进行等待
                double waitTime = m_targetFrameTime - currentFrameTime;
                if (waitTime > 1.0) {
                    Sleep(static_cast<DWORD>(waitTime - 1.0)); // 留出1ms用于高精度自旋等待
                }

                // 自旋等待剩余时间
                do {
                    QueryPerformanceCounter(&currentTime);
                    currentFrameTime = static_cast<double>(currentTime.QuadPart - m_lastTime.QuadPart) * 1000.0 / m_frequency.QuadPart;
                } while (currentFrameTime < m_targetFrameTime);
            }
        }
    };
}
