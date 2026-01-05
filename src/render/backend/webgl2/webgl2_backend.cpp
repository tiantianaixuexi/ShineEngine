#include "webgl2_backend.h"

#include <memory>
#include <fmt/format.h>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_opengl3.h>

#include "manager/CameraManager.h"
#include "manager/light_manager.h"
#include "render/command/webgl2_command_list.h"
#include "../../EngineCore/engine_context.h"

extern shine::EngineContext* g_EngineContext;

namespace shine::render::webgl2
{
#ifdef SHINE_WEBGL2

int WebGL2RenderBackend::init(HWND hwnd, WNDCLASSEXW& wc)
{
    if (!CreateDevice(hwnd))
    {
        CleanupDevice(hwnd);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    wglMakeCurrent(g_hdc, g_hRC);

    // Initialize GLEW
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        fmt::println("GLEW initialization error: {}", reinterpret_cast<const char*>(glewGetErrorString(err)));
        CleanupDevice(hwnd);
        wglDeleteContext(g_hRC);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    fmt::println("GLEW version: {}", reinterpret_cast<const char*>(glewGetString(GLEW_VERSION)));

    // Check for OpenGL ES 3.0 or WebGL2 support
    // WebGL2 is based on OpenGL ES 3.0, so we check for ES 3.0 features
    // Note: On Windows, this backend assumes OpenGL ES 3.0 context is available via ANGLE or similar
    if (!GLEW_ARB_framebuffer_object && !GLEW_EXT_framebuffer_object)
    {
        fmt::println("Graphics card does not support framebuffer objects, cannot create render targets");
        CleanupDevice(hwnd);
        wglDeleteContext(g_hRC);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    fmt::println("WebGL2 backend initialized (using OpenGL ES 3.0 API)");

    // Create command list
    m_CommandList = std::make_unique<WebGL2CommandList>();

    // Create global camera UBO, binding = 0
    if (!m_CameraUbo) {
        glGenBuffers(1, &m_CameraUbo);
        glBindBuffer(GL_UNIFORM_BUFFER, m_CameraUbo);
        // std140 alignment: mat4 64 bytes + vec4 16 bytes, reserve 96 bytes
        glBufferData(GL_UNIFORM_BUFFER, 96, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_CameraUbo);
    }
    // Create global light UBO, binding = 1: dir(vec4) + color(vec4) + intensity(vec4)
    if (!m_LightUbo) {
        glGenBuffers(1, &m_LightUbo);
        glBindBuffer(GL_UNIFORM_BUFFER, m_LightUbo);
        glBufferData(GL_UNIFORM_BUFFER, 3 * 16, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_LightUbo);
    }
    return 0;
}

void WebGL2RenderBackend::InitImguiBackend(HWND hwnd)
{
    ImGui_ImplWin32_InitForOpenGL(hwnd);
    // Use OpenGL3 backend for ImGui (compatible with WebGL2/OpenGL ES 3.0)
    ImGui_ImplOpenGL3_Init("#version 300 es");
}

void WebGL2RenderBackend::ImguiNewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
}

bool WebGL2RenderBackend::CreateDevice(HWND hwnd)
{
    HDC hDc = ::GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == false)
        return false;
    ::ReleaseDC(hwnd, hDc);

    g_hdc = ::GetDC(hwnd);
    if (!g_hRC)
        g_hRC = wglCreateContext(g_hdc);
    return true;
}

void WebGL2RenderBackend::CleanupDevice(HWND hwnd)
{
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hwnd, g_hdc);
}

bool WebGL2RenderBackend::CreateFrameBuffer()
{
    // Clean up any existing framebuffer
    if (g_FramebufferObject != 0)
    {
        glDeleteFramebuffers(1, &g_FramebufferObject);
        glDeleteTextures(1, &g_FramebufferTexture);
        glDeleteRenderbuffers(1, &g_DepthRenderbuffer);
        g_FramebufferObject = 0;
        g_FramebufferTexture = 0;
        g_DepthRenderbuffer = 0;
    }

    // Create framebuffer object
    glGenFramebuffers(1, &g_FramebufferObject);
    glBindFramebuffer(GL_FRAMEBUFFER, g_FramebufferObject);

    // Create texture attachment
    glGenTextures(1, &g_FramebufferTexture);
    glBindTexture(GL_TEXTURE_2D, g_FramebufferTexture);
    // WebGL2/OpenGL ES 3.0 uses GL_RGBA8 for internal format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, g_Width, g_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_FramebufferTexture, 0);

    // Create renderbuffer object as depth buffer
    glGenRenderbuffers(1, &g_DepthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, g_DepthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, g_Width, g_Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_DepthRenderbuffer);

    // Check if framebuffer is complete
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        fmt::println("Framebuffer creation error, error code: 0x{:x}", status);
        return false;
    }

    // Restore default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    fmt::println("Successfully created framebuffer, size: {}x{}", g_Width, g_Height);
    return true;
}

