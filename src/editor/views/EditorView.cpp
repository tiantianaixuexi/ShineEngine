#include "EditorView.h"

#include "gameplay/camera.h"
#include "imgui/imgui.h"
#include "manager/CameraManager.h"

namespace shine::editor::EditorView {

static manager::CameraManager  *cameraManager    = nullptr;
static render::RendererService *renderer         = nullptr;

void EditView::Init() {


    renderer        = m_Context.GetSystem<render::RendererService>();
    cameraManager   = m_Context.GetSystem<manager::CameraManager>();

        // 创建一个与窗口大小类似的视口（这里写死，后面可以在WM_SIZE中更新）
    Viewport        = renderer->createViewport(1280, 720);
}

void EditView::Render() const {

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

            renderer->renderView(Viewport, cam);

            ImGui::Image(renderer->getViewportTexture(Viewport), rightSize);
        } else 
        {
            ImGui::InvisibleButton("EditorViewportArea", rightSize);
        }

        const bool is_hovered = ImGui::IsItemHovered();
        const bool is_focused = ImGui::IsItemActive();

        // 相机控制逻辑视口悬停和聚焦时响应
        if (is_hovered && is_focused) {

            const ImGuiIO &io = ImGui::GetIO();

            // 鼠标右键拖拽旋转，使用MouseDelta精确计算
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) 
            {
                // 检查是否有鼠标移动（拖拽）
                if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) {
                    const float dx = io.MouseDelta.x;
                    const float dy = io.MouseDelta.y;
                    cam->ProcessMouseMovement(-dx, -dy, true);
                }
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
