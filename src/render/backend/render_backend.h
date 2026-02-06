#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "shine_define.h"

namespace shine::render {
class CommandBuffer;
}

namespace shine::render::backend {

// ============================================================================
// Platform-opaque handle — cast to HWND / NSWindow* / etc. in implementations
// ============================================================================
using NativeWindowHandle = void*;

// ============================================================================
// IRenderBackend — graphics-API-agnostic interface
//   * No GL / Win32 / Vulkan includes
//   * No public data members — all state lives in concrete implementations
// ============================================================================
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // ---- Lifecycle ---------------------------------------------------------
    virtual int  init(NativeWindowHandle window, void* platformData = nullptr) = 0;
    virtual void InitImguiBackend(NativeWindowHandle window) = 0;
    virtual void ImguiNewFrame() = 0;
    virtual bool CreateDevice(NativeWindowHandle window) = 0;
    virtual void CleanupDevice(NativeWindowHandle window) = 0;
    virtual void ClearUp(NativeWindowHandle window) = 0;

    // ---- Framebuffer -------------------------------------------------------
    virtual bool         CreateFrameBuffer() = 0;
    virtual void         ReSizeFrameBuffer(int width, int height) = 0;
    virtual unsigned int GetFramebufferTexture() = 0;

    // ---- Size --------------------------------------------------------------
    virtual void SetSize(int width, int height) = 0;
    [[nodiscard]] virtual std::pair<int,int> GetSize() const noexcept = 0;

    // ---- Scene rendering ---------------------------------------------------
    virtual void RenderScene(float deltaTime = 0.016f) = 0;
    virtual void RenderSceneToFrameBuffer() = 0;
    virtual void RenderSceneToViewport(s32 handle) = 0;
    virtual void RenderToFramebuffer(const std::array<float, 4>& clear_color) = 0;

    // ---- Shaders -----------------------------------------------------------
    virtual void CompileShaders() = 0;

    // ---- Command buffer execution ------------------------------------------
    virtual void ExecuteCommandBuffer(s32 viewportHandle,
                                      const shine::render::CommandBuffer* cmdBuffer) = 0;

    // ---- Multi-viewport / FBO management (optional overrides) ---------------
    virtual s32                CreateViewport(int width, int height) { return 1; }
    virtual void               DestroyViewport(s32 /*handle*/) {}
    virtual void               ResizeViewport(s32 /*handle*/, int /*w*/, int /*h*/) {}
    virtual void               BindViewport(s32 /*handle*/) {}
    virtual unsigned long long  GetViewportTexture(u32 /*handle*/) {
        return GetFramebufferTexture();
    }

    // ---- Texture creation (cross-API) --------------------------------------
    virtual uint32_t CreateTexture2D(int width, int height,
                                     const void* data        = nullptr,
                                     bool generateMipmaps    = false,
                                     bool linearFilter       = true,
                                     bool clampToEdge        = true) = 0;
    virtual void     UpdateTexture2D(uint32_t textureId, int width, int height,
                                     const void* data) = 0;
    virtual void     ReleaseTexture(uint32_t textureId) = 0;

    // ---- Shader program (cross-API) ----------------------------------------
    // Returns program id on success, or error string on failure.
    virtual std::expected<uint32_t, std::string>
    CreateShaderProgram(std::string_view vsSource,
                        std::string_view fsSource) = 0;

    virtual void ReleaseShaderProgram(uint32_t programId) = 0;

    // ---- Uniform query (cross-API) -----------------------------------------
    // Returns uniform location for a given program; -1 if not found.
    virtual int32_t GetUniformLocation(uint32_t programId,
                                       std::string_view name) = 0;
};

} // namespace shine::render::backend
