#include "EditorView.h"

#include "fmt/format.h"
#include "gameplay/actor.h"
#include "gameplay/camera.h"
#include "gameplay/component/StaticMeshComponent.h"
#include "gameplay/component/TransformComponent.h"
#include "gameplay/mesh/StaticMesh.h"
#include "gameplay/world/WorldServiceInterfaces.h"
#include "imgui/imgui.h"
#include "manager/CameraManager.h"
#include "render/demo/EngineDemoScene.h"

namespace shine::editor::views 
{

static manager::CameraManager  *cameraManager    = nullptr;
static render::RendererService *renderer         = nullptr;


EditView::~EditView() = default;

void EditView::SetWorldPlacementService(shine::gameplay::world::IWorldActorPlacementService* worldPlacementService)
{
    worldPlacementService_ = worldPlacementService;
}

void EditView::onInit() {

    SetName("编辑器视图");
    renderer        = EngineContext::Get().GetSystem<render::RendererService>();
    cameraManager   = EngineContext::Get().GetSystem<manager::CameraManager>();

        // 创建一个与窗口大小类似的视口（这里写死，后面可以在WM_SIZE中更新）
    Viewport        = renderer->createViewport(1280, 720);

    // Initialize Demo Scene
    m_DemoScene = std::make_unique<shine::render::demo::EngineDemoScene>(EngineContext::Get());
    m_DemoScene->Init();
}

void EditView::onShutDown() {
    m_DemoScene.reset();
    if (renderer && Viewport != 0) {
        renderer->destroyViewport(Viewport);
        Viewport = 0;
    }
}

void EditView::onRender() {

    // 编辑器视图
    ImGui::Begin("编辑器视图");
    {
        // 计算视口面板大小
        ImVec2    viewportPanelSize = ImGui::GetContentRegionAvail();
        ImVec2 rightSize         = ImVec2(ImGui::GetContentRegionAvail().x, viewportPanelSize.y);
        gameplay::Camera *cam    = cameraManager->getMainCamera();

        // 渲染到该视口的FBO，再显示其纹理
        if (Viewport) {

            // 当前面板大小改变时，动态调整渲染视口与相机宽高比，避免图像拉伸变形
            int w = static_cast<int>(rightSize.x);
            if (w < 1)
                w = 1;
            int h = static_cast<int>(rightSize.y);
            if (h < 1)
                h = 1;

            static int last_w = 0;
            static int lastH  = 0;
            if (w != last_w || h != lastH) {

                renderer->resizeViewport(Viewport, w, h);
                cam->SetPerspective(cam->fov, static_cast<float>(w) / static_cast<float>(h), cam->nearPlane, cam->farPlane);

                last_w = w;
                lastH = h;
            }

            // Render View
            renderer->renderView(Viewport, cam);

            ImGui::Image(renderer->getViewportTexture(Viewport), rightSize);
        } else 
        {
            ImGui::InvisibleButton("EditorViewportArea", rightSize);
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SHINE_PLACE_ITEM"))
            {
                if (payload->Data && payload->DataSize == sizeof(PlacementItemPayload))
                {
                    const auto* item = static_cast<const PlacementItemPayload*>(payload->Data);
                    SpawnPlacementActor(item->type, item->scale, cam);
                }
            }
            ImGui::EndDragDropTarget();
        }

        const bool is_hovered = ImGui::IsItemHovered();
        
        // 当鼠标悬停在视口上，或者正在进行右键操作时（即使鼠标移出了视口范围），都应响应控制
        // 使用 static 变量保持“正在操作”的状态，直到右键松开
        static bool is_operating = false;
        if (is_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            is_operating = true;
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            is_operating = false;
        }

        // 相机控制逻辑
        if (is_hovered || is_operating) {

            const ImGuiIO &io = ImGui::GetIO();
            float deltaTime = io.DeltaTime;

            // 鼠标右键拖拽旋转
            if (is_operating) 
            {
                // 检查是否有鼠标移动（拖拽）
                if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) {
                    const float dx = io.MouseDelta.x;
                    const float dy = io.MouseDelta.y;
                    cam->ProcessMouseMovement(-dx, -dy, true);
                }

                // WSAD 漫游控制 (仅当按住右键时生效，模仿 UE5/Unity 操作习惯)
                if (ImGui::IsKeyDown(ImGuiKey_W)) cam->ProcessKeyboard(gameplay::CameraMovement::FORWARD, deltaTime);
                if (ImGui::IsKeyDown(ImGuiKey_S)) cam->ProcessKeyboard(gameplay::CameraMovement::BACKWARD, deltaTime);
                if (ImGui::IsKeyDown(ImGuiKey_A)) cam->ProcessKeyboard(gameplay::CameraMovement::LEFT, deltaTime);
                if (ImGui::IsKeyDown(ImGuiKey_D)) cam->ProcessKeyboard(gameplay::CameraMovement::RIGHT, deltaTime);
                if (ImGui::IsKeyDown(ImGuiKey_Q)) cam->ProcessKeyboard(gameplay::CameraMovement::DOWN, deltaTime);
                if (ImGui::IsKeyDown(ImGuiKey_E)) cam->ProcessKeyboard(gameplay::CameraMovement::UP, deltaTime);
            }

            // 鼠标滚轮缩放
            if (io.MouseWheel != 0.0f) {
                cam->ProcessMouseScroll(io.MouseWheel);
            }
        }
    }

    ImGui::End();
}

void EditView::SpawnPlacementActor(EPlacementItemType type, float scale, gameplay::Camera* cam)
{
    if (!worldPlacementService_ || !cam)
    {
        return;
    }

    const auto camPos = cam->GetPosition();
    const auto spawnPos = camPos + cam->front * 3.0;

    if (type == EPlacementItemType::EmptyActor)
    {
        auto actor = std::make_unique<shine::gameplay::EmptyActor>();
        actor->setName(fmt::format("PlacedEmpty_{}", nextPlacedActorId_++));
        auto* transform = actor->addComponent<shine::gameplay::component::TransformComponent>();
        transform->setPosition({
            static_cast<float>(spawnPos.X),
            static_cast<float>(spawnPos.Y),
            static_cast<float>(spawnPos.Z)
        });
        worldPlacementService_->addActorToPersistentLevel(std::move(actor));
        return;
    }

    auto actor = std::make_unique<shine::gameplay::StaticMeshActor>();
    actor->setName(fmt::format("PlacedMesh_{}", nextPlacedActorId_++));
    auto* transform = actor->addComponent<shine::gameplay::component::TransformComponent>();
    transform->setPosition({
        static_cast<float>(spawnPos.X),
        static_cast<float>(spawnPos.Y),
        static_cast<float>(spawnPos.Z)
    });
    transform->setScale({ scale, scale, scale });
    auto* meshComp = actor->addComponent<shine::gameplay::component::StaticMeshComponent>();
    auto mesh = std::make_shared<shine::gameplay::StaticMesh>();
    mesh->initCubeWithNormals();
    meshComp->setMesh(mesh);
    worldPlacementService_->addActorToPersistentLevel(std::move(actor));
}
} // namespace shine::editor::EditorView