void WebGL2RenderBackend::RenderScene(float deltaTime)
{
    // Default rendering flow, clear screen and set viewport, but don't handle specific object rendering
    m_CommandList->begin();
    m_CommandList->setViewport(0, 0, g_Width, g_Height);
    m_CommandList->clearColor(0.2f, 0.3f, 0.4f, 1.0f);
    m_CommandList->clear(true, true);
    m_CommandList->enableDepthTest(true);

    m_CommandList->end();
}

void WebGL2RenderBackend::CompileShaders()
{
    // Shader compilation is handled elsewhere
}

void WebGL2RenderBackend::RenderSceneToFrameBuffer()
{
    // Bind framebuffer and call common rendering flow
    m_CommandList->bindFramebuffer(g_FramebufferObject);
    RenderScene(0.016f);
    m_CommandList->bindFramebuffer(0);
}

void WebGL2RenderBackend::RenderSceneToViewport(s32 handle)
{
    auto it = m_Viewports.find(handle);
    if (it == m_Viewports.end()) { RenderSceneToFrameBuffer(); return; }
    m_CommandList->bindFramebuffer(it->second.fbo);
    RenderScene(0.016f);
    m_CommandList->bindFramebuffer(0);
}

void WebGL2RenderBackend::RenderSceneWith(s32 handle,
                                 const std::function<void(shine::render::command::ICommandList&)> &record)
{
    auto bindFbo = [&](s32 h){
        auto it = m_Viewports.find(h);
        if (it == m_Viewports.end())
            m_CommandList->bindFramebuffer(static_cast<std::uint64_t>(g_FramebufferObject));
        else
            m_CommandList->bindFramebuffer(static_cast<std::uint64_t>(it->second.fbo));
    };

    bindFbo(handle);
    // Uniformly clear screen and set camera UBO
    m_CommandList->begin();
    // Choose appropriate viewport size based on target FBO to avoid viewport confusion
    int vpW = g_Width, vpH = g_Height;
    {
        auto it2 = m_Viewports.find(handle);
        if (it2 != m_Viewports.end()) { vpW = it2->second.width; vpH = it2->second.height; }
    }
    m_CommandList->setViewport(0, 0, vpW, vpH);
    m_CommandList->clearColor(0.2f, 0.3f, 0.4f, 1.0f);
    m_CommandList->clear(true, true);
    m_CommandList->enableDepthTest(true);

    // Update related UBOs once per frame
    UpdateCameraUBO();
    UpdateLightUBO();

    // External code records correct rendering commands
    if (record) { record(*m_CommandList); }

    m_CommandList->end();
    m_CommandList->bindFramebuffer(0);
}

void WebGL2RenderBackend::RenderToFramebuffer(std::array<float, 4> clear_color)
{
    // Render to framebuffer
    RenderSceneToFrameBuffer();
    m_CommandList->setViewport(0, 0, g_Width, g_Height);
    m_CommandList->clearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    m_CommandList->clear(true, false);
    m_CommandList->imguiRender(ImGui::GetDrawData());
    // Present
    m_CommandList->swapBuffers(static_cast<void*>(g_hdc));
}

unsigned int WebGL2RenderBackend::GetFramebufferTexture()
{
    return g_FramebufferTexture;
}

s32 WebGL2RenderBackend::CreateViewport(int width, int height)
{
    // Create color texture
    GLuint color=0, depth=0, fbo=0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    // WebGL2/OpenGL ES 3.0 uses GL_RGBA8 for internal format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (color) glDeleteTextures(1, &color);
        if (depth) glDeleteRenderbuffers(1, &depth);
        if (fbo) glDeleteFramebuffers(1, &fbo);
        return 0;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    s32 handle = m_NextViewportHandle++;
    m_Viewports.emplace(handle, ViewportInfo(fbo, color, depth, width, height));
    return handle;
}

void WebGL2RenderBackend::DestroyViewport(s32 handle)
{
    auto it = m_Viewports.find(handle);
    if (it == m_Viewports.end()) return;
    if (it->second.fbo) glDeleteFramebuffers(1, &it->second.fbo);
    if (it->second.color) glDeleteTextures(1, &it->second.color);
    if (it->second.depth) glDeleteRenderbuffers(1, &it->second.depth);
    m_Viewports.erase(it);
}

