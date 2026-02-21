#include "EngineDemoScene.h"
#include "render/renderer_service.h"
#include "gameplay/object.h"
#include "gameplay/component/StaticMeshComponent.h"
#include "gameplay/component/TransformComponent.h"
#include "render/material.h"

namespace shine::render::demo
{
    EngineDemoScene::EngineDemoScene(shine::EngineContext& context)
        : m_Context(context)
    {
    }

    EngineDemoScene::~EngineDemoScene()
    {
        auto* renderer = m_Context.GetSystem<shine::render::RendererService>();
        if (renderer)
        {
            for (const auto& obj : m_Objects)
            {
                renderer->unregisterObject(obj.get());
            }
        }
        m_Objects.clear();
    }

    void EngineDemoScene::Init()
    {
        auto* renderer = m_Context.GetSystem<shine::render::RendererService>();
        if (!renderer) return;

        // 1. Create Cube
        auto cubeObj = std::make_unique<shine::gameplay::SObject>();
        cubeObj->setName("Cube");
        
        auto* transform = cubeObj->addComponent<shine::gameplay::component::TransformComponent>();
        transform->setPosition({ 0.0f, 0.0f, 0.0f });
        transform->setScale({ 1.0f, 1.0f, 1.0f });
        
        auto* meshComp = cubeObj->addComponent<shine::gameplay::component::StaticMeshComponent>();
        auto cubeMesh = std::make_shared<shine::gameplay::StaticMesh>();
        cubeMesh->initCubeWithNormals();
        meshComp->setMesh(cubeMesh);
        
        // Use Fancy Material
        meshComp->getMesh()->setMaterial(shine::render::Material::GetFancyRimToon());

        renderer->registerObject(cubeObj.get());
        m_Objects.push_back(std::move(cubeObj));
    }

    void EngineDemoScene::Tick(float deltaTime)
    {
        m_RotationAngle += deltaTime * 50.0f; // degrees per second
        
        // Find Cube and Rotate
        for(const auto& obj : m_Objects)
        {
            if(obj->getName() == "Cube")
            {
                if(auto* tr = obj->getComponent<shine::gameplay::component::TransformComponent>())
                {
                    shine::math::FRotator3f rot = tr->getRotation();
                    rot.Yaw = m_RotationAngle;
                    rot.Pitch = m_RotationAngle * 0.5f;
                    tr->setRotation(rot);
                }
            }
        }
    }
}
