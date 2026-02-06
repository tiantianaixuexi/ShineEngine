#include "opengl3_backend.h"

#include <memory>
#include <fmt/format.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "manager/CameraManager.h"
#include "manager/light_manager.h"
#include "render/backend/gl/gl_executor.h"
#include "render/pipeline/command_buffer.h"
#include "EngineCore/engine_context.h"

namespace shine::render::opengl3
{
#ifdef SHINE_OPENGL

    int OpenGLRenderBackend::init(backend::NativeWindowHandle window, void* platformData)
    {
        HWND hwnd = static_cast<HWND>(window);
        auto* wc  = static_cast<WNDCLASSEXW*>(platformData);

        if (!CreateDevice(window))
        {
            CleanupDevice(window);
            ::DestroyWindow(hwnd);
            if (wc) ::UnregisterClassW(wc->lpszClassName, wc->hInstance);
            return 1;
        }
        wglMakeCurrent(g_hdc, g_hRC);

        // Initialize GLEW
        GLenum err = glewInit();
        if (GLEW_OK != err)
        {
            fmt::println("GLEW初始化错误: {}", reinterpret_cast<const char*>(glewGetErrorString(err)));
            CleanupDevice(window);
            wglDeleteContext(g_hRC);
            ::DestroyWindow(hwnd);
            if (wc) ::UnregisterClassW(wc->lpszClassName, wc->hInstance);
            return 1;
        }

        fmt::println("GLEW版本: {}", reinterpret_cast<const char*>(glewGetString(GLEW_VERSION)));

        // Check framebuffer extension
        if (!GLEW_ARB_framebuffer_object)
        {
            fmt::println("您的显卡不支持帧缓冲对象扩展，无法创建渲染目标");
            CleanupDevice(window);
            wglDeleteContext(g_hRC);
            ::DestroyWindow(hwnd);
            if (wc) ::UnregisterClassW(wc->lpszClassName, wc->hInstance);
            return 1;
        }

        // Create global camera UBO, binding = 0
        if (!m_CameraUbo) {
            glGenBuffers(1, &m_CameraUbo);
            glBindBuffer(GL_UNIFORM_BUFFER, m_CameraUbo);
            glBufferData(GL_UNIFORM_BUFFER, 96, nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_CameraUbo);
        }
        // Create global light UBO, binding = 1
        if (!m_LightUbo) {
            glGenBuffers(1, &m_LightUbo);
            glBindBuffer(GL_UNIFORM_BUFFER, m_LightUbo);
            glBufferData(GL_UNIFORM_BUFFER, 3 * 16, nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_LightUbo);
        }
        return 0;
    }

    void OpenGLRenderBackend::InitImguiBackend(backend::NativeWindowHandle window)
    {
        ImGui_ImplWin32_InitForOpenGL(static_cast<HWND>(window));
        ImGui_ImplOpenGL3_Init();
    }

