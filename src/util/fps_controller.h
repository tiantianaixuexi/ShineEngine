#pragma once


#include <string> 


#include "fmt/format.h"


namespace shine::util {

    // FPS控制器类
    class FPSController {
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
        void SetTargetFPS(double fps) {
            m_targetFPS = fps;
            m_targetFrameTime = 1000.0 / fps;
        }

        // 获取目标FPS
        double GetTargetFPS() const {
            return m_targetFPS;
        }

        // 获取实际FPS
        double GetActualFPS() const {
            return m_actualFPS;
        }

        // 获取帧时间（毫秒）
        double GetDeltaTime() const {
            return m_deltaTime;
        }

        // 启用/禁用FPS控制
        void SetEnabled(bool enabled) {
            m_enabled = enabled;
        }

        bool IsEnabled() const {
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

            // 计算需要等待的时间
            double waitTime = m_targetFrameTime - m_deltaTime;

            if (waitTime > 0.0) {
                // 使用高精度等待
                LARGE_INTEGER startWait, currentWait;
                QueryPerformanceCounter(&startWait);

                double waitTimeSeconds = waitTime / 1000.0;
                double waitTicks = waitTimeSeconds * m_frequency.QuadPart;

                do {
                    QueryPerformanceCounter(&currentWait);
                } while ((currentWait.QuadPart - startWait.QuadPart) < waitTicks);
            }
        }

        // 重置计时器
        void Reset() {
            QueryPerformanceCounter(&m_lastTime);
            m_frameTimeAccumulator = 0.0;
            m_frameCount = 0;
            m_actualFPS = 0.0;
        }
    };

    // 游戏引擎FPS管理器
    class EngineFPSManager {
    private:
        FPSController m_editorUIController;
        FPSController m_gameController;
        bool m_gameMode; // true为游戏模式，false为编辑器模式

    public:
        EngineFPSManager(double editorFPS = 60.0, double gameFPS = 60.0)
            : m_editorUIController(editorFPS)
            , m_gameController(gameFPS)
            , m_gameMode(false) {
        }

        // 设置编辑器UI FPS
        void SetEditorFPS(double fps) {
            m_editorUIController.SetTargetFPS(fps);
        }

        // 设置游戏FPS
        void SetGameFPS(double fps) {
            m_gameController.SetTargetFPS(fps);
        }

        // 获取编辑器FPS控制器
        FPSController& GetEditorController() {
            return m_editorUIController;
        }

        // 获取游戏FPS控制器
        FPSController& GetGameController() {
            return m_gameController;
        }

        // 设置当前模式
        void SetGameMode(bool gameMode) {
            m_gameMode = gameMode;
        }

        bool IsGameMode() const {
            return m_gameMode;
        }

        // 帧开始 - 根据当前模式选择控制器
        void BeginFrame() {
            if (m_gameMode) {
                m_gameController.BeginFrame();
            } else {
                m_editorUIController.BeginFrame();
            }
        }

        // 帧结束 - 根据当前模式选择控制器
        void EndFrame() {
            if (m_gameMode) {
                m_gameController.EndFrame();
            } else {
                m_editorUIController.EndFrame();
            }
        }

        // 获取当前活跃控制器的信息
        double GetCurrentFPS() const {
            return m_gameMode ? m_gameController.GetActualFPS() : m_editorUIController.GetActualFPS();
        }

        double GetCurrentTargetFPS() const {
            return m_gameMode ? m_gameController.GetTargetFPS() : m_editorUIController.GetTargetFPS();
        }

        double GetCurrentDeltaTime() const {
            return m_gameMode ? m_gameController.GetDeltaTime() : m_editorUIController.GetDeltaTime();
        }

        // 启用/禁用FPS控制
        void SetEditorFPSEnabled(bool enabled) {
            m_editorUIController.SetEnabled(enabled);
        }

        void SetGameFPSEnabled(bool enabled) {
            m_gameController.SetEnabled(enabled);
        }

        // 获取详细信息用于调试
        std::string GetDebugInfo() const {
            return fmt::format(
                "Mode: {} | Editor: {:.1f}/{:.1f} FPS | Game: {:.1f}/{:.1f} FPS | DeltaTime: {:.2f}ms",
                m_gameMode ? "Game" : "Editor",
                m_editorUIController.GetActualFPS(), m_editorUIController.GetTargetFPS(),
                m_gameController.GetActualFPS(), m_gameController.GetTargetFPS(),
                GetCurrentDeltaTime()
            );
        }
    };

} // namespace shine::util

