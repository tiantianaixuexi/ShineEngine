#pragma once

#include "shine_define.h"


#include <memory>
#include <unordered_set>
#include <functional>

#include <GL/glew.h>


#include "imgui.h"
#include "render/render_backend.h"



namespace shine::render::opengl3
{

#ifdef SHINE_OPENGL

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

        // Command List (OpenGL immediate-mode implementation)
        std::unique_ptr<command::ICommandList> m_CommandList;

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
        virtual unsigned int GetViewportTexture(s32 handle) override;

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
        virtual void RenderSceneWith(s32 handle,
                                     const std::function<void(shine::render::command::ICommandList&)> &record) override;

        // 每帧更新相机UBO
        void UpdateCameraUBO();
        // 每帧更新光照UBO
        void UpdateLightUBO();
	};

}

#endif