void WebGL2RenderBackend::ResizeViewport(s32 handle, int width, int height)
{
    auto it = m_Viewports.find(handle);
    if (it == m_Viewports.end()) return;
    // Recreate attachments
    glBindTexture(GL_TEXTURE_2D, it->second.color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindRenderbuffer(GL_RENDERBUFFER, it->second.depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    it->second.width = width; it->second.height = height;
}

void WebGL2RenderBackend::BindViewport(s32 handle)
{
    auto it = m_Viewports.find(handle);
    if (it == m_Viewports.end()) { glBindFramebuffer(GL_FRAMEBUFFER, g_FramebufferObject); return; }
    glBindFramebuffer(GL_FRAMEBUFFER, it->second.fbo);
}

unsigned int WebGL2RenderBackend::GetViewportTexture(s32 handle)
{
    auto it = m_Viewports.find(handle);
    if (it == m_Viewports.end()) return GetFramebufferTexture();
    return it->second.color;
}

void WebGL2RenderBackend::ReSizeFrameBuffer(int width, int height)
{
    if (g_FramebufferObject != 0) {
        g_Width = width;
        g_Height = height;
        CreateFrameBuffer();
    }
}

void WebGL2RenderBackend::ClearUp(HWND hwnd)
{
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    // Cleanup WebGL2/OpenGL ES resources
    
    // Cleanup all viewport FBOs
    for (auto& [handle, viewport] : m_Viewports)
    {
        if (viewport.fbo) glDeleteFramebuffers(1, &viewport.fbo);
        if (viewport.color) glDeleteTextures(1, &viewport.color);
        if (viewport.depth) glDeleteRenderbuffers(1, &viewport.depth);
    }
    m_Viewports.clear();
    
    // Cleanup default framebuffer resources
    if (g_FramebufferObject) glDeleteFramebuffers(1, &g_FramebufferObject);
    if (g_FramebufferTexture) glDeleteTextures(1, &g_FramebufferTexture);
    if (g_DepthRenderbuffer) glDeleteRenderbuffers(1, &g_DepthRenderbuffer);
    g_FramebufferObject = 0;
    g_FramebufferTexture = 0;
    g_DepthRenderbuffer = 0;
    
    // Cleanup UBOs
    if (m_CameraUbo) glDeleteBuffers(1, &m_CameraUbo);
    if (m_LightUbo) glDeleteBuffers(1, &m_LightUbo);
    m_CameraUbo = 0;
    m_LightUbo = 0;

    CleanupDevice(hwnd);
    if (g_hRC) {
        wglDeleteContext(g_hRC);
        g_hRC = nullptr;
    }
}

void WebGL2RenderBackend::UpdateCameraUBO()
{
    auto* cam = shine::manager::CameraManager::get().getMainCamera();
    if (!cam || !m_CameraUbo) return;
    const math::FMatrix4d VP = cam->GetViewProjectionMatrixM();
    std::array<float, 16> vpFloat{};
    const double* src = VP.data();
    for (int i = 0; i < 16; ++i) vpFloat[i] = static_cast<float>(src[i]);
    glBindBuffer(GL_UNIFORM_BUFFER, m_CameraUbo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 64, vpFloat.data());
    float viewPos[4] = { (float)cam->position.X, (float)cam->position.Y, (float)cam->position.Z, 0.0f };
    glBufferSubData(GL_UNIFORM_BUFFER, 64, 16, viewPos);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_CameraUbo);
}

void WebGL2RenderBackend::UpdateLightUBO()
{
    if (!m_LightUbo) return;
    auto& lm = shine::manager::LightManager::get();
    const float dir4[4]   = { lm.directional().dir[0], lm.directional().dir[1], lm.directional().dir[2], 0.0f };
    const float color4[4] = { lm.directional().color[0], lm.directional().color[1], lm.directional().color[2], 1.0f };
    const float inten4[4] = { lm.directional().intensity, 0.0f, 0.0f, 0.0f };
    glBindBuffer(GL_UNIFORM_BUFFER, m_LightUbo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 16, dir4);
    glBufferSubData(GL_UNIFORM_BUFFER, 16, 16, color4);
    glBufferSubData(GL_UNIFORM_BUFFER, 32, 16, inten4);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_LightUbo);
}

int WebGL2RenderBackend::getWidth() const
{
    return g_Width;
}

int WebGL2RenderBackend::getHeight() const
{
    return g_Height;
}

void WebGL2RenderBackend::setWidth(int width)
{
    g_Width = width;
}

void WebGL2RenderBackend::setHeight(int height)
{
    g_Height = height;
}

uint32_t WebGL2RenderBackend::CreateTexture2D(int width, int height, const void* data,
    bool generateMipmaps, bool linearFilter, bool clampToEdge)
{
    if (width <= 0 || height <= 0)
    {
        return 0;
    }

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Set texture filter parameters
    GLint minFilter = linearFilter ? (generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR) : GL_NEAREST;
    GLint magFilter = linearFilter ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

    // Set texture wrap mode
    GLint wrapMode = clampToEdge ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

    // Upload texture data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    // WebGL2/OpenGL ES 3.0 uses GL_RGBA8 for internal format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Generate mipmap (if needed)
    if (generateMipmaps && data != nullptr)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return static_cast<uint32_t>(textureId);
}

void WebGL2RenderBackend::UpdateTexture2D(uint32_t textureId, int width, int height, const void* data)
{
    if (textureId == 0 || width <= 0 || height <= 0)
    {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));

    // Update texture data
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void WebGL2RenderBackend::ReleaseTexture(uint32_t textureId)
{
    if (textureId != 0)
    {
        GLuint glTextureId = static_cast<GLuint>(textureId);
        glDeleteTextures(1, &glTextureId);
    }
}

#endif
}

