#include "webgl2_backend.h"

#include <memory>
#include <fmt/format.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "manager/CameraManager.h"
#include "manager/light_manager.h"
#include "render/pipeline/command_buffer.h"
#include "render/backend/gl/gl_executor.h"
#include "EngineCore/engine_context.h"

namespace shine::render::webgl2
{
#ifdef SHINE_WEBGL2

    int WebGL2RenderBackend::init(backend::NativeWindowHandle window, void* platformData)
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

        GLenum err = glewInit();
        if (GLEW_OK != err)
        {
            fmt::println("GLEW initialization error: {}", reinterpret_cast<const char*>(glewGetErrorString(err)));
            CleanupDevice(window);
            wglDeleteContext(g_hRC);
            ::DestroyWindow(hwnd);
            if (wc) ::UnregisterClassW(wc->lpszClassName, wc->hInstance);
            return 1;
        }

        fmt::println("GLEW version: {}", reinterpret_cast<const char*>(glewGetString(GLEW_VERSION)));

        if (!GLEW_ARB_framebuffer_object && !GLEW_EXT_framebuffer_object)
        {
            fmt::println("Graphics card does not support framebuffer objects");
            CleanupDevice(window);
            wglDeleteContext(g_hRC);
            ::DestroyWindow(hwnd);
            if (wc) ::UnregisterClassW(wc->lpszClassName, wc->hInstance);
            return 1;
        }

        fmt::println("WebGL2 backend initialized (using OpenGL ES 3.0 API)");

