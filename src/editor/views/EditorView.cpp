#include "EditorView.h"


#include <string>
#include <functional>



#include "imgui.h"
#include "fmt/format.h"
#include "gameplay/camera.h"
#include "manager/CameraManager.h"
#include "render/shader_manager.h"
#include "render/material.h"

namespace shine::editor::EditorView
{

void EditView::Init() {

    // 创建一个与窗口大小类似的视口（这里写死，后面可以在WM_SIZE中更新）
	Viewport = m_Context.GetSystem<render::RendererService>()->createViewport(1280, 720);
}

void EditView::Render() {

  // 编辑器视图
  ImGui::Begin("编辑器视图");
  {
    // 计算视口面板大小
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    static float h = 30.0f;

    // 创建水平分割面板
    ImGui::Columns(2, "编辑器分割", true);

    // 左侧视口
    ImGui::Text("原始视图");
    // ImVec2 leftSize = ImVec2(ImGui::GetContentRegionAvail().x,
    // viewportPanelSize.y - h);
    // ImGui::Image((ImTextureID)(static_cast<s64>(RenderBackend->g_FramebufferTexture)),
    // leftSize);

    bool left_viewport_hovered = ImGui::IsItemHovered();
    if (left_viewport_hovered) {
      ImGui::SetTooltip("原始OpenGL渲染内容");
    }

    ImGui::NextColumn();

    // 右侧视口
    ImGui::Text("编辑视图");
    ImVec2 rightSize = ImVec2(ImGui::GetContentRegionAvail().x, viewportPanelSize.y - h);

    // 渲染到该视口的FBO，再显示其纹理
    if ( Viewport) {
		auto* renderer = m_Context.GetSystem<render::RendererService>();
      // 当前面板大小改变时，动态调整渲染视口与相机宽高比，避免图像拉伸变形
      int w = static_cast<int>(rightSize.x); if (w < 1) w = 1;
      int h = static_cast<int>(rightSize.y); if (h < 1) h = 1;
      static int lastW = 0, lastH = 0;
      if (w != lastW || h != lastH) {
          renderer->resizeViewport(Viewport, w, h);
        if (auto cam = m_Context.GetSystem<shine::manager::CameraManager>()->getMainCamera()) {
          cam->SetPerspective(cam->fov, static_cast<float>(w) / static_cast<float>(h), cam->nearPlane, cam->farPlane);
        }
        lastW = w; lastH = h;
      }

      if (auto camera = m_Context.GetSystem<shine::manager::CameraManager>()->getMainCamera()) {
          renderer->renderView(Viewport, camera);
      }
      ImGui::Image(renderer->getViewportTexture(Viewport), rightSize);
    } else {
      ImGui::InvisibleButton("EditorViewportArea", rightSize);
    }
    bool right_viewport_hovered = ImGui::IsItemHovered();
    bool right_viewport_focused = ImGui::IsItemActive();


    // 相机控制逻辑（仅在右侧视口悬停或聚焦时响应）
    if (right_viewport_hovered || right_viewport_focused) {
      ImGuiIO &io = ImGui::GetIO();

      // 获取当前相机
      auto camera = m_Context.GetSystem<shine::manager::CameraManager>()->getMainCamera();
      if (camera) {
        // 鼠标右键拖拽旋转，使用MouseDelta精确计算，避免首次过敏
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            // 检查是否有鼠标移动（拖拽）
            if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) {
                const float dx = io.MouseDelta.x;
                const float dy = io.MouseDelta.y;
                camera->ProcessMouseMovement(-dx, -dy, true);
            }
        }

        // 鼠标滚轮缩放
        if (io.MouseWheel != 0.0f) {
          camera->ProcessMouseScroll(io.MouseWheel);
        }
      }
    }
  }
  ImGui::End();
}
}
