#include "EditorView.h"

#include "gameplay/camera.h"
#include "imgui/imgui.h"
#include "manager/CameraManager.h"
#include "render/demo/EngineDemoScene.h"

namespace shine::editor::EditorView {

static manager::CameraManager  *cameraManager    = nullptr;
static render::RendererService *renderer         = nullptr;

EditView::EditView(shine::EngineContext& context) : m_Context(context) {}

EditView::~EditView() = default;

void EditView::Init() {


    renderer        = m_Context.GetSystem<render::RendererService>();
    cameraManager   = m_Context.GetSystem<manager::CameraManager>();

        // 创建一个与窗口大小类似的视口（这里写死，后面可以在WM_SIZE中更新）
    Viewport        = renderer->createViewport(1280, 720);

    // Initialize Demo Scene
    m_DemoScene = std::make_unique<shine::render::demo::EngineDemoScene>(m_Context);
    m_DemoScene->Init();
}

void EditView::Render() {

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

            // Update Scene
            if (m_DemoScene) {
                m_DemoScene->Tick(ImGui::GetIO().DeltaTime);
            }
            
            // Render View
            renderer->renderView(Viewport, cam);

            ImGui::Image(renderer->getViewportTexture(Viewport), rightSize);
        } else 
        {
            ImGui::InvisibleButton("EditorViewportArea", rightSize);
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
} // namespace shine::editor::EditorView
