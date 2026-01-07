#pragma once

#include "render/command/render_commands.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/glew.h>
#include "imgui/imgui.h" 

namespace shine::render::backend::gl
{
    using namespace shine::render::command;

    // Shared Executor for OpenGL 3.3+ and OpenGL ES 3.0 (WebGL2)
    struct GLExecutor
    {
        // Lifecycle
        void operator()(const CmdBegin&) { /* no-op */ }
        void operator()(const CmdEnd&) { /* no-op */ }
        void operator()(const CmdExecute&) { /* no-op */ }
        void operator()(const CmdReset&) { /* no-op */ }

        // Frame / target
        void operator()(const CmdBindFramebuffer& cmd)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(cmd.framebufferHandle));
        }

        void operator()(const CmdSetViewport& cmd)
        {
            glViewport(cmd.x, cmd.y, cmd.width, cmd.height);
        }

        // Clear / state
        void operator()(const CmdClearColor& cmd)
        {
            glClearColor(cmd.r, cmd.g, cmd.b, cmd.a);
        }

        void operator()(const CmdClear& cmd)
        {
            GLbitfield mask = 0;
            if (cmd.clearColorBuffer) mask |= GL_COLOR_BUFFER_BIT;
            if (cmd.clearDepthBuffer) mask |= GL_DEPTH_BUFFER_BIT;
            glClear(mask);
        }

        void operator()(const CmdEnableDepthTest& cmd)
        {
            if (cmd.enabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        }

        // Pipeline / geometry
        void operator()(const CmdUseProgram& cmd)
        {
            glUseProgram(static_cast<GLuint>(cmd.programHandle));
        }

        void operator()(const CmdBindVertexArray& cmd)
        {
            glBindVertexArray(static_cast<GLuint>(cmd.vaoHandle));
        }

        void operator()(const CmdDrawTriangles& cmd)
        {
            glDrawArrays(GL_TRIANGLES, cmd.firstVertex, cmd.vertexCount);
        }

        void operator()(const CmdDrawIndexedTriangles& cmd)
        {
             // Safety check
            if (cmd.indexCount <= 0) return;

            // Check VAO
            GLint currentVAO = 0;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
            if (currentVAO == 0) return;

            GLenum glIndexType = GL_UNSIGNED_INT;
            switch (cmd.indexType)
            {
            case IndexType::Uint16: glIndexType = GL_UNSIGNED_SHORT; break;
            case IndexType::Uint32: glIndexType = GL_UNSIGNED_INT; break;
            default: return;
            }
            
            const void* offsetPtr = reinterpret_cast<const void*>(static_cast<uintptr_t>(cmd.indexBufferOffsetBytes));
            
            // Clear errors
            while (glGetError() != GL_NO_ERROR) {}
            
            glDrawElements(GL_TRIANGLES, cmd.indexCount, glIndexType, offsetPtr);
            
            // Check errors (logging omitted for brevity/performance in visitor, but could be added)
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                // Log error
            }
        }

        // Uniforms
        void operator()(const CmdSetUniform1f& cmd)
        {
            if (cmd.location >= 0) glUniform1f(cmd.location, cmd.value);
        }

        void operator()(const CmdSetUniform3f& cmd)
        {
            if (cmd.location >= 0) glUniform3f(cmd.location, cmd.x, cmd.y, cmd.z);
        }

        // UI
        void operator()(const CmdImguiRender& cmd)
        {
            extern void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);
            ImGui_ImplOpenGL3_RenderDrawData(static_cast<ImDrawData*>(cmd.drawData));
        }

        // Present
        void operator()(const CmdSwapBuffers& cmd)
        {
            ::SwapBuffers(static_cast<HDC>(cmd.nativeSwapContext));
        }
    };
}
