#include "EngineDemoScene.h"
#include "EngineCore/reflection/Reflection.h"
#include "render/renderer_service.h"
#include "gameplay/actor.h"
#include "gameplay/object.h"
#include "gameplay/world/world_service.h"
#include "gameplay/component/ScriptComponent.h"
#include "gameplay/component/StaticMeshComponent.h"
#include "gameplay/component/TransformComponent.h"
#include "render/pipeline/render_passes.h"
#include "string/shine_string.h"

#include <algorithm>

namespace shine::render::demo
{
    class DemoReflectActor final : public shine::gameplay::StaticMeshActor
    {
    public:
        int hp = 120;
        float speedScale = 1.0f;
        shine::SString title = shine::SString("ReflectDemoActor");

        int GetHp() const
        {
            return hp;
        }

        void SetHp(int value)
        {
            hp = std::max(0, value);
        }

        float MultiplySpeed(float factor)
        {
            speedScale *= factor;
            return speedScale;
        }

        std::string GetTitleUtf8() const
        {
            return title.to_string();
        }
    };

    using DemoReflectActorReg = DemoReflectActor;
    REFLECTION_STRUCT(DemoReflectActorReg)
    {
        REFLECT_FIELD(hp).ScriptRead().ScriptWrite();
        REFLECT_FIELD(speedScale).ScriptRead().ScriptWrite();
        REFLECT_FIELD(title).ScriptRead().ScriptWrite();
        REFLECT_METHOD(GetHp).ScriptCallable();
        REFLECT_METHOD(SetHp).ScriptCallable();
        REFLECT_METHOD(MultiplySpeed).ScriptCallable();
        REFLECT_METHOD(GetTitleUtf8).ScriptCallable();
    }

    EngineDemoScene::EngineDemoScene(shine::EngineContext& context)
        : m_Context(context)
    {
    }

    EngineDemoScene::~EngineDemoScene()
    {
    }

    void EngineDemoScene::Init()
    {
        auto* renderer = m_Context.GetSystem<shine::render::RendererService>();
        auto* worldService = m_Context.GetSystem<shine::gameplay::world::WorldService>();
        if (worldService)
        {
            worldService->createMapAsset("DemoMap");
            auto* map = worldService->getActiveMap();
            if (map)
            {
               // map->getWorldSettings().gravityZ = -980.0f;
                //map->getWorldSettings().timeDilation = 1.0f;

                auto& subLevel = worldService->ensureStreamingLevel("Gameplay_SubLevel_A");
                subLevel.actorDefinitions = {
                    {"SubLevelCube_A", gameplay::world::ActorArchetype::StaticMeshCube, {-4.0f, 0.3f, 0.2f}, {0.8f, 0.8f, 0.8f}},
                    {"SubLevelSphere_A", gameplay::world::ActorArchetype::StaticMeshSphere, {-3.0f, -0.7f, 1.3f}, {0.6f, 0.6f, 0.6f}}
                };
                worldService->requestLoadLevelAsync("Gameplay_SubLevel_A");
            }
        }

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

        auto sceneObj = std::make_unique<shine::gameplay::StaticMeshActor>();
        sceneObj->setName("Changjing");
        auto* sceneTransform = sceneObj->addComponent<shine::gameplay::component::TransformComponent>();
        sceneTransform->setPosition({ 0.0f, -1.0f, 0.0f });
        sceneTransform->setScale({ 0.4f, 0.4f, 0.4f });
        auto* sceneMeshComp = sceneObj->addComponent<shine::gameplay::component::StaticMeshComponent>();
        if (!sceneMeshComp->loadModelMesh("Content/model/hellowkiti.glb", 0))
        {
            auto fallbackMesh = std::make_shared<shine::gameplay::StaticMesh>();
            fallbackMesh->initCubeWithNormals();
            sceneMeshComp->setMesh(fallbackMesh);
        }
        auto* scriptComp = sceneObj->addComponent<shine::gameplay::component::ScriptComponent>(
            shine::STextView::from_literal("build/script/game.ts")
        );
        scriptComp->setTickGroup(shine::gameplay::ETickGroup::Late);
        if (worldService)
        {
            worldService->addActorToPersistentLevel(std::move(sceneObj));
        }

        auto demoActor = std::make_unique<DemoReflectActor>();
        demoActor->setName("ReflectDemoActor");
        auto* demoTransform = demoActor->addComponent<shine::gameplay::component::TransformComponent>();
        demoTransform->setPosition({ 2.2f, 0.4f, 0.6f });
        demoTransform->setScale({ 0.7f, 0.7f, 0.7f });
        auto* demoMesh = demoActor->addComponent<shine::gameplay::component::StaticMeshComponent>();
        auto meshData = std::make_shared<shine::gameplay::StaticMesh>();
        meshData->initCubeWithNormals();
        demoMesh->setMesh(meshData);
        if (worldService)
        {
            worldService->addActorToPersistentLevel(std::move(demoActor));
        }
    }
}
