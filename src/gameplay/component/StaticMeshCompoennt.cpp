#include "StaticMeshComponent.h"

namespace shine::gameplay::component
{

    void StaticMeshComponent::onRender(shine::render::command::ICommandList& cmd)
    {
        if (!m_StaticMesh) return;
        // 用网格自己设置 Program/Uniforms，并提交渲染命令
        m_StaticMesh->render(cmd);
    }

}


