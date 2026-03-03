#pragma once

#include "render/command/render_commands.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/glew.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h" 

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
            const GLuint target = static_cast<GLuint>(cmd.framebufferHandle);
            if (m_HasFramebuffer && m_LastFramebuffer == target) return;
            m_HasFramebuffer = true;
            m_LastFramebuffer = target;
            glBindFramebuffer(GL_FRAMEBUFFER, target);
        }

        void operator()(const CmdBindTexture& cmd)
        {
            const GLuint texture = static_cast<GLuint>(cmd.textureHandle);
            if (cmd.unit < m_TextureBound.size())
            {
                if (!m_HasActiveTexture || m_ActiveTextureUnit != cmd.unit)
                {
                    glActiveTexture(GL_TEXTURE0 + cmd.unit);
                    m_HasActiveTexture = true;
                    m_ActiveTextureUnit = cmd.unit;
                }
                if (m_TextureBound[cmd.unit] && m_TextureHandle[cmd.unit] == texture) return;
                m_TextureBound[cmd.unit] = true;
                m_TextureHandle[cmd.unit] = texture;
            }
            else
            {
                glActiveTexture(GL_TEXTURE0 + cmd.unit);
            }
            glBindTexture(GL_TEXTURE_2D, texture);
        }

        void operator()(const CmdSetViewport& cmd)
        {
            if (m_HasViewport &&
                m_ViewportX == cmd.x &&
                m_ViewportY == cmd.y &&
                m_ViewportW == cmd.width &&
                m_ViewportH == cmd.height)
            {
                return;
            }
            m_HasViewport = true;
            m_ViewportX = cmd.x;
            m_ViewportY = cmd.y;
            m_ViewportW = cmd.width;
            m_ViewportH = cmd.height;
            glViewport(cmd.x, cmd.y, cmd.width, cmd.height);
        }

        // Clear / state
        void operator()(const CmdClearColor& cmd)
        {
            if (m_HasClearColor &&
                m_ClearR == cmd.r &&
                m_ClearG == cmd.g &&
                m_ClearB == cmd.b &&
                m_ClearA == cmd.a)
            {
                return;
            }
            m_HasClearColor = true;
            m_ClearR = cmd.r;
            m_ClearG = cmd.g;
            m_ClearB = cmd.b;
            m_ClearA = cmd.a;
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
            if (m_HasDepthTest && m_DepthTestEnabled == cmd.enabled) return;
            m_HasDepthTest = true;
            m_DepthTestEnabled = cmd.enabled;
            if (cmd.enabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        }

        // Pipeline / geometry
        void operator()(const CmdUseProgram& cmd)
        {
            const GLuint program = static_cast<GLuint>(cmd.programHandle);
            if (m_HasProgram && m_CurrentProgram == program) return;
            m_HasProgram = true;
            m_CurrentProgram = program;
            InvalidateUniformCache();
            glUseProgram(program);
        }

        void operator()(const CmdBindVertexArray& cmd)
        {
            const GLuint vao = static_cast<GLuint>(cmd.vaoHandle);
            if (m_HasVao && m_CurrentVao == vao) return;
            m_HasVao = true;
            m_CurrentVao = vao;
            glBindVertexArray(vao);
        }

        void operator()(const CmdDrawTriangles& cmd)
        {
            glDrawArrays(GL_TRIANGLES, cmd.firstVertex, cmd.vertexCount);
        }

        void operator()(const CmdDrawIndexedTriangles& cmd)
        {
            if (cmd.indexCount <= 0) return;

            GLenum glIndexType = GL_UNSIGNED_INT;
            switch (cmd.indexType)
            {
            case IndexType::Uint16: glIndexType = GL_UNSIGNED_SHORT; break;
            case IndexType::Uint32: glIndexType = GL_UNSIGNED_INT; break;
            default: return;
            }
            const void* offsetPtr = reinterpret_cast<const void*>(static_cast<uintptr_t>(cmd.indexBufferOffsetBytes));
            glDrawElements(GL_TRIANGLES, cmd.indexCount, glIndexType, offsetPtr);
        }

        // Uniforms
        void operator()(const CmdSetUniform1f& cmd)
        {
            if (cmd.location < 0) return;
            if (m_HasUniform1f && m_Uniform1fLocation == cmd.location && m_Uniform1fValue == cmd.value) return;
            m_HasUniform1f = true;
            m_Uniform1fLocation = cmd.location;
            m_Uniform1fValue = cmd.value;
            glUniform1f(cmd.location, cmd.value);
        }

        void operator()(const CmdSetUniform1i& cmd)
        {
            if (cmd.location < 0) return;
            if (m_HasUniform1i && m_Uniform1iLocation == cmd.location && m_Uniform1iValue == cmd.value) return;
            m_HasUniform1i = true;
            m_Uniform1iLocation = cmd.location;
            m_Uniform1iValue = cmd.value;
            glUniform1i(cmd.location, cmd.value);
        }

        void operator()(const CmdSetUniform2f& cmd)
        {
            if (cmd.location < 0) return;
            if (m_HasUniform2f &&
                m_Uniform2fLocation == cmd.location &&
                m_Uniform2fX == cmd.x &&
                m_Uniform2fY == cmd.y)
            {
                return;
            }
            m_HasUniform2f = true;
            m_Uniform2fLocation = cmd.location;
            m_Uniform2fX = cmd.x;
            m_Uniform2fY = cmd.y;
            glUniform2f(cmd.location, cmd.x, cmd.y);
        }

        void operator()(const CmdSetUniform3f& cmd)
        {
            if (cmd.location < 0) return;
            if (m_HasUniform3f &&
                m_Uniform3fLocation == cmd.location &&
                m_Uniform3fX == cmd.x &&
                m_Uniform3fY == cmd.y &&
                m_Uniform3fZ == cmd.z)
            {
                return;
            }
            m_HasUniform3f = true;
            m_Uniform3fLocation = cmd.location;
            m_Uniform3fX = cmd.x;
            m_Uniform3fY = cmd.y;
            m_Uniform3fZ = cmd.z;
            glUniform3f(cmd.location, cmd.x, cmd.y, cmd.z);
        }

        void operator()(const CmdSetUniform4f& cmd)
        {
            if (cmd.location < 0) return;
            if (m_HasUniform4f &&
                m_Uniform4fLocation == cmd.location &&
                m_Uniform4fX == cmd.x &&
                m_Uniform4fY == cmd.y &&
                m_Uniform4fZ == cmd.z &&
                m_Uniform4fW == cmd.w)
            {
                return;
            }
            m_HasUniform4f = true;
            m_Uniform4fLocation = cmd.location;
            m_Uniform4fX = cmd.x;
            m_Uniform4fY = cmd.y;
            m_Uniform4fZ = cmd.z;
            m_Uniform4fW = cmd.w;
            glUniform4f(cmd.location, cmd.x, cmd.y, cmd.z, cmd.w);
        }

        void operator()(const CmdSetUniformMatrix4fv& cmd)
        {
            if (cmd.location < 0) return;
            const bool transpose = cmd.transpose;
            if (m_HasUniformMatrix4fv &&
                m_UniformMatrix4fvLocation == cmd.location &&
                m_UniformMatrix4fvTranspose == transpose &&
                m_UniformMatrix4fvData == cmd.data)
            {
                return;
            }
            m_HasUniformMatrix4fv = true;
            m_UniformMatrix4fvLocation = cmd.location;
            m_UniformMatrix4fvTranspose = transpose;
            m_UniformMatrix4fvData = cmd.data;
            glUniformMatrix4fv(cmd.location, 1, transpose ? GL_TRUE : GL_FALSE, cmd.data.data());
        }

        // UI
        void operator()(const CmdImguiRender& cmd)
        {
            ImGui_ImplOpenGL3_RenderDrawData(static_cast<ImDrawData*>(cmd.drawData));
        }

        // Present
        void operator()(const CmdSwapBuffers& cmd)
        {
            ::SwapBuffers(static_cast<HDC>(cmd.nativeSwapContext));
        }

    private:
        void InvalidateUniformCache()
        {
            m_HasUniform1f = false;
            m_HasUniform1i = false;
            m_HasUniform2f = false;
            m_HasUniform3f = false;
            m_HasUniform4f = false;
            m_HasUniformMatrix4fv = false;
        }

        bool m_HasProgram = false;
        GLuint m_CurrentProgram = 0;
        bool m_HasVao = false;
        GLuint m_CurrentVao = 0;
        bool m_HasDepthTest = false;
        bool m_DepthTestEnabled = false;
        bool m_HasFramebuffer = false;
        GLuint m_LastFramebuffer = 0;
        bool m_HasViewport = false;
        s32 m_ViewportX = 0;
        s32 m_ViewportY = 0;
        s32 m_ViewportW = 0;
        s32 m_ViewportH = 0;
        bool m_HasClearColor = false;
        float m_ClearR = 0.0f;
        float m_ClearG = 0.0f;
        float m_ClearB = 0.0f;
        float m_ClearA = 0.0f;
        bool m_HasActiveTexture = false;
        u32 m_ActiveTextureUnit = 0;
        std::array<bool, 32> m_TextureBound{};
        std::array<GLuint, 32> m_TextureHandle{};
        bool m_HasUniform1f = false;
        s32 m_Uniform1fLocation = -1;
        float m_Uniform1fValue = 0.0f;
        bool m_HasUniform1i = false;
        s32 m_Uniform1iLocation = -1;
        s32 m_Uniform1iValue = 0;
        bool m_HasUniform2f = false;
        s32 m_Uniform2fLocation = -1;
        float m_Uniform2fX = 0.0f;
        float m_Uniform2fY = 0.0f;
        bool m_HasUniform3f = false;
        s32 m_Uniform3fLocation = -1;
        float m_Uniform3fX = 0.0f;
        float m_Uniform3fY = 0.0f;
        float m_Uniform3fZ = 0.0f;
        bool m_HasUniform4f = false;
        s32 m_Uniform4fLocation = -1;
        float m_Uniform4fX = 0.0f;
        float m_Uniform4fY = 0.0f;
        float m_Uniform4fZ = 0.0f;
        float m_Uniform4fW = 0.0f;
        bool m_HasUniformMatrix4fv = false;
        s32 m_UniformMatrix4fvLocation = -1;
        bool m_UniformMatrix4fvTranspose = false;
        std::array<float, 16> m_UniformMatrix4fvData{};
    };
}
