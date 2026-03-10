#include "EditorView.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "EngineCore/engine_context.h"
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

void EditView::SetWorldHierarchyService(shine::gameplay::world::IWorldActorHierarchyService* worldHierarchyService)
{
    worldHierarchyService_ = worldHierarchyService;
}

void EditView::SetSelectedObject(shine::gameplay::SObject* obj)
{
    if (worldHierarchyService_) worldHierarchyService_->setSelectedObject(obj);
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

            // OpenGL 纹理原点在左下角，ImGui 在左上角，需要翻转 Y 轴
            ImGui::Image(renderer->getViewportTexture(Viewport), rightSize, ImVec2(0, 1), ImVec2(1, 0));
        } else 
        {
            ImGui::InvisibleButton("EditorViewportArea", rightSize);
        }

        const ImVec2 viewportMin = ImGui::GetItemRectMin();
        const ImVec2 viewportSize = ImGui::GetItemRectSize();
        const bool is_hovered = ImGui::IsItemHovered();

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

        if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !is_operating)
        {
            if (worldHierarchyService_)
            {
                auto* pickedObject = PickObjectInViewport(viewportMin, viewportSize, ImGui::GetMousePos(), cam);
                if (ImGui::GetIO().KeyCtrl)
                {
                    if (pickedObject)
                    {
                        worldHierarchyService_->toggleSelectedObject(pickedObject);
                    }
                }
                else
                {
                    worldHierarchyService_->setSelectedObject(pickedObject);
                }
            }
        }

        DrawSelectedObjectOutline(viewportMin, viewportSize, cam);
    }

    ImGui::End();
}

    shine::gameplay::SObject* EditView::PickObjectInViewport(
        const ImVec2& viewportMin,
        const ImVec2& viewportSize,
        const ImVec2& mousePos,
        gameplay::Camera* cam
    ) const
    {
        if (!worldHierarchyService_ || !cam || viewportSize.x <= 1.0f || viewportSize.y <= 1.0f)
        {
            return nullptr;
        }

        const auto projection = cam->GetProjectionMatrixM();
        const auto view = cam->GetViewMatrixM();
        const auto vp = projection * view;
        
        const auto objects = worldHierarchyService_->getAllActorsSnapshot();
        float nearestZ = std::numeric_limits<float>::max();
        float nearestDistanceSq = std::numeric_limits<float>::max();
        shine::gameplay::SObject* picked = nullptr;
        
        // 增加点击判定范围，改善操作体验
        constexpr float pickRadiusPx = 32.0f;
        const float maxDistanceSq = pickRadiusPx * pickRadiusPx;

        for (auto* obj : objects)
        {
            if (!obj) continue;
            
            auto* transform = obj->getComponent<shine::gameplay::component::TransformComponent>();
            if (!transform) continue;

            const auto& position = transform->getPosition();
            const math::FVector3d worldPos(position.X, position.Y, position.Z);
            
            // 手动进行透视变换以检查 w 分量，确保物体在相机前方
            const auto& m = vp.m_data;
            double w = m[3] * worldPos.X + m[7] * worldPos.Y + m[11] * worldPos.Z + m[15];
            
            if (w <= 0.0001) continue; // 剔除相机背后的物体

            const auto ndc = vp.transformPoint(worldPos);
            if (ndc.Z < -1.0 || ndc.Z > 1.0) continue;

            // NDC -> Screen
            const float screenX = viewportMin.x + static_cast<float>((ndc.X * 0.5 + 0.5) * viewportSize.x);
            const float screenY = viewportMin.y + static_cast<float>((1.0 - (ndc.Y * 0.5 + 0.5)) * viewportSize.y);
            
            const float dx = mousePos.x - screenX;
            const float dy = mousePos.y - screenY;
            const float distanceSq = dx * dx + dy * dy;
            
            if (distanceSq <= maxDistanceSq)
            {
                // 优先选择离相机更近的物体 (NDC Z 越小越近)
                if (ndc.Z < nearestZ - 0.001f || (std::abs(ndc.Z - nearestZ) < 0.001f && distanceSq < nearestDistanceSq))
                {
                    nearestZ = static_cast<float>(ndc.Z);
                    nearestDistanceSq = distanceSq;
                    picked = obj;
                }
            }
        }

        return picked;
    }

void EditView::DrawSelectedObjectOutline(const ImVec2& viewportMin, const ImVec2& viewportSize, gameplay::Camera* cam) const
{
    if (!worldHierarchyService_ || !cam || viewportSize.x <= 1.0f || viewportSize.y <= 1.0f)
    {
        return;
    }

    const auto selectedObjects = worldHierarchyService_->getSelectedObjectsSnapshot();
    const auto vp = cam->GetViewProjectionMatrixM();
    auto* drawList = ImGui::GetWindowDrawList();
    const ImU32 outlineColor = IM_COL32(255, 208, 48, 255);
    for (auto* selectedObject : selectedObjects)
    {
        if (!selectedObject)
        {
            continue;
        }

        auto* transform = selectedObject->getComponent<shine::gameplay::component::TransformComponent>();
        if (!transform)
        {
            continue;
        }

        const auto& position = transform->getPosition();
        const auto& scale = transform->getScale();

        const math::FVector3d worldPos(position.X, position.Y, position.Z);
        const auto ndc = vp.transformPoint(worldPos);
        if (ndc.Z < -1.0 || ndc.Z > 1.0)
        {
            continue;
        }

        const float centerX = viewportMin.x + static_cast<float>((ndc.X * 0.5 + 0.5) * viewportSize.x);
        const float centerY = viewportMin.y + static_cast<float>((1.0 - (ndc.Y * 0.5 + 0.5)) * viewportSize.y);

        const float worldRadius = std::max({ std::abs(scale.X), std::abs(scale.Y), std::abs(scale.Z), 0.5f });
        const math::FVector3d radiusPos = worldPos + cam->right * static_cast<double>(worldRadius);
        const auto radiusNdc = vp.transformPoint(radiusPos);
        const float radiusX = viewportMin.x + static_cast<float>((radiusNdc.X * 0.5 + 0.5) * viewportSize.x);
        float radiusPx = std::abs(radiusX - centerX);
        if (radiusPx < 14.0f)
        {
            radiusPx = 14.0f;
        }

        drawList->AddCircle(ImVec2(centerX, centerY), radiusPx, outlineColor, 40, 2.4f);
    }
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
        auto* actorPtr = actor.get();
        worldPlacementService_->addActorToPersistentLevel(std::move(actor));
        if (worldHierarchyService_) worldHierarchyService_->setSelectedObject(actorPtr);
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
    auto* actorPtr = actor.get();
    worldPlacementService_->addActorToPersistentLevel(std::move(actor));
    if (worldHierarchyService_) worldHierarchyService_->setSelectedObject(actorPtr);
}
} // namespace shine::editor::EditorView
