#pragma once

#include "EngineCore/engine_context.h"
#include "EngineCore/subsystem.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <intrin.h>   // _mm_pause
#include <mmsystem.h> // timeBeginPeriod, timeEndPeriod

#pragma comment(lib, "winmm.lib")

namespace shine::util {

// FPS控制器类
class FPSController : public Subsystem {
private:
    double m_targetFPS{};
    double m_targetFrameTime{}; // 目标帧时间（毫秒）
    double m_deltaTime{};
    double m_frameTimeAccumulator{};
    int    m_frameCount{};
    double m_actualFPS{};
    bool   m_enabled{};

    // 使用Windows高精度计时器
    LARGE_INTEGER m_frequency{};
    LARGE_INTEGER m_frameStartTime{};  // 重命名：明确是帧开始时间
    UINT          m_timerResolution{}; // 存储设置的时间精度

    // 将QPC转换为毫秒
    [[nodiscard]] double ToMilliseconds(const LARGE_INTEGER &start, const LARGE_INTEGER &end) const {
        return static_cast<double>(end.QuadPart - start.QuadPart) * 1000.0 / (double)m_frequency.QuadPart;
    }

public:
    static FPSController &get() {
        return *EngineContext::Get().GetSystem<FPSController>();
    }

    void Shutdown(EngineContext &ctx) override {
        // 恢复系统定时器精度
        if (m_timerResolution > 0) {
            timeEndPeriod(m_timerResolution);
        }
    }

    FPSController(double targetFPS = 60.0)
        : m_targetFPS(targetFPS),
        m_targetFrameTime(1000.0 / targetFPS),
        m_enabled(true)
    {

        // 初始化高精度计时器
        QueryPerformanceFrequency(&m_frequency);
        QueryPerformanceCounter(&m_frameStartTime);

        timeBeginPeriod(m_timerResolution);
    }

    // 设置目标FPS
    void SetTargetFPS(double fps) noexcept {
        m_targetFPS       = fps;
        m_targetFrameTime = 1000.0 / fps;
    }

    // 获取目标FPS
    [[nodiscard]] double GetTargetFPS() const noexcept {
        return m_targetFPS;
    }

    // 获取实际FPS
    [[nodiscard]] double GetActualFPS() const noexcept {
        return m_actualFPS;
    }

    // 获取帧时间（毫秒）
    [[nodiscard]] double GetDeltaTime() const noexcept {
        return m_deltaTime;
    }

    // 启用/禁用FPS控制
    void SetEnabled(bool enabled) noexcept {
        m_enabled = enabled;
    }

    [[nodiscard]] bool IsEnabled() const noexcept {
        return m_enabled;
    }

    // 帧开始时调用
    void BeginFrame() {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        // 计算上一帧的持续时间（毫秒）
        m_deltaTime      = ToMilliseconds(m_frameStartTime, currentTime);
        m_frameStartTime = currentTime;

        // 累积帧时间用于计算实际FPS
        m_frameTimeAccumulator += m_deltaTime;
        m_frameCount++;

        // 每秒更新一次实际FPS
        if (m_frameTimeAccumulator >= 1000.0) {
            m_actualFPS            = m_frameCount * 1000.0 / m_frameTimeAccumulator;
            m_frameTimeAccumulator = 0.0;
            m_frameCount           = 0;
        }
    }

    // 帧结束时调用，进行FPS控制
    void EndFrame() {
        if (!m_enabled) {
            return;
        }

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        // 计算本帧已经执行的时间（从BeginFrame到现在）
        double elapsed = ToMilliseconds(m_frameStartTime, currentTime);

        if (elapsed < m_targetFrameTime) {
            // 需要等待的时间
            double remaining = m_targetFrameTime - elapsed;

            // 策略：Sleep大部分，自旋等待剩余（减少CPU占用同时保持精度）
            if (remaining > 2.0) {
                // 留出2ms用于自旋等待，补偿Sleep的不精确性
                // 由于设置了timeBeginPeriod(1)，Sleep精度约为1ms
                auto sleepMs = static_cast<DWORD>(remaining - 2.0);
                Sleep(sleepMs);

                // 重新计算剩余时间
                QueryPerformanceCounter(&currentTime);
                elapsed   = ToMilliseconds(m_frameStartTime, currentTime);
                remaining = m_targetFrameTime - elapsed;
            }

            // 自旋等待剩余时间（通常 < 2ms，保证精确帧率）
            while (elapsed < m_targetFrameTime) {
                QueryPerformanceCounter(&currentTime);
                elapsed = ToMilliseconds(m_frameStartTime, currentTime);

                _mm_pause();
            }
        }
    }
};
} // namespace shine::util