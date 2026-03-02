#include "StaticMeshComponent.h"
#include "gameplay/object.h"
#include "TransformComponent.h"
#include "render/material.h"

namespace shine::gameplay::component
{
	StaticMeshComponent::StaticMeshComponent()
	{
	}

	StaticMeshComponent::~StaticMeshComponent()
	{
	}

    void StaticMeshComponent::onRender(shine::render::CommandBuffer& cmd)
    {
        if (!m_StaticMesh) return;

        // 1. Get Model Matrix
        shine::math::FMatrix4f modelMatrix = m_Owner->getComponent<TransformComponent>()->getModelMatrix();
        // 2. Prepare Matrix Data (column-major for OpenGL)
        std::vector<float> matData(16);
        const float* src = modelMatrix.data();
        for(int i=0; i<16; ++i) matData[i] = src[i];

        // 3. Bind Material and set uniforms
        auto material = m_StaticMesh->getMaterial();
        if (!material) 
        {
            // Use default if none
            material = shine::render::Material::GetDefaultPhong();
            m_StaticMesh->setMaterial(material);
        }

        if (material)
        {
            material->bind(cmd);
            
            // Set Model Matrix
            int32_t loc = material->getLocationModel();
            if (loc >= 0)
            {
                cmd.SetUniformMatrix4fv(loc, matData, false);
            }
        }
        
        // 4. Draw
        if (m_StaticMesh->meshHandle() > 0 && m_StaticMesh->vertexCount() > 0)
        {
            cmd.BindVertexArray(m_StaticMesh->meshHandle());
            cmd.DrawTriangles(0, m_StaticMesh->vertexCount());
        }
    }

}
