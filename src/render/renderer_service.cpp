#include "renderer_service.h"

#include "render/backend/render_backend.h"
#include "render/pipeline/render_pipeline_asset.h"
#include "render/pipeline/command_buffer.h"
#include "gameplay/object.h"
#include "manager/CameraManager.h"
#include "manager/light_manager.h"
#include "render/resources/TextureManager.h"
#include "render/resources/shader_manager.h"

namespace shine::render
{
    void RendererService::init(backend::IRenderBackend* backend) noexcept
    {
        m_Backend = backend;
        TextureManager::get().Initialize(backend);
        ShaderManager::get().Initialize(backend);

        // 创建默认渲染管线资源
        if (!m_RenderPipelineAsset)
        {
            m_RenderPipelineAsset = std::make_shared<DefaultRenderPipelineAsset>();
            m_RenderPipeline = m_RenderPipelineAsset->CreatePipeline();
        }

        setupRenderContext();
    }

    ViewportHandle RendererService::createViewport(int width, int height) noexcept
    {
        if (!m_Backend) return 0;
        const auto handle = m_Backend->CreateViewport(width, height);
        if (handle != 0) {
            m_Viewports.emplace(handle, ViewportRecord{ width, height });
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
        if (auto it = m_Viewports.find(handle); it != m_Viewports.end()) {
            it->second = ViewportRecord{ width, height };
        }
    }

    unsigned long long RendererService::getViewportTexture(ViewportHandle handle) const noexcept
    {
        if (!m_Backend || handle == 0) return 0;
        return m_Backend->GetViewportTexture(handle);
    }

    void RendererService::beginFrame() noexcept
    {
        m_Backend->ImguiNewFrame();
    }

    void RendererService::renderView(ViewportHandle handle, shine::gameplay::Camera* camera) noexcept
    {
        if (!m_Backend || handle == 0) return;
        if (!m_RenderPipeline) return;

        RenderingData renderingData = collectRenderingData(handle, camera);
        m_CurrentViewportHandle = handle;
        m_CurrentRenderingData = &renderingData;

        m_RenderContext.Clear();
        m_RenderContext.Reserve(m_RenderPipeline->GetPasses().size());

        m_RenderPipeline->Render(m_RenderContext, renderingData);

        m_RenderContext.Execute();
        m_CurrentRenderingData = nullptr;
        m_CurrentViewportHandle = 0;
    }

    void RendererService::endFrame(const std::array<float,4>& clear_color) noexcept
    {
        m_Backend->RenderToFramebuffer(clear_color);
    }

    void RendererService::setRenderPipelineAsset(std::shared_ptr<RenderPipelineAsset> asset) noexcept
    {
        if (asset)
        {
            m_RenderPipelineAsset = std::move(asset);
            m_RenderPipeline = m_RenderPipelineAsset->CreatePipeline();
        }
    }

    RenderingData RendererService::collectRenderingData(ViewportHandle handle, shine::gameplay::Camera* camera) noexcept
    {
        RenderingData data;

        if (camera)
        {
            data.mainCamera = camera;
            data.cameras.push_back(camera);
        }
        else
        {
            if (auto* mainCam = manager::CameraManager::get().getMainCamera())
            {
                data.mainCamera = mainCam;
                data.cameras.push_back(mainCam);
            }
        }

        data.lightManager = &shine::manager::LightManager::get();

        data.sceneObjects.reserve(m_SceneObjects.size());
        for (auto* obj : m_SceneObjects)
        {
            if (obj) data.sceneObjects.push_back(obj);
        }

        if (const auto it = m_Viewports.find(handle); it != m_Viewports.end())
        {
            data.viewport.handle = handle;
            data.viewport.width  = it->second.width;
            data.viewport.height = it->second.height;
        }

        data.enablePostProcessing = m_EnablePostProcessing;

        return data;
    }

    void RendererService::setupRenderContext() noexcept
    {
        m_RenderContext.SetExecuteCallback([this](CommandBuffer* cmdBuffer) {
            if (!cmdBuffer || !m_Backend) return;
            if (!m_CurrentRenderingData) return;

            ViewportHandle viewportHandle = m_CurrentViewportHandle;
            if (viewportHandle == 0 && !m_Viewports.empty())
            {
                viewportHandle = m_Viewports.begin()->first;
            }

            m_Backend->ExecuteCommandBuffer(static_cast<s32>(viewportHandle), *m_CurrentRenderingData, cmdBuffer);
        });
    }
}
