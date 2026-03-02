#pragma once

#include <memory>
#include <vector>
#include <cmath>


#include "GL/glew.h"



#include "shine_define.h"
#include "render/material.h"
#include "render/pipeline/command_buffer.h"


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

        void initSphereWithNormals(int segments = 24, int rings = 16)
        {
#ifdef SHINE_OPENGL
            if (segments < 3) segments = 3;
            if (rings < 2) rings = 2;
            const float radius = 0.5f;
            std::vector<GLfloat> vertices;
            vertices.reserve(segments * rings * 6 * 6);

            auto pushVertex = [&](float x, float y, float z) {
                float len = std::sqrt(x * x + y * y + z * z);
                float nx = x / len;
                float ny = y / len;
                float nz = z / len;
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
                vertices.push_back(nx);
                vertices.push_back(ny);
                vertices.push_back(nz);
            };

            const float pi = 3.14159265358979323846f;
            for (int y = 0; y < rings; ++y)
            {
                float v0 = static_cast<float>(y) / rings;
                float v1 = static_cast<float>(y + 1) / rings;
                float phi0 = v0 * pi;
                float phi1 = v1 * pi;

                for (int x = 0; x < segments; ++x)
                {
                    float u0 = static_cast<float>(x) / segments;
                    float u1 = static_cast<float>(x + 1) / segments;
                    float theta0 = u0 * pi * 2.0f;
                    float theta1 = u1 * pi * 2.0f;

                    float x00 = std::sin(phi0) * std::cos(theta0) * radius;
                    float y00 = std::cos(phi0) * radius;
                    float z00 = std::sin(phi0) * std::sin(theta0) * radius;

                    float x10 = std::sin(phi0) * std::cos(theta1) * radius;
                    float y10 = y00;
                    float z10 = std::sin(phi0) * std::sin(theta1) * radius;

                    float x01 = std::sin(phi1) * std::cos(theta0) * radius;
                    float y01 = std::cos(phi1) * radius;
                    float z01 = std::sin(phi1) * std::sin(theta0) * radius;

                    float x11 = std::sin(phi1) * std::cos(theta1) * radius;
                    float y11 = y01;
                    float z11 = std::sin(phi1) * std::sin(theta1) * radius;

                    pushVertex(x00, y00, z00);
                    pushVertex(x10, y10, z10);
                    pushVertex(x11, y11, z11);

                    pushVertex(x00, y00, z00);
                    pushVertex(x11, y11, z11);
                    pushVertex(x01, y01, z01);
                }
            }

            m_VertexCount = static_cast<int>(vertices.size() / 6);
            if (!m_VAO) glGenVertexArrays(1, &m_VAO);
            glBindVertexArray(m_VAO);

            if (!m_VBO) glGenBuffers(1, &m_VBO);
            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
#endif
        }

        // 渲染，绑定着色器（用后端编译缓存），提交渲染
        void render(render::CommandBuffer& cmd)
        {
#ifdef SHINE_OPENGL
            if (!m_VAO || m_VertexCount <= 0) return;
            if (!m_Material) m_Material = shine::render::Material::GetDefaultPhong();
            if (m_Material) m_Material->bind(cmd);

            cmd.BindVertexArray(static_cast<u64>(m_VAO));
            cmd.DrawTriangles(0, m_VertexCount);
#endif
        }

        // 材质接口
        void setMaterial(std::shared_ptr<shine::render::Material> mat) { 
            m_Material = std::move(mat); 
        }
        std::shared_ptr<shine::render::Material> getMaterial() const { return m_Material; }

        [[nodiscard]] u64 vaoHandle() const { return static_cast<u64>(m_VAO); }
        [[nodiscard]] int vertexCount() const { return m_VertexCount; }

    private:

        unsigned int m_VAO { 0 };
        unsigned int m_VBO { 0 };
        std::shared_ptr<shine::render::Material> m_Material;
        
        int m_VertexCount { 0 };
    };
}

