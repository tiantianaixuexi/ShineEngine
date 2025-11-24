#pragma once


#include <GL/glew.h>


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <array>
#include <vector>
#include <functional>



#include "shine_define.h"
#include "render/command/command_list.h"

namespace shine::render::backend
{
    class IRenderBackend
    {
	public:
	    virtual ~IRenderBackend() = default;
	    //  初始化
		virtual int  init(HWND hwnd, WNDCLASSEXW& wc)   = 0 ;

		//  初始化 Imgui 后端
		virtual void InitImguiBackend(HWND hwnd)   = 0;

		//  Imgui 新帧
		virtual void ImguiNewFrame()  = 0;

		//  初始化设备
		virtual bool CreateDevice(HWND hwnd)  = 0;

		//  清理设备
		virtual void CleanupDevice(HWND hwnd)  = 0;

		//  创建帧缓冲
		virtual bool CreateFrameBuffer()  = 0;

		//  渲染屏幕
		virtual void RenderScene(float deltaTime = 0.016f) = 0;

        //  渲染场景到帧缓冲
        virtual void RenderSceneToFrameBuffer() = 0;

		//  渲染到帧缓冲
		virtual void RenderToFramebuffer(std::array<float, 4> clear_color) = 0;


		virtual void CompileShaders() = 0;

		virtual void ReSizeFrameBuffer(int width, int height) = 0;

        virtual void RenderSceneToViewport(s32 handle) = 0;

        //  使用回调提交渲染命令（由外部负责记录绘制，而非后端硬编码）
        virtual void RenderSceneWith(s32 handle,const std::function<void(shine::render::command::ICommandList&)> &record) = 0;

		// 多视口/FBO 管理（可选实现）
		virtual s32 CreateViewport(int width, int height) { (void)width; (void)height; return 1; }
		virtual void DestroyViewport(s32 /*handle*/) {}
		virtual void ResizeViewport(s32 /*handle*/, int /*width*/, int /*height*/) {}
		virtual void BindViewport(s32 /*handle*/) {}
		virtual unsigned int GetViewportTexture(s32 /*handle*/) { return GetFramebufferTexture(); }

		//  清理
		virtual void ClearUp(HWND hwnd) = 0;


		virtual unsigned int GetFramebufferTexture() = 0;


		int g_Width = 800;
		int g_Height = 600;
		GLuint  my_image_texture = 0;
		size_t my_image_height = 200;
		size_t my_image_width = 200;
		std::vector<unsigned char >data;
    };



}

