#include "renderer_service.h"

#include "render/backend/render_backend.h"
#include "render/pipeline/render_pipeline_asset.h"
#include "render/pipeline/command_buffer.h"
#include "render/command/command_list.h"
#include "gameplay/object.h"
#include "manager/CameraManager.h"
#include "manager/light_manager.h"

namespace shine::render
{
    void RendererService::init(backend::IRenderBackend* backend) noexcept
    {
        m_Backend = backend;
        TextureManager::get().Initialize(backend);

        // 创建默认渲染管线资源
        if (!m_RenderPipelineAsset)
        {
            m_RenderPipelineAsset = std::make_shared<DefaultRenderPipelineAsset>();
            m_RenderPipeline = m_RenderPipelineAsset->CreatePipeline();
        }

        // 设置渲染上下文的执行回调
        setupRenderContext();
    }
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
        if (!m_Backend || handle == 0) return;
        if (!m_RenderPipeline) return;

        // 收集渲染数据
        RenderingData renderingData = collectRenderingData(handle, camera);

        // 清空渲染上下文
        m_RenderContext.Clear();

        // 执行渲染管线
        m_RenderPipeline->Render(m_RenderContext, renderingData);

        // 执行所有提交的命令
        m_RenderContext.Execute();
    }

    void RendererService::endFrame(const std::array<float,4>& clear_color) noexcept
    {
        if (!m_Backend) return;
        m_Backend->RenderToFramebuffer(clear_color);
    }

    void RendererService::setRenderPipelineAsset(std::shared_ptr<RenderPipelineAsset> asset) noexcept
    {
        if (asset)
        {
            m_RenderPipelineAsset = asset;
            m_RenderPipeline = asset->CreatePipeline();
        }
    }

    RenderingData RendererService::collectRenderingData(ViewportHandle handle, shine::gameplay::Camera* camera) noexcept
    {
        RenderingData data;

        // 设置主相机
        if (camera)
        {
            data.mainCamera = camera;
            data.cameras.push_back(camera);
        }
        else
        {
            // 如果没有传入相机，从 CameraManager 获取主相机
            auto* mainCam = shine::manager::CameraManager::get().getMainCamera();
            if (mainCam)
            {
                data.mainCamera = mainCam;
                data.cameras.push_back(mainCam);
            }
        }

        // 设置光源管理器
        data.lightManager = &shine::manager::LightManager::get();

        // 收集场景对象
        data.sceneObjects.reserve(m_SceneObjects.size());
        for (auto* obj : m_SceneObjects)
        {
            if (obj)
            {
                data.sceneObjects.push_back(obj);
            }
        }

        // 设置视口信息
        if (const auto it = m_Viewports.find(handle); it != m_Viewports.end())
        {
            data.viewport.handle = handle;
            data.viewport.width = it->second.width;
            data.viewport.height = it->second.height;
        }

        return data;
    }

    void RendererService::setupRenderContext() noexcept
    {
        // 设置执行回调，将 CommandBuffer 的命令执行到后端
        // 使用 RenderSceneWith 的回调机制来执行命令
        m_RenderContext.SetExecuteCallback([this](CommandBuffer* cmdBuffer) {
            if (!cmdBuffer || !m_Backend) return;

            // 获取当前视口句柄（使用第一个视口，或默认0）
            ViewportHandle viewportHandle = 0;
            if (!m_Viewports.empty())
            {
                viewportHandle = m_Viewports.begin()->first;
            }

            // 通过 RenderSceneWith 执行命令
            // 这会绑定正确的FBO并执行命令
            m_Backend->RenderSceneWith(static_cast<s32>(viewportHandle), [cmdBuffer](shine::render::command::ICommandList& cmdList) {
                // 执行命令缓冲区中的所有命令
                cmdBuffer->Execute(cmdList);
            });
        });
    }
}


