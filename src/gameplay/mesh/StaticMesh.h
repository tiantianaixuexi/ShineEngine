#pragma once

#include <memory>


#include "GL/glew.h"



#include "shine_define.h"
#include "render/material.h"
#include "render/command/command_list.h"


namespace shine::gameplay
{
    // 构建静态网格，内部存放一个 VAO/VBO，支持三角形
    class StaticMesh
    {
    public:
        StaticMesh() = default;
        ~StaticMesh()
        {
#ifdef SHINE_OPENGL
            if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
            if (m_VBO) glDeleteBuffers(1, &m_VBO);
#endif
        }

        void initTriangle()
        {
#ifdef SHINE_OPENGL
            m_VertexCount = 3;
            const GLfloat vertices[] = {
                -0.5f, -0.5f, 0.0f,
                 0.5f, -0.5f, 0.0f,
                 0.0f,  0.5f, 0.0f
            };

            if (!m_VAO) glGenVertexArrays(1, &m_VAO);
            glBindVertexArray(m_VAO);

            if (!m_VBO) glGenBuffers(1, &m_VBO);
            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
#endif
        }

        // 在 XY 平面绘制一个正方形（两个三角形），每个顶点包含位置(vec3) + 法线(vec3)
        void initQuadWithNormals()
        {
#ifdef SHINE_OPENGL
            // 6 椤剁偣锛堜袱涓笁瑙掑舰锛夛紝娉曠嚎缁熶竴涓?+Z
            m_VertexCount = 6;
            const GLfloat vertices[] = {
                // pos                // normal
                -0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
                 0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
                 0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,

                -0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
                 0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
                -0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
            };

            if (!m_VAO) glGenVertexArrays(1, &m_VAO);
            glBindVertexArray(m_VAO);

            if (!m_VBO) glGenBuffers(1, &m_VBO);
            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            // layout(location=0) -> position
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
            glEnableVertexAttribArray(0);
            // layout(location=1) -> normal
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
#endif
        }

        // 绘制一个正方体（单位长宽高，中心点原点），每个面具有其面法线
        void initCubeWithNormals()
        {
#ifdef SHINE_OPENGL
            // 6 涓潰 脳 姣忛潰 2 涓笁瑙掑舰 脳 姣忎笁瑙掑舰 3 椤剁偣 = 36 椤剁偣
            m_VertexCount = 36;
            const GLfloat vertices[] = {
                // 浣嶇疆                // 娉曠嚎
                // +X 闈?(鍙?
                 0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
                 0.5f,  0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,    1.0f, 0.0f, 0.0f,

                 0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,    1.0f, 0.0f, 0.0f,
                 0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 0.0f,

                // -X 闈?(宸?
                -0.5f, -0.5f,  0.5f,   -1.0f, 0.0f, 0.0f,
                -0.5f,  0.5f,  0.5f,   -1.0f, 0.0f, 0.0f,
                -0.5f,  0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,

                -0.5f, -0.5f,  0.5f,   -1.0f, 0.0f, 0.0f,
                -0.5f,  0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,
                -0.5f, -0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,

                // +Y 闈?(涓?
                -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,
                 0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,

                -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,
                -0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,

                // -Y 闈?(涓?
                -0.5f, -0.5f,  0.5f,    0.0f,-1.0f, 0.0f,
                 0.5f, -0.5f,  0.5f,    0.0f,-1.0f, 0.0f,
                 0.5f, -0.5f, -0.5f,    0.0f,-1.0f, 0.0f,

                -0.5f, -0.5f,  0.5f,    0.0f,-1.0f, 0.0f,
                 0.5f, -0.5f, -0.5f,    0.0f,-1.0f, 0.0f,
                -0.5f, -0.5f, -0.5f,    0.0f,-1.0f, 0.0f,

                // +Z 闈?(鍓?
                -0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
                 0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
                 0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,

                -0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
                 0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
                -0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,

                // -Z 闈?(鍚?
                -0.5f,  0.5f, -0.5f,    0.0f, 0.0f,-1.0f,
                 0.5f,  0.5f, -0.5f,    0.0f, 0.0f,-1.0f,
                 0.5f, -0.5f, -0.5f,    0.0f, 0.0f,-1.0f,

                -0.5f,  0.5f, -0.5f,    0.0f, 0.0f,-1.0f,
                 0.5f, -0.5f, -0.5f,    0.0f, 0.0f,-1.0f,
                -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,-1.0f,
            };

            if (!m_VAO) glGenVertexArrays(1, &m_VAO);
            glBindVertexArray(m_VAO);

            if (!m_VBO) glGenBuffers(1, &m_VBO);
            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
#endif
        }

        // 渲染，绑定着色器（用后端编译缓存），提交渲染
        void render(shine::render::command::ICommandList& cmd)
        {
#ifdef SHINE_OPENGL
            if (!m_VAO || m_VertexCount <= 0) return;
            if (!m_Material) m_Material = shine::render::Material::GetDefaultPhong();
            if (m_Material) m_Material->bind(cmd);

            cmd.bindVertexArray(static_cast<u64>(m_VAO));
            cmd.drawTriangles(0, m_VertexCount);
#endif
        }

        // 材质接口
        void setMaterial(std::shared_ptr<shine::render::Material> mat) { m_Material = std::move(mat); }
        std::shared_ptr<shine::render::Material> getMaterial() const { return m_Material; }

        [[nodiscard]] u64 vaoHandle() const { return static_cast<u64>(m_VAO); }
        [[nodiscard]] int vertexCount() const { return m_VertexCount; }

    private:

#ifdef SHINE_OPENGL
        unsigned int m_VAO { 0 };
        unsigned int m_VBO { 0 };
        std::shared_ptr<shine::render::Material> m_Material;
#else
        unsigned int m_VAO { 0 };
        unsigned int m_VBO { 0 };
        std::shared_ptr<shine::render::Material> m_Material;
#endif
        int m_VertexCount { 0 };
    };
}

