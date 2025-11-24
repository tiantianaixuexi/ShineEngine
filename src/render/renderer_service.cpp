#include "renderer_service.h"


#include "render/render_backend.h"
#include "gameplay/object.h"
#include "gameplay/component/component.h"
namespace shine::render
{
    ViewportHandle RendererService::createViewport(int width, int height) noexcept
    {
        if (!m_Backend) return 0;
        const auto handle = m_Backend->CreateViewport(width, height);
        if (handle != 0) {
            m_Viewports.emplace(handle, ViewportRecord(width, height));
        }
        return handle;
    }

    void RendererService::destroyViewport(ViewportHandle handle) noexcept
    {
        if (!m_Backend || handle == 0) return;
        m_Backend->DestroyViewport(handle);
        m_Viewports.erase(handle);
    }

    void RendererService::resizeViewport(ViewportHandle handle, int width, int height) noexcept
    {
        if (!m_Backend || handle == 0) return;
        m_Backend->ResizeViewport(handle, width, height);
        auto it = m_Viewports.find(handle);
        if (it != m_Viewports.end()) { it->second.width = width; it->second.height = height; }
    }

    unsigned int RendererService::getViewportTexture(ViewportHandle handle) const noexcept
    {
        if (!m_Backend || handle == 0) return 0;
        return m_Backend->GetViewportTexture(handle);
    }

    void RendererService::beginFrame() noexcept
    {
        // 目前 ImGui 已在应用层创建，这里无需额外处理
    }

    void RendererService::renderView(ViewportHandle handle, shine::gameplay::Camera* camera) noexcept
    {
        (void)camera; // 当前后端从CameraManager读取相机，后续可传参
        if (!m_Backend || handle == 0) return;

        // 通过回调将场景对象的组件渲染提交到CommandList
        m_Backend->RenderSceneWith(handle, [this](shine::render::command::ICommandList& cmd){
            for (auto* obj : m_SceneObjects) {
                if (!obj) continue;
                for (auto& compPtr : obj->getComponents()) {
                    compPtr->onRender(cmd);
                }
            }
        });
    }

    void RendererService::endFrame(const std::array<float,4>& clear_color) noexcept
    {
        if (!m_Backend) return;
        m_Backend->RenderToFramebuffer(clear_color);
    }
}


