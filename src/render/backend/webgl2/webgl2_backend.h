#pragma once

#include "shine_define.h"

#include <memory>
#include <unordered_set>
#include <functional>
#include <unordered_map>

// WebGL2 uses OpenGL ES 3.0 API
#include <GL/glew.h>

#include "imgui.h"
#include "render/core/render_backend.h"

namespace shine::render::webgl2
{

#ifdef SHINE_WEBGL2

struct ViewportInfo {
    GLuint fbo{0};
    GLuint color{0};
    GLuint depth{0};
    int width{0};
    int height{0};

    ViewportInfo() : fbo(0), color(0), depth(0), width(0), height(0) {}
    ViewportInfo(GLuint f, GLuint c, GLuint d, int w, int h)
        : fbo(f), color(c), depth(d), width(w), height(h) {}
    ViewportInfo(const ViewportInfo&) = default;
    ViewportInfo(ViewportInfo&&) = default;
    ViewportInfo& operator=(const ViewportInfo&) = default;
    ViewportInfo& operator=(ViewportInfo&&) = default;
};

class WebGL2RenderBackend : public backend::IRenderBackend
{
public:
    // Framebuffer objects
    GLuint g_FramebufferObject = 0;
    GLuint g_FramebufferTexture = 0;
    GLuint g_DepthRenderbuffer = 0;

    // Windows OpenGL context
    HGLRC g_hRC = nullptr;
    HDC g_hdc = nullptr;

    // Global camera UBO, std140, binding=0
    GLuint m_CameraUbo = 0;
    // Global light UBO, std140, binding=1
    GLuint m_LightUbo = 0;

    // Command List (WebGL2 immediate-mode implementation)
    std::unique_ptr<command::ICommandList> m_CommandList;

    // Multi-viewport FBO registry
    std::unordered_map<s32, ViewportInfo> m_Viewports;
    s32 m_NextViewportHandle{1};

    // Initialization
    virtual int init(HWND hwnd, WNDCLASSEXW& wc);

    // Initialize ImGui backend
    virtual void InitImguiBackend(HWND hwnd);

    // ImGui new frame
    virtual void ImguiNewFrame();

    // Initialize device
    virtual bool CreateDevice(HWND hwnd);

    // Cleanup device
    virtual void CleanupDevice(HWND hwnd);

    // Create framebuffer
    virtual bool CreateFrameBuffer();

    // Render scene
    virtual void RenderScene(float deltaTime = 0.016f);

    // Render scene to framebuffer (default FBO)
    virtual void RenderSceneToFrameBuffer();
    // Render scene to specified viewport FBO (multi-viewport)
    virtual void RenderSceneToViewport(s32 handle) override;

    // Render to framebuffer
    virtual void RenderToFramebuffer(std::array<float, 4> clear_color);

    virtual unsigned int GetFramebufferTexture();
    virtual s32 CreateViewport(int width, int height) override;
    virtual void DestroyViewport(s32 handle) override;
    virtual void ResizeViewport(s32 handle, int width, int height) override;
    virtual void BindViewport(s32 handle) override;
    virtual unsigned int GetViewportTexture(s32 handle) override;

    virtual void ReSizeFrameBuffer(int width, int height);

    // Compile/link shaders and create VAO/VBO
    virtual void CompileShaders();

    // Cleanup
    virtual void ClearUp(HWND hwnd);

    virtual int getWidth() const;
    virtual int getHeight() const;

    virtual void setWidth(int width);
    virtual void setHeight(int height);

    // Callback-based rendering interface implementation
    virtual void RenderSceneWith(s32 handle,
                                 const std::function<void(shine::render::command::ICommandList&)> &record) override;

    // Update camera UBO per frame
    void UpdateCameraUBO();
    // Update light UBO per frame
    void UpdateLightUBO();

    // ========================================================================
    // Texture creation interface implementation
    // ========================================================================

    virtual uint32_t CreateTexture2D(int width, int height, const void* data = nullptr,
        bool generateMipmaps = false, bool linearFilter = true, bool clampToEdge = true) override;

    virtual void UpdateTexture2D(uint32_t textureId, int width, int height, const void* data) override;

    virtual void ReleaseTexture(uint32_t textureId) override;
};

#endif

}

