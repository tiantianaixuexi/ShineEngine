#pragma once


#include <array>
#include <unordered_map>
#include <unordered_set>



#include "shine_define.h"
#include "util/singleton.h"
#include "render/render_backend.h"


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
	class RendererService : public shine::util::Singleton<RendererService>
    {
    public:
        RendererService() = default;

        // 注入应用后端（必须先调用）
        void init(backend::IRenderBackend* backend) noexcept { m_Backend = backend; }

        // 创建/销毁视图（当前实现基于封装后端的单一 FBO，返回句柄不为1）
        ViewportHandle createViewport(int width, int height) noexcept;
        void destroyViewport(ViewportHandle handle) noexcept;
        void resizeViewport(ViewportHandle handle, int width, int height) noexcept;

        // 获取视图纹理，用于ImGui 显示
        unsigned int getViewportTexture(ViewportHandle handle) const noexcept;

        // 帧流程与视图渲染（最小实现，后续支持多视窗、多相机）
        void beginFrame() noexcept;
        void renderView(ViewportHandle handle,  shine::gameplay::Camera* camera) noexcept;
        void endFrame(const std::array<float,4>& clear_color) noexcept;

        // 场景对象注册（暂时简单保护空指针，生命周期由外部管理）
        void registerObject(shine::gameplay::SObject* object) noexcept { if (object) m_SceneObjects.insert(object); }
        void unregisterObject(shine::gameplay::SObject* object) noexcept { m_SceneObjects.erase(object); }

    private:

        backend::IRenderBackend* m_Backend { nullptr };
        std::unordered_map<ViewportHandle, ViewportRecord> m_Viewports;
        ViewportHandle m_NextHandle { 1 };

        std::unordered_set<shine::gameplay::SObject*> m_SceneObjects;
    };
}

