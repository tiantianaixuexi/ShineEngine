#include "opengl_command_list.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/glew.h>


#include "shine_define.h"
#include "imgui.h" // for draw data type only

namespace shine::render::opengl3
{
    void OpenGLCommandList::begin() { /* no-op for immediate mode */ }
    void OpenGLCommandList::end() { /* no-op */ }
    void OpenGLCommandList::execute() { /* no recorded buffer; immediate */ }
    void OpenGLCommandList::reset() { /* no-op */ }

    void OpenGLCommandList::bindFramebuffer(u64 framebufferHandle)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(framebufferHandle));
    }

    void OpenGLCommandList::setViewport(s32 x, s32 y, s32 width, s32 height)
    {
        glViewport(x, y, width, height);
    }

    void OpenGLCommandList::clearColor(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
    }

    void OpenGLCommandList::clear(bool clearColorBuffer, bool clearDepthBuffer)
    {
        GLbitfield mask = 0;
        if (clearColorBuffer) mask |= GL_COLOR_BUFFER_BIT;
        if (clearDepthBuffer) mask |= GL_DEPTH_BUFFER_BIT;
        glClear(mask);
    }

    void OpenGLCommandList::enableDepthTest(bool enabled)
    {
        if (enabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    }

    void OpenGLCommandList::useProgram(u64 programHandle)
    {
        glUseProgram(static_cast<GLuint>(programHandle));
    }

    void OpenGLCommandList::bindVertexArray(u64 vaoHandle)
    {
        glBindVertexArray(static_cast<GLuint>(vaoHandle));
    }

    void OpenGLCommandList::drawTriangles(s32 firstVertex, s32 vertexCount)
    {
        glDrawArrays(GL_TRIANGLES, firstVertex, vertexCount);
    }

    void OpenGLCommandList::drawIndexedTriangles(s32 indexCount, command::IndexType indexType,
                                                 u64 indexBufferOffsetBytes)
    {
        // 安全检查：索引数量必须大于0
        if (indexCount <= 0)
        {
            return; // 无效的索引数量，直接返回
        }

        // 检查当前绑定的 VAO
        GLint currentVAO = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
        if (currentVAO == 0)
        {
            // 没有绑定 VAO，glDrawElements 会崩溃
            // 这通常表示在调用 drawIndexedTriangles 之前没有调用 bindVertexArray
            return;
        }

        GLenum glIndexType = GL_UNSIGNED_INT;
        switch (indexType)
        {
        case command::IndexType::Uint16: glIndexType = GL_UNSIGNED_SHORT; break;
        case command::IndexType::Uint32: glIndexType = GL_UNSIGNED_INT; break;
        default:
            return; // 无效的索引类型
        }
        
        const void* offsetPtr = reinterpret_cast<const void*>(static_cast<uintptr_t>(indexBufferOffsetBytes));
        
        // 检查 OpenGL 错误状态
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            // 清除之前的错误状态
            while (glGetError() != GL_NO_ERROR) {}
        }
        
        glDrawElements(GL_TRIANGLES, indexCount, glIndexType, offsetPtr);
        
        // 检查绘制后的错误
        error = glGetError();
        if (error != GL_NO_ERROR)
        {
            // OpenGL 错误：可能是索引缓冲区未绑定或其他问题
            // 这里可以添加日志记录
        }
    }

    void OpenGLCommandList::setUniform1f(s32 location, float value)
    {
        if (location >= 0)
        {
            glUniform1f(location, value);
        }
    }

    void OpenGLCommandList::setUniform3f(s32 location, float x, float y, float z)
    {
        if (location >= 0)
        {
            glUniform3f(location, x, y, z);
        }
    }

    void OpenGLCommandList::imguiRender(void* drawData)
    {
        // ImGui OpenGL3 backend expects ImDrawData*
        extern void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);
        ImGui_ImplOpenGL3_RenderDrawData(static_cast<ImDrawData*>(drawData));
    }

    void OpenGLCommandList::swapBuffers(void* nativeSwapContext)
    {
        // nativeSwapContext is HDC on Windows for WGL
        ::SwapBuffers(static_cast<HDC>(nativeSwapContext));
    }
}


