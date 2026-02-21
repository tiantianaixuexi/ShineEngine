#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "shine_define.h"
#include "EngineCore/subsystem.h"
#include "render/backend/render_backend.h"
#include "render/pipeline/render_data.h"
#include "render/pipeline/scriptable_render_context.h"
#include "render/pipeline/render_pipeline_asset.h"
#include "render/pipeline/render_pipeline.h"
#include "render/resources/TextureManager.h"

namespace shine::gameplay
{
    class Camera;
    class SObject;
}

namespace shine::render
{
    using ViewportHandle = s32;

    // C++20 aggregate — no boilerplate needed
    struct ViewportRecord {
        int width  = 0;
        int height = 0;
    };

    // 渲染服务，管理一个后端实例，管理多个视图
    class RendererService : public shine::Subsystem
    {
    public:
        RendererService() = default;

        // 注入应用后端（必须先调用）
        void init(backend::IRenderBackend* backend) noexcept;

        // 视口管理
        ViewportHandle createViewport(int width, int height) noexcept;
        void destroyViewport(ViewportHandle handle) noexcept;
        void resizeViewport(ViewportHandle handle, int width, int height) noexcept;

        // 获取视图纹理（用于 ImGui 显示）
        [[nodiscard]]
        unsigned long long getViewportTexture(ViewportHandle handle) const noexcept;

        // 帧流程
        void beginFrame() noexcept;
        void renderView(ViewportHandle handle, shine::gameplay::Camera* camera) noexcept;
        void endFrame(const std::array<float,4>& clear_color) noexcept;

        // 场景对象注册（生命周期由外部管理）
        void registerObject(shine::gameplay::SObject* object) noexcept {
            if (object) m_SceneObjects.insert(object);
        }
        void unregisterObject(shine::gameplay::SObject* object) noexcept {
            m_SceneObjects.erase(object);
        }

        // 渲染管线
        void setRenderPipelineAsset(std::shared_ptr<RenderPipelineAsset> asset) noexcept;

        [[nodiscard]]
        backend::IRenderBackend* GetBackend() const noexcept { return m_Backend; }

    private:
        RenderingData collectRenderingData(ViewportHandle handle, shine::gameplay::Camera* camera) noexcept;
        void setupRenderContext() noexcept;

        backend::IRenderBackend* m_Backend{ nullptr };
        std::unordered_map<ViewportHandle, ViewportRecord> m_Viewports;
        ViewportHandle m_NextHandle{ 1 };

        std::unordered_set<shine::gameplay::SObject*> m_SceneObjects;

        // 渲染管线
        std::shared_ptr<RenderPipelineAsset> m_RenderPipelineAsset;
        std::shared_ptr<RenderPipeline>      m_RenderPipeline;
        ScriptableRenderContext              m_RenderContext;
        ViewportHandle                       m_CurrentViewportHandle{ 0 };
    };
}