    void OpenGLRenderBackend::ImguiNewFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    bool OpenGLRenderBackend::CreateDevice(backend::NativeWindowHandle window)
    {
        HWND hwnd = static_cast<HWND>(window);
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

    void OpenGLRenderBackend::CleanupDevice(backend::NativeWindowHandle window)
    {
        HWND hwnd = static_cast<HWND>(window);
        wglMakeCurrent(nullptr, nullptr);
        ::ReleaseDC(hwnd, g_hdc);
    }

    bool OpenGLRenderBackend::CreateFrameBuffer()
    {
        // Clean up existing framebuffer
        if (g_FramebufferObject != 0)
        {
            glDeleteFramebuffers(1, &g_FramebufferObject);
            glDeleteTextures(1, &g_FramebufferTexture);
            glDeleteRenderbuffers(1, &g_DepthRenderbuffer);
            g_FramebufferObject = 0;
            g_FramebufferTexture = 0;
            g_DepthRenderbuffer = 0;
        }

        glGenFramebuffers(1, &g_FramebufferObject);
        glBindFramebuffer(GL_FRAMEBUFFER, g_FramebufferObject);

        glGenTextures(1, &g_FramebufferTexture);
        glBindTexture(GL_TEXTURE_2D, g_FramebufferTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_FramebufferTexture, 0);

        glGenRenderbuffers(1, &g_DepthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, g_DepthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_Width, m_Height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_DepthRenderbuffer);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            fmt::println("帧缓冲创建错误，错误码: 0x{:x}", status);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        fmt::println("成功创建帧缓冲，大小：{}x{}", m_Width, m_Height);
        return true;
    }

    void OpenGLRenderBackend::RenderScene(float deltaTime)
    {
        glViewport(0, 0, m_Width, m_Height);
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
    }

    void OpenGLRenderBackend::CompileShaders()
    {
    }

    void OpenGLRenderBackend::RenderSceneToFrameBuffer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, g_FramebufferObject);
        RenderScene(0.016f);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderBackend::RenderSceneToViewport(s32 handle)
    {
        auto it = m_Viewports.find(handle);
        if (it == m_Viewports.end()) { RenderSceneToFrameBuffer(); return; }
        glBindFramebuffer(GL_FRAMEBUFFER, it->second.fbo);
        RenderScene(0.016f);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderBackend::ExecuteCommandBuffer(s32 handle, const shine::render::CommandBuffer* cmdBuffer)
    {
        if (!cmdBuffer) return;

        auto bindFbo = [&](s32 h) {
            auto it = m_Viewports.find(h);
            if (it == m_Viewports.end())
                glBindFramebuffer(GL_FRAMEBUFFER, g_FramebufferObject);
            else
                glBindFramebuffer(GL_FRAMEBUFFER, it->second.fbo);
        };

        bindFbo(handle);

        int vpW = m_Width, vpH = m_Height;
        if (auto it = m_Viewports.find(handle); it != m_Viewports.end()) {
            vpW = it->second.width;
            vpH = it->second.height;
        }

        // Default state
        glViewport(0, 0, vpW, vpH);
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        UpdateCameraUBO();
        UpdateLightUBO();

        // Execute commands via visitor
        shine::render::backend::gl::GLExecutor executor;
        for (const auto& cmd : cmdBuffer->GetCommands())
        {
            std::visit(executor, cmd);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderBackend::RenderToFramebuffer(const std::array<float, 4>& clear_color)
    {
        RenderSceneToFrameBuffer();

        glViewport(0, 0, m_Width, m_Height);
        glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ::SwapBuffers(g_hdc);
    }

    unsigned int OpenGLRenderBackend::GetFramebufferTexture()
    {
        return g_FramebufferTexture;
    }

    // ---- Size ----
    void OpenGLRenderBackend::SetSize(int width, int height)
    {
        m_Width  = width;
        m_Height = height;
    }

    std::pair<int,int> OpenGLRenderBackend::GetSize() const noexcept
    {
        return { m_Width, m_Height };
    }

    // ---- Viewport management ----
    s32 OpenGLRenderBackend::CreateViewport(int width, int height)
    {
        GLuint color = 0, depth = 0, fbo = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(1, &color);
        glBindTexture(GL_TEXTURE_2D, color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

        glGenRenderbuffers(1, &depth);
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            if (color) glDeleteTextures(1, &color);
            if (depth) glDeleteRenderbuffers(1, &depth);
            if (fbo)   glDeleteFramebuffers(1, &fbo);
            return 0;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        s32 handle = m_NextViewportHandle++;
        m_Viewports.emplace(handle, ViewportInfo(fbo, color, depth, width, height));
        return handle;
    }

    void OpenGLRenderBackend::DestroyViewport(s32 handle)
    {
        auto it = m_Viewports.find(handle);
        if (it == m_Viewports.end()) return;
        if (it->second.fbo)   glDeleteFramebuffers(1, &it->second.fbo);
        if (it->second.color) glDeleteTextures(1, &it->second.color);
        if (it->second.depth) glDeleteRenderbuffers(1, &it->second.depth);
        m_Viewports.erase(it);
    }

    void OpenGLRenderBackend::ResizeViewport(s32 handle, int width, int height)
    {
        auto it = m_Viewports.find(handle);
        if (it == m_Viewports.end()) return;
        glBindTexture(GL_TEXTURE_2D, it->second.color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glBindRenderbuffer(GL_RENDERBUFFER, it->second.depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        it->second.width = width;
        it->second.height = height;
    }

    void OpenGLRenderBackend::BindViewport(s32 handle)
    {
        auto it = m_Viewports.find(handle);
        if (it == m_Viewports.end()) {
            glBindFramebuffer(GL_FRAMEBUFFER, g_FramebufferObject);
            return;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, it->second.fbo);
    }

    unsigned long long OpenGLRenderBackend::GetViewportTexture(u32 handle)
    {
        auto it = m_Viewports.find(handle);
        if (it == m_Viewports.end()) return GetFramebufferTexture();
        return it->second.color;
    }

    void OpenGLRenderBackend::ReSizeFrameBuffer(int width, int height)
    {
        if (g_FramebufferObject != 0) {
            m_Width  = width;
            m_Height = height;
            CreateFrameBuffer();
        }
    }

    void OpenGLRenderBackend::ClearUp(backend::NativeWindowHandle window)
    {
        HWND hwnd = static_cast<HWND>(window);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDevice(window);
        wglDeleteContext(g_hRC);
    }

    void OpenGLRenderBackend::UpdateCameraUBO()
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

    void OpenGLRenderBackend::UpdateLightUBO()
    {
        if (!m_LightUbo) return;
        auto& lm = shine::manager::LightManager::get();
        const float dir4[4]   = { lm.directional().dir[0], lm.directional().dir[1], lm.directional().dir[2], 0.0f };
        const float color4[4] = { lm.directional().color[0], lm.directional().color[1], lm.directional().color[2], 1.0f };
        const float inten4[4] = { lm.directional().intensity, 0.0f, 0.0f, 0.0f };
        glBindBuffer(GL_UNIFORM_BUFFER, m_LightUbo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0,  16, dir4);
        glBufferSubData(GL_UNIFORM_BUFFER, 16, 16, color4);
        glBufferSubData(GL_UNIFORM_BUFFER, 32, 16, inten4);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_LightUbo);
    }

    // ---- Texture ----
    uint32_t OpenGLRenderBackend::CreateTexture2D(int width, int height, const void* data,
        bool generateMipmaps, bool linearFilter, bool clampToEdge)
    {
        if (width <= 0 || height <= 0) return 0;

        GLuint textureId = 0;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        GLint minFilter = linearFilter ? (generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR) : GL_NEAREST;
        GLint magFilter = linearFilter ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

        GLint wrapMode = clampToEdge ? GL_CLAMP_TO_EDGE : GL_REPEAT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            static_cast<GLsizei>(width), static_cast<GLsizei>(height),
            0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        if (generateMipmaps && data != nullptr)
            glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
        return static_cast<uint32_t>(textureId);
    }

    void OpenGLRenderBackend::UpdateTexture2D(uint32_t textureId, int width, int height, const void* data)
    {
        if (textureId == 0 || width <= 0 || height <= 0) return;
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void OpenGLRenderBackend::ReleaseTexture(uint32_t textureId)
    {
        if (textureId != 0) {
            GLuint glId = static_cast<GLuint>(textureId);
            glDeleteTextures(1, &glId);
        }
    }

    // ---- Shader ----
    std::expected<uint32_t, std::string>
    OpenGLRenderBackend::CreateShaderProgram(std::string_view vsSource,
                                             std::string_view fsSource)
    {
        GLint ok = 0;

        // Vertex shader
        GLuint v = glCreateShader(GL_VERTEX_SHADER);
        const char* vsSrc = vsSource.data();
        GLint vsLen = static_cast<GLint>(vsSource.size());
        glShaderSource(v, 1, &vsSrc, &vsLen);
        glCompileShader(v);
        glGetShaderiv(v, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048]{};
            glGetShaderInfoLog(v, 2048, nullptr, log);
            glDeleteShader(v);
            return std::unexpected(std::string("VS:") + log);
        }

        // Fragment shader
        GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fsSrc = fsSource.data();
        GLint fsLen = static_cast<GLint>(fsSource.size());
        glShaderSource(f, 1, &fsSrc, &fsLen);
        glCompileShader(f);
        glGetShaderiv(f, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048]{};
            glGetShaderInfoLog(f, 2048, nullptr, log);
            glDeleteShader(v);
            glDeleteShader(f);
            return std::unexpected(std::string("FS:") + log);
        }

        // Link
        GLuint prog = glCreateProgram();
        glAttachShader(prog, v);
        glAttachShader(prog, f);
        glLinkProgram(prog);
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[2048]{};
            glGetProgramInfoLog(prog, 2048, nullptr, log);
            glDeleteShader(v);
            glDeleteShader(f);
            glDeleteProgram(prog);
            return std::unexpected(std::string("LK:") + log);
        }
        glDeleteShader(v);
        glDeleteShader(f);

        // Bind CameraUBO if present
        GLuint blockIndex = glGetUniformBlockIndex(prog, "CameraUBO");
        if (blockIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(prog, blockIndex, 0);
        }

        return static_cast<uint32_t>(prog);
    }

    void OpenGLRenderBackend::ReleaseShaderProgram(uint32_t programId)
    {
        if (programId) glDeleteProgram(static_cast<GLuint>(programId));
    }

    // ---- Uniform query ----
    int32_t OpenGLRenderBackend::GetUniformLocation(uint32_t programId,
                                                     std::string_view name)
    {
        if (programId == 0) return -1;
        // glGetUniformLocation needs a null-terminated string.
        // string_view from string literals / std::string is already null-terminated in practice,
        // but to be safe we pass the data pointer (which is fine for our use case).
        return static_cast<int32_t>(
            glGetUniformLocation(static_cast<GLuint>(programId),
                                 name.data()));
    }

#endif // SHINE_OPENGL
}