        if (!m_CameraUbo) {
            glGenBuffers(1, &m_CameraUbo);
            glBindBuffer(GL_UNIFORM_BUFFER, m_CameraUbo);
            glBufferData(GL_UNIFORM_BUFFER, 96, nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_CameraUbo);
        }
        if (!m_LightUbo) {
            glGenBuffers(1, &m_LightUbo);
            glBindBuffer(GL_UNIFORM_BUFFER, m_LightUbo);
            glBufferData(GL_UNIFORM_BUFFER, 3 * 16, nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_LightUbo);
        }
        return 0;
    }

    void WebGL2RenderBackend::InitImguiBackend(backend::NativeWindowHandle window)
    {
        ImGui_ImplWin32_InitForOpenGL(static_cast<HWND>(window));
        extern bool ImGui_ImplOpenGL3_Init(const char* glsl_version);
        ImGui_ImplOpenGL3_Init("#version 300 es");
    }

    void WebGL2RenderBackend::ImguiNewFrame()
    {
        extern void ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    bool WebGL2RenderBackend::CreateDevice(backend::NativeWindowHandle window)
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
        if (pf == 0) return false;
        if (::SetPixelFormat(hDc, pf, &pfd) == false) return false;
        ::ReleaseDC(hwnd, hDc);

        g_hdc = ::GetDC(hwnd);
        if (!g_hRC)
            g_hRC = wglCreateContext(g_hdc);
        return true;
    }

    void WebGL2RenderBackend::CleanupDevice(backend::NativeWindowHandle window)
    {
        HWND hwnd = static_cast<HWND>(window);
        wglMakeCurrent(nullptr, nullptr);
        ::ReleaseDC(hwnd, g_hdc);
    }

    bool WebGL2RenderBackend::CreateFrameBuffer()
    {
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_FramebufferTexture, 0);

        glGenRenderbuffers(1, &g_DepthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, g_DepthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_Width, m_Height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_DepthRenderbuffer);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            fmt::println("Framebuffer creation error: 0x{:x}", status);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        fmt::println("Successfully created framebuffer: {}x{}", m_Width, m_Height);
        return true;
    }

    void WebGL2RenderBackend::RenderScene(float deltaTime)
    {
        glViewport(0, 0, m_Width, m_Height);
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
    }

    void WebGL2RenderBackend::CompileShaders() {}

    void WebGL2RenderBackend::RenderSceneToFrameBuffer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, g_FramebufferObject);
        RenderScene(0.016f);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void WebGL2RenderBackend::RenderSceneToViewport(s32 handle)
    {
        auto it = m_Viewports.find(handle);
        if (it == m_Viewports.end()) { RenderSceneToFrameBuffer(); return; }
        glBindFramebuffer(GL_FRAMEBUFFER, it->second.fbo);
        RenderScene(0.016f);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void WebGL2RenderBackend::ExecuteCommandBuffer(s32 handle, const shine::render::CommandBuffer* cmdBuffer)
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

        glViewport(0, 0, vpW, vpH);
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        UpdateCameraUBO();
        UpdateLightUBO();

        shine::render::backend::gl::GLExecutor executor;
        for (const auto& cmd : cmdBuffer->GetCommands())
        {
            std::visit(executor, cmd);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void WebGL2RenderBackend::RenderToFramebuffer(const std::array<float, 4>& clear_color)
    {
        RenderSceneToFrameBuffer();
        glViewport(0, 0, m_Width, m_Height);
        glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ::SwapBuffers(g_hdc);
    }

    unsigned int WebGL2RenderBackend::GetFramebufferTexture()
    {
        return g_FramebufferTexture;
    }

    void WebGL2RenderBackend::SetSize(int width, int height)
    {
        m_Width  = width;
        m_Height = height;
    }

    std::pair<int,int> WebGL2RenderBackend::GetSize() const noexcept
    {
        return { m_Width, m_Height };
    }

    s32 WebGL2RenderBackend::CreateViewport(int width, int height)
    {
        GLuint color = 0, depth = 0, fbo = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(1, &color);
        glBindTexture(GL_TEXTURE_2D, color);
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
            if (fbo)   glDeleteFramebuffers(1, &fbo);
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
        if (it->second.fbo)   glDeleteFramebuffers(1, &it->second.fbo);
        if (it->second.color) glDeleteTextures(1, &it->second.color);
        if (it->second.depth) glDeleteRenderbuffers(1, &it->second.depth);
        m_Viewports.erase(it);
    }

    void WebGL2RenderBackend::ResizeViewport(s32 handle, int width, int height)
    {
        auto it = m_Viewports.find(handle);
        if (it == m_Viewports.end()) return;
        glBindTexture(GL_TEXTURE_2D, it->second.color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindRenderbuffer(GL_RENDERBUFFER, it->second.depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        it->second.width  = width;
        it->second.height = height;
    }

    void WebGL2RenderBackend::BindViewport(s32 handle)
    {
        auto it = m_Viewports.find(handle);
        if (it == m_Viewports.end()) {
            glBindFramebuffer(GL_FRAMEBUFFER, g_FramebufferObject);
            return;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, it->second.fbo);
    }

    unsigned long long WebGL2RenderBackend::GetViewportTexture(u32 handle)
    {
        auto it = m_Viewports.find(handle);
        if (it == m_Viewports.end()) return GetFramebufferTexture();
        return it->second.color;
    }

    void WebGL2RenderBackend::ReSizeFrameBuffer(int width, int height)
    {
        if (g_FramebufferObject != 0) {
            m_Width  = width;
            m_Height = height;
            CreateFrameBuffer();
        }
    }

    void WebGL2RenderBackend::ClearUp(backend::NativeWindowHandle window)
    {
        HWND hwnd = static_cast<HWND>(window);

        extern void ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        for (auto& [h, viewport] : m_Viewports)
        {
            if (viewport.fbo)   glDeleteFramebuffers(1, &viewport.fbo);
            if (viewport.color) glDeleteTextures(1, &viewport.color);
            if (viewport.depth) glDeleteRenderbuffers(1, &viewport.depth);
        }
        m_Viewports.clear();

        if (g_FramebufferObject)  glDeleteFramebuffers(1, &g_FramebufferObject);
        if (g_FramebufferTexture) glDeleteTextures(1, &g_FramebufferTexture);
        if (g_DepthRenderbuffer)  glDeleteRenderbuffers(1, &g_DepthRenderbuffer);
        g_FramebufferObject = 0;
        g_FramebufferTexture = 0;
        g_DepthRenderbuffer = 0;

        if (m_CameraUbo) glDeleteBuffers(1, &m_CameraUbo);
        if (m_LightUbo)  glDeleteBuffers(1, &m_LightUbo);
        m_CameraUbo = 0;
        m_LightUbo = 0;

        CleanupDevice(window);
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
        glBufferSubData(GL_UNIFORM_BUFFER, 0,  16, dir4);
        glBufferSubData(GL_UNIFORM_BUFFER, 16, 16, color4);
        glBufferSubData(GL_UNIFORM_BUFFER, 32, 16, inten4);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_LightUbo);
    }

    // ---- Texture ----
    uint32_t WebGL2RenderBackend::CreateTexture2D(int width, int height, const void* data,
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
            static_cast<GLsizei>(width), static_cast<GLsizei>(height),
            0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        if (generateMipmaps && data != nullptr)
            glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
        return static_cast<uint32_t>(textureId);
    }

    void WebGL2RenderBackend::UpdateTexture2D(uint32_t textureId, int width, int height, const void* data)
    {
        if (textureId == 0 || width <= 0 || height <= 0) return;
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void WebGL2RenderBackend::ReleaseTexture(uint32_t textureId)
    {
        if (textureId != 0) {
            GLuint glId = static_cast<GLuint>(textureId);
            glDeleteTextures(1, &glId);
        }
    }

    u64 WebGL2RenderBackend::CreateMesh(const backend::MeshCreateInfo& info)
    {
        if (!info.vertexData || info.vertexDataSize == 0 || info.vertexCount <= 0) return 0;
        if (info.layout.strideBytes == 0 || info.layout.attributes.empty()) return 0;

        GLuint vao = 0;
        GLuint vbo = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, info.vertexDataSize, info.vertexData, GL_STATIC_DRAW);

        for (const auto& attr : info.layout.attributes)
        {
            glVertexAttribPointer(attr.location, attr.components, GL_FLOAT, GL_FALSE,
                                  info.layout.strideBytes,
                                  reinterpret_cast<const void*>(static_cast<uintptr_t>(attr.offsetBytes)));
            glEnableVertexAttribArray(attr.location);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_MeshVbos[vao] = vbo;
        return static_cast<u64>(vao);
    }

    void WebGL2RenderBackend::ReleaseMesh(u64 handle)
    {
        if (handle == 0) return;
        const GLuint vao = static_cast<GLuint>(handle);
        if (const auto it = m_MeshVbos.find(vao); it != m_MeshVbos.end())
        {
            const GLuint vbo = it->second;
            if (vbo) glDeleteBuffers(1, &vbo);
            m_MeshVbos.erase(it);
        }
        glDeleteVertexArrays(1, &vao);
    }

    // ---- Shader ----
    std::expected<uint32_t, std::string>
    WebGL2RenderBackend::CreateShaderProgram(std::string_view vsSource,
                                             std::string_view fsSource)
    {
        GLint ok = 0;

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

        GLuint blockIndex = glGetUniformBlockIndex(prog, "CameraUBO");
        if (blockIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(prog, blockIndex, 0);
        }

        return static_cast<uint32_t>(prog);
    }

    void WebGL2RenderBackend::ReleaseShaderProgram(uint32_t programId)
    {
        if (programId) glDeleteProgram(static_cast<GLuint>(programId));
    }

    int32_t WebGL2RenderBackend::GetUniformLocation(uint32_t programId,
                                                     std::string_view name)
    {
        if (programId == 0) return -1;
        return static_cast<int32_t>(
            glGetUniformLocation(static_cast<GLuint>(programId),
                                 name.data()));
    }

#endif // SHINE_WEBGL2
}
