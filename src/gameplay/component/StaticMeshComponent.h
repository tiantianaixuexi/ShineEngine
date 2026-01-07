#pragma once


#include <memory>


#include "gameplay/component/component.h"
#include "gameplay/mesh/StaticMesh.h"


namespace shine::render::command
{
	class ICommandList;
}

namespace shine::gameplay::component
{
    class StaticMeshComponent : public UComponent
    {
    public:
        StaticMeshComponent();
        ~StaticMeshComponent() override;

        void setMesh(std::shared_ptr<StaticMesh> mesh) { m_StaticMesh = std::move(mesh); }
        [[nodiscard]] std::shared_ptr<StaticMesh> getMesh() const { return m_StaticMesh; }



        void onRender(shine::render::CommandBuffer& cmd) override;

    private:

        std::shared_ptr<StaticMesh> m_StaticMesh;
    };
}

