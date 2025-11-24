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
        GLenum glIndexType = GL_UNSIGNED_INT;
        switch (indexType)
        {
        case command::IndexType::Uint16: glIndexType = GL_UNSIGNED_SHORT; break;
        case command::IndexType::Uint32: glIndexType = GL_UNSIGNED_INT; break;
        }
        const void* offsetPtr = reinterpret_cast<const void*>(static_cast<uintptr_t>(indexBufferOffsetBytes));
        glDrawElements(GL_TRIANGLES, indexCount, glIndexType, offsetPtr);
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


