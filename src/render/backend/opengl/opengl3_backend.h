#pragma once

#include "shine_define.h"


#include <memory>
#include <unordered_set>
#include <functional>

#include <GL/glew.h>

#include "render/backend/render_backend.h"
#include "render/backend/gl/gl_common.h"


namespace shine::render::opengl3
{

#ifdef SHINE_OPENGL

    using ViewportInfo = shine::render::backend::gl::ViewportInfo;

    class OpenGLRenderBackend : public backend::IRenderBackend
	{
	public:
		//  一些参数

		GLuint g_FramebufferObject = 0;
		GLuint g_FramebufferTexture = 0;
		GLuint g_DepthRenderbuffer = 0;

		HGLRC g_hRC;
		HDC g_hdc;



        // 全局相机UBO，std140, binding=0
        GLuint m_CameraUbo = 0;
        // 全局光照UBO，std140, binding=1
        GLuint m_LightUbo = 0;

        // Command List removed (stateless visitor used instead)

        // 多视口FBO注册表
        std::unordered_map<s32, ViewportInfo> m_Viewports;
        s32 m_NextViewportHandle{1};

		//  初始化
		virtual int  init(HWND hwnd, WNDCLASSEXW& wc);


		//  初始化Imgui后端
		virtual void InitImguiBackend(HWND hwnd);

		//  Imgui 新帧
		virtual void ImguiNewFrame();

		//  初始化设备
		virtual bool CreateDevice(HWND hwnd);

		//  清理设备
		virtual void CleanupDevice(HWND hwnd);

		//  创建帧缓冲
		virtual bool CreateFrameBuffer();

		//  渲染场景
		virtual void RenderScene(float deltaTime = 0.016f);

        //  渲染场景到帧缓冲（默认FBO）
        virtual void RenderSceneToFrameBuffer();
        //  渲染场景到指定视口FBO（多视口）
        virtual void RenderSceneToViewport(s32 handle) override;

		//  渲染到帧缓冲
        virtual void RenderToFramebuffer(std::array<float, 4> clear_color);

        virtual unsigned int GetFramebufferTexture();
        virtual s32 CreateViewport(int width, int height) override;
        virtual void DestroyViewport(s32 handle) override;
        virtual void ResizeViewport(s32 handle, int width, int height) override;
        virtual void BindViewport(s32 handle) override;
        virtual unsigned long long  GetViewportTexture(u32 handle) override;

		virtual void ReSizeFrameBuffer(int width, int height);

        // 编译/链接着色器并创建VAO/VBO
        virtual void CompileShaders();

		//  清理
		virtual void ClearUp(HWND hwnd);


		virtual int getWidth() const;
		virtual int getHeight() const;


		virtual void setWidth(int width);
		virtual void setHeight(int height);

        // 回调式渲染接口实现
        virtual void ExecuteCommandBuffer(s32 viewportHandle, const shine::render::CommandBuffer* cmdBuffer) override;

        // 每帧更新相机UBO
        void UpdateCameraUBO();
        // 每帧更新光照UBO
        void UpdateLightUBO();

        // ========================================================================
        // 纹理创建接口实现
        // ========================================================================

        virtual uint32_t CreateTexture2D(int width, int height, const void* data = nullptr,
            bool generateMipmaps = false, bool linearFilter = true, bool clampToEdge = true) override;

        virtual void UpdateTexture2D(uint32_t textureId, int width, int height, const void* data) override;

        virtual void ReleaseTexture(uint32_t textureId) override;

        // Shader creation interface implementation
        virtual uint32_t CreateShaderProgram(const char* vsSource, const char* fsSource, std::string& outLog) override;
        virtual void ReleaseShaderProgram(uint32_t programId) override;
	};

}

#endif
