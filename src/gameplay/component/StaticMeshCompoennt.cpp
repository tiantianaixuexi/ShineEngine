#include "StaticMeshComponent.h"

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
        // 用网格自己设置 Program/Uniforms，并提交渲染命令
        m_StaticMesh->render(cmd);
    }

}


