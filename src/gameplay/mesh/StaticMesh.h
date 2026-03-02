#pragma once

#include <memory>
#include <vector>
#include <cmath>

#include "EngineCore/engine_context.h"
#include "shine_define.h"
#include "render/material.h"
#include "render/pipeline/command_buffer.h"
#include "render/renderer_service.h"


namespace shine::gameplay
{
    // 构建静态网格，内部存放一个 VAO/VBO，支持三角形
    class StaticMesh
    {
    public:
        StaticMesh() = default;
        ~StaticMesh()
        {
            releaseMesh();
        }

        void initTriangle()
        {
            m_VertexCount = 3;
            std::vector<float> vertices = {
                -0.5f, -0.5f, 0.0f,
                 0.5f, -0.5f, 0.0f,
                 0.0f,  0.5f, 0.0f
            };

            render::backend::VertexLayoutDesc layout;
            layout.strideBytes = 3 * sizeof(float);
            layout.attributes = { {0, 3, 0} };

            buildMesh(vertices, layout);
        }

        // 在 XY 平面绘制一个正方形（两个三角形），每个顶点包含位置(vec3) + 法线(vec3)
        void initQuadWithNormals()
        {
            m_VertexCount = 6;
            std::vector<float> vertices = {
                -0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
                 0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
                 0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,

                -0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
                 0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
                -0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
            };

            render::backend::VertexLayoutDesc layout;
            layout.strideBytes = 6 * sizeof(float);
            layout.attributes = { {0, 3, 0}, {1, 3, 3 * sizeof(float)} };

            buildMesh(vertices, layout);
        }

        // 绘制一个正方体（单位长宽高，中心点原点），每个面具有其面法线
        void initCubeWithNormals()
        {
            m_VertexCount = 36;
            std::vector<float> vertices = {
                 0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
                 0.5f,  0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,    1.0f, 0.0f, 0.0f,

                 0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,    1.0f, 0.0f, 0.0f,
                 0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 0.0f,

                -0.5f, -0.5f,  0.5f,   -1.0f, 0.0f, 0.0f,
                -0.5f,  0.5f,  0.5f,   -1.0f, 0.0f, 0.0f,
                -0.5f,  0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,

                -0.5f, -0.5f,  0.5f,   -1.0f, 0.0f, 0.0f,
                -0.5f,  0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,
                -0.5f, -0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,

                -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,
                 0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,

                -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,
                -0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,

                -0.5f, -0.5f,  0.5f,    0.0f,-1.0f, 0.0f,
                 0.5f, -0.5f,  0.5f,    0.0f,-1.0f, 0.0f,
                 0.5f, -0.5f, -0.5f,    0.0f,-1.0f, 0.0f,

                -0.5f, -0.5f,  0.5f,    0.0f,-1.0f, 0.0f,
                 0.5f, -0.5f, -0.5f,    0.0f,-1.0f, 0.0f,
                -0.5f, -0.5f, -0.5f,    0.0f,-1.0f, 0.0f,

                -0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
                 0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
                 0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,

                -0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
                 0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
                -0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,

                -0.5f,  0.5f, -0.5f,    0.0f, 0.0f,-1.0f,
                 0.5f,  0.5f, -0.5f,    0.0f, 0.0f,-1.0f,
                 0.5f, -0.5f, -0.5f,    0.0f, 0.0f,-1.0f,

                -0.5f,  0.5f, -0.5f,    0.0f, 0.0f,-1.0f,
                 0.5f, -0.5f, -0.5f,    0.0f, 0.0f,-1.0f,
                -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,-1.0f,
            };

            render::backend::VertexLayoutDesc layout;
            layout.strideBytes = 6 * sizeof(float);
            layout.attributes = { {0, 3, 0}, {1, 3, 3 * sizeof(float)} };

            buildMesh(vertices, layout);
        }

        void initSphereWithNormals(int segments = 24, int rings = 16)
        {
            if (segments < 3) segments = 3;
            if (rings < 2) rings = 2;
            const float radius = 0.5f;
            std::vector<float> vertices;
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

            render::backend::VertexLayoutDesc layout;
            layout.strideBytes = 6 * sizeof(float);
            layout.attributes = { {0, 3, 0}, {1, 3, 3 * sizeof(float)} };

            buildMesh(vertices, layout);
        }

        // 渲染，绑定着色器（用后端编译缓存），提交渲染
        void render(render::CommandBuffer& cmd)
        {
            if (m_MeshHandle == 0 || m_VertexCount <= 0) return;
            if (!m_Material) m_Material = shine::render::Material::GetDefaultPhong();
            if (m_Material) m_Material->bind(cmd);

            cmd.BindVertexArray(m_MeshHandle);
            cmd.DrawTriangles(0, m_VertexCount);
        }

        // 材质接口
        void setMaterial(std::shared_ptr<shine::render::Material> mat) { 
            m_Material = std::move(mat); 
        }
        std::shared_ptr<shine::render::Material> getMaterial() const { return m_Material; }

        [[nodiscard]] u64 meshHandle() const { return m_MeshHandle; }
        [[nodiscard]] int vertexCount() const { return m_VertexCount; }

    private:
        void buildMesh(const std::vector<float>& vertices, const render::backend::VertexLayoutDesc& layout)
        {
            releaseMesh();
            auto* renderer = shine::EngineContext::Get().GetSystem<shine::render::RendererService>();
            if (!renderer) return;
            auto* backend = renderer->GetBackend();
            if (!backend) return;

            render::backend::MeshCreateInfo info;
            info.vertexData = vertices.data();
            info.vertexDataSize = vertices.size() * sizeof(float);
            info.vertexCount = m_VertexCount;
            info.layout = layout;

            m_MeshHandle = backend->CreateMesh(info);
        }

        void releaseMesh()
        {
            if (m_MeshHandle == 0) return;
            if (!shine::EngineContext::IsInitialized()) {
                m_MeshHandle = 0;
                return;
            }
            auto* renderer = shine::EngineContext::Get().GetSystem<shine::render::RendererService>();
            if (renderer) {
                if (auto* backend = renderer->GetBackend()) {
                    backend->ReleaseMesh(m_MeshHandle);
                }
            }
            m_MeshHandle = 0;
        }

        u64 m_MeshHandle { 0 };
        std::shared_ptr<shine::render::Material> m_Material;
        
        int m_VertexCount { 0 };
    };
}

