#include "webgl2_command_list.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// WebGL2 uses OpenGL ES 3.0 API
// On Windows, we can use ANGLE or EGL to provide OpenGL ES 3.0 support
// For now, we'll use GLEW with OpenGL ES 3.0 extensions
#include <GL/glew.h>

#include "shine_define.h"
#include "imgui.h" // for draw data type only

namespace shine::render::webgl2
{
    void WebGL2CommandList::begin() { /* no-op for immediate mode */ }
    void WebGL2CommandList::end() { /* no-op */ }
    void WebGL2CommandList::execute() { /* no recorded buffer; immediate */ }
    void WebGL2CommandList::reset() { /* no-op */ }

    void WebGL2CommandList::bindFramebuffer(u64 framebufferHandle)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(framebufferHandle));
    }

    void WebGL2CommandList::setViewport(s32 x, s32 y, s32 width, s32 height)
    {
        glViewport(x, y, width, height);
    }

    void WebGL2CommandList::clearColor(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
    }

    void WebGL2CommandList::clear(bool clearColorBuffer, bool clearDepthBuffer)
    {
        GLbitfield mask = 0;
        if (clearColorBuffer) mask |= GL_COLOR_BUFFER_BIT;
        if (clearDepthBuffer) mask |= GL_DEPTH_BUFFER_BIT;
        glClear(mask);
    }

    void WebGL2CommandList::enableDepthTest(bool enabled)
    {
        if (enabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    }

    void WebGL2CommandList::useProgram(u64 programHandle)
    {
        glUseProgram(static_cast<GLuint>(programHandle));
    }

    void WebGL2CommandList::bindVertexArray(u64 vaoHandle)
    {
        // WebGL2/OpenGL ES 3.0 supports VAO
        glBindVertexArray(static_cast<GLuint>(vaoHandle));
    }

    void WebGL2CommandList::drawTriangles(s32 firstVertex, s32 vertexCount)
    {
        glDrawArrays(GL_TRIANGLES, firstVertex, vertexCount);
    }

    void WebGL2CommandList::drawIndexedTriangles(s32 indexCount, command::IndexType indexType,
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

    void WebGL2CommandList::setUniform1f(s32 location, float value)
    {
        if (location >= 0)
        {
            glUniform1f(location, value);
        }
    }

    void WebGL2CommandList::setUniform3f(s32 location, float x, float y, float z)
    {
        if (location >= 0)
        {
            glUniform3f(location, x, y, z);
        }
    }

    void WebGL2CommandList::imguiRender(void* drawData)
    {
        // ImGui OpenGL3 backend can work with WebGL2/OpenGL ES 3.0
        extern void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);
        ImGui_ImplOpenGL3_RenderDrawData(static_cast<ImDrawData*>(drawData));
    }

    void WebGL2CommandList::swapBuffers(void* nativeSwapContext)
    {
        // nativeSwapContext is HDC on Windows for WGL
        ::SwapBuffers(static_cast<HDC>(nativeSwapContext));
    }
}

