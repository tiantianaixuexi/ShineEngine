#include "EngineDemoScene.h"
#include "render/renderer_service.h"
#include "gameplay/object.h"
#include "gameplay/component/StaticMeshComponent.h"
#include "gameplay/component/TransformComponent.h"
#include "gameplay/component/tickableComponent.h"
#include "render/material.h"
#include "render/pipeline/render_passes.h"

namespace shine::render::demo
{
    class DemoRotationTickComponent : public gameplay::component::TickableComponent
    {
    public:
        void setAngularVelocity(float degreesPerSecond, float pitchScale)
        {
            m_DegreesPerSecond = degreesPerSecond;
            m_PitchScale = pitchScale;
        }

    protected:
        void onTick(float deltaTime) override
        {
            auto* owner = getOwner();
            if (!owner)
            {
                return;
            }

            auto* tr = owner->getComponent<gameplay::component::TransformComponent>();
            if (!tr)
            {
                return;
            }

            m_RotationAngle += deltaTime * m_DegreesPerSecond;
            auto rot = tr->getRotation();
            rot.Yaw = m_RotationAngle;
            rot.Pitch = m_RotationAngle * m_PitchScale;
            tr->setRotation(rot);
        }

    private:
        float m_DegreesPerSecond = 50.0f;
        float m_PitchScale = 0.35f;
        float m_RotationAngle = 0.0f;
    };

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

        // Setup Pipeline Passes (Refactored to Pass-based)
        if (auto pipeline = renderer->GetPipeline())
        {
            pipeline->ClearPasses();
            auto opaquePass = std::make_unique<shine::render::OpaquePass>();
            auto* pOpaque = opaquePass.get();
            
            auto bloomPass = std::make_unique<shine::render::BloomPass>();
            bloomPass->SetOpaquePass(pOpaque);
            bloomPass->m_Threshold = 0.75f;
            bloomPass->m_BlurRadius = 2;
            bloomPass->m_Intensity = 0.95f;
            bloomPass->m_Exposure = 0.85f;
            bloomPass->m_BloomWeights = { 1.0f, 0.8f, 0.6f, 0.4f, 0.3f };
            bloomPass->m_BloomRadius = 0.2f;
            bloomPass->m_SoftKnee = 0.5f;
            bloomPass->m_ActiveLevels = 4;
            bloomPass->m_EnableFXAA = false;

            pipeline->AddPass(std::move(opaquePass));
            pipeline->AddPass(std::move(bloomPass));
        }

        auto spawnMesh = [&](const std::string& name,
                             const shine::math::FVector3f& pos,
                             const shine::math::FVector3f& scale,
                             bool sphere,
                             const shine::math::FVector3f& base,
                             const shine::math::FVector3f& emissiveColor,
                             float emissiveIntensity)
        {
            auto obj = std::make_unique<shine::gameplay::SObject>();
            obj->setName(name);
            auto* transform = obj->addComponent<shine::gameplay::component::TransformComponent>();
            transform->setPosition(pos);
            transform->setScale(scale);
            auto* meshComp = obj->addComponent<shine::gameplay::component::StaticMeshComponent>();
            auto mesh = std::make_shared<shine::gameplay::StaticMesh>();
            if (sphere) mesh->initSphereWithNormals(32, 20);
            else mesh->initCubeWithNormals();
            meshComp->setMesh(mesh);
            auto mat = shine::render::Material::CreateFancyRimToon();
            mat->setBaseColor(base.X, base.Y, base.Z);
            mat->setEmissive(emissiveColor.X, emissiveColor.Y, emissiveColor.Z, emissiveIntensity);
            meshComp->getMesh()->setMaterial(mat);
            auto* tickComp = obj->addComponent<DemoRotationTickComponent>();
            tickComp->setTickGroup(shine::gameplay::ETickGroup::Late);
            tickComp->setAngularVelocity(50.0f, 0.35f);
            renderer->registerObject(obj.get());
            m_Objects.push_back(std::move(obj));
        };

        spawnMesh("CubeA", { -2.0f, 0.0f, -0.6f }, { 0.9f, 0.9f, 0.9f }, false,
                  { 0.12f, 0.05f, 0.05f }, { 1.0f, 0.15f, 0.1f }, 3.8f);
        spawnMesh("CubeB", { 2.0f, 0.2f, 0.4f }, { 0.8f, 0.8f, 0.8f }, false,
                  { 0.05f, 0.1f, 0.18f }, { 0.1f, 0.4f, 1.0f }, 3.4f);
        spawnMesh("CubeC", { -0.2f, 1.1f, 0.9f }, { 0.7f, 0.7f, 0.7f }, false,
                  { 0.1f, 0.05f, 0.18f }, { 0.6f, 0.2f, 1.0f }, 3.0f);
        spawnMesh("SphereA", { 0.0f, 0.0f, -1.6f }, { 1.0f, 1.0f, 1.0f }, true,
                  { 0.12f, 0.12f, 0.12f }, { 1.0f, 0.8f, 0.2f }, 3.6f);
        spawnMesh("SphereB", { 0.6f, -1.2f, 1.6f }, { 0.7f, 0.7f, 0.7f }, true,
                  { 0.05f, 0.12f, 0.07f }, { 0.2f, 1.0f, 0.35f }, 3.2f);
        spawnMesh("SphereC", { -1.0f, -0.9f, 0.9f }, { 0.6f, 0.6f, 0.6f }, true,
                  { 0.12f, 0.12f, 0.02f }, { 1.0f, 0.95f, 0.2f }, 2.8f);
    }
}
