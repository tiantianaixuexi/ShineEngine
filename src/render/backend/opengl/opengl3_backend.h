#pragma once

#include "shine_define.h"

#include <memory>
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <expected>

#include <GL/glew.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "render/backend/render_backend.h"
#include "render/backend/gl/gl_common.h"

namespace shine::render::opengl3
{

#ifdef SHINE_OPENGL

    using ViewportInfo = shine::render::backend::gl::ViewportInfo;

    class OpenGLRenderBackend : public backend::IRenderBackend
    {
    public:
        // ---- Framebuffer resources (previously in base) ----
        GLuint g_FramebufferObject  = 0;
        GLuint g_FramebufferTexture = 0;
        GLuint g_DepthRenderbuffer  = 0;

        // ---- Platform context ----
        HGLRC g_hRC  = nullptr;
        HDC   g_hdc  = nullptr;

        // ---- Per-frame UBOs ----
        GLuint m_CameraUbo = 0;  // std140, binding=0
        GLuint m_LightUbo  = 0;  // std140, binding=1
        bool m_HasCameraUboCache = false;
        std::array<float, 20> m_CameraUboCache{};
        bool m_HasLightUboCache = false;
        std::array<float, 12> m_LightUboCache{};

        // ---- Multi-viewport FBO registry ----
        std::unordered_map<s32, ViewportInfo> m_Viewports;
        s32 m_NextViewportHandle{1};

        std::unordered_map<GLuint, GLuint> m_MeshVbos;

        // ---- Backend dimensions (moved from base class) ----
        int m_Width  = 800;
        int m_Height = 600;

        // ==== IRenderBackend implementation ====

        int  init(backend::NativeWindowHandle window, void* platformData) override;
        void InitImguiBackend(backend::NativeWindowHandle window) override;
        void ImguiNewFrame() override;
        bool CreateDevice(backend::NativeWindowHandle window) override;
        void CleanupDevice(backend::NativeWindowHandle window) override;
        void ClearUp(backend::NativeWindowHandle window) override;

        bool         CreateFrameBuffer() override;
        void         ReSizeFrameBuffer(int width, int height) override;
        unsigned int GetFramebufferTexture() override;

        void SetSize(int width, int height) override;
        [[nodiscard]] std::pair<int,int> GetSize() const noexcept override;

        void RenderScene(float deltaTime = 0.016f) override;
        void RenderSceneToFrameBuffer() override;
        void RenderSceneToViewport(s32 handle) override;
        void RenderToFramebuffer(const std::array<float, 4>& clear_color) override;

        void CompileShaders() override;

        void ExecuteCommandBuffer(s32 viewportHandle,
                                  const shine::render::RenderingData& renderingData,
                                  const shine::render::CommandBuffer* cmdBuffer) override;

        u64 CreateMesh(const backend::MeshCreateInfo& info) override;
        void ReleaseMesh(u64 handle) override;

        s32                CreateViewport(int width, int height) override;
        void               DestroyViewport(s32 handle) override;
        void               ResizeViewport(s32 handle, int width, int height) override;
        void               BindViewport(s32 handle) override;
        unsigned long long  GetViewportTexture(u32 handle) override;
        u32                GetViewportFBO(s32 handle) override;

        // ---- Custom FBO ----
        s32 CreateCustomFramebuffer(int width, int height, const std::vector<uint32_t>& colorAttachments, uint32_t depthAttachment = 0) override;
        void BindCustomFramebuffer(s32 handle) override;
        void DeleteCustomFramebuffer(s32 handle) override;

        // ---- Texture ----
        uint32_t CreateTexture2D(int width, int height, const void* data = nullptr,
                                 bool generateMipmaps = false, bool linearFilter = true,
                                 bool clampToEdge = true) override;
        void UpdateTexture2D(uint32_t textureId, int width, int height,
                             const void* data) override;
        void ReleaseTexture(uint32_t textureId) override;

        // ---- Shader ----
        std::expected<uint32_t, std::string>
        CreateShaderProgram(std::string_view vsSource,
                            std::string_view fsSource) override;
        void ReleaseShaderProgram(uint32_t programId) override;

        // ---- Uniform query ----
        int32_t GetUniformLocation(uint32_t programId,
                                   std::string_view name) override;

        // ---- Vertex Arrays ----
        uint32_t CreateVertexArray() override;
        void ReleaseVertexArray(uint32_t vao) override;

        // ---- Per-frame UBO updates ----
        void UpdateCameraUBO(const shine::render::RenderingData& renderingData);
        void UpdateLightUBO(const shine::render::RenderingData& renderingData);
    };

#endif // SHINE_OPENGL

}
