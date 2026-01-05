#pragma once

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "shine_define.h"
#include "../../EngineCore/subsystem.h"
#include "../../EngineCore/engine_context.h"
#include "render/backend/render_backend.h"
#include "render/pipeline/rendering_data.h"
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
    using ViewportHandle = u32;

	struct ViewportRecord {
        int width{0};
        int height{0};

        ViewportRecord() : width(0), height(0) {}
        ViewportRecord(int w, int h) : width(w), height(h) {}
        ViewportRecord(const ViewportRecord&) = default;
        ViewportRecord(ViewportRecord&&) = default;
        ViewportRecord& operator=(const ViewportRecord&) = default;
        ViewportRecord& operator=(ViewportRecord&&) = default;
    };

    // 渲染服务，管理一个后端实例，管理多个视图（第一步先支持单视图，后续扩展）
	class RendererService : public shine::Subsystem
    {
    public:
        static constexpr size_t GetStaticID() { return shine::HashString("RendererService"); }

        RendererService() = default;

        // 注入应用后端（必须先调用）
        void init(backend::IRenderBackend* backend) noexcept;

        // 创建/销毁视图（当前实现基于封装后端的单一 FBO，返回句柄不为1）
        ViewportHandle createViewport(int width, int height) noexcept;
        void destroyViewport(ViewportHandle handle) noexcept;
        void resizeViewport(ViewportHandle handle, int width, int height) noexcept;

        // 获取视图纹理，用于ImGui 显示
        unsigned long long  getViewportTexture(ViewportHandle handle) const noexcept;

        // 帧流程与视图渲染（最小实现，后续支持多视窗、多相机）
        void beginFrame() noexcept;
        void renderView(ViewportHandle handle,  shine::gameplay::Camera* camera) noexcept;
        void endFrame(const std::array<float,4>& clear_color) noexcept;

        // 场景对象注册（暂时简单保护空指针，生命周期由外部管理）
        void registerObject(shine::gameplay::SObject* object) noexcept { if (object) m_SceneObjects.insert(object); }
        void unregisterObject(shine::gameplay::SObject* object) noexcept { m_SceneObjects.erase(object); }

        // 设置渲染管线资源
        void setRenderPipelineAsset(std::shared_ptr<RenderPipelineAsset> asset) noexcept;

    private:
        // 收集渲染数据
        RenderingData collectRenderingData(ViewportHandle handle, shine::gameplay::Camera* camera) noexcept;

        // 设置渲染上下文
        void setupRenderContext() noexcept;

        backend::IRenderBackend* m_Backend { nullptr };
        std::unordered_map<ViewportHandle, ViewportRecord> m_Viewports;
        ViewportHandle m_NextHandle { 1 };

        std::unordered_set<shine::gameplay::SObject*> m_SceneObjects;

        // 渲染管线相关
        std::shared_ptr<RenderPipelineAsset> m_RenderPipelineAsset;
        std::shared_ptr<RenderPipeline> m_RenderPipeline;
        ScriptableRenderContext m_RenderContext;
    };
}
