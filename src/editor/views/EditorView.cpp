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
	Viewport = render::RendererService::get().createViewport(1280, 720);
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
		auto& renderer = render::RendererService::get();
      // 当前面板大小改变时，动态调整渲染视口与相机宽高比，避免图像拉伸变形
      int w = static_cast<int>(rightSize.x); if (w < 1) w = 1;
      int h = static_cast<int>(rightSize.y); if (h < 1) h = 1;
      static int lastW = 0, lastH = 0;
      if (w != lastW || h != lastH) {
          renderer.resizeViewport(Viewport, w, h);
        if (auto cam = shine::manager::CameraManager::get().getMainCamera()) {
          cam->SetPerspective(cam->fov, (float)w / (float)h, cam->nearPlane, cam->farPlane);
        }
        lastW = w; lastH = h;
      }

      if (auto camera = shine::manager::CameraManager::get().getMainCamera()) {
          renderer.renderView(Viewport, camera);
      }
      ImGui::Image(renderer.getViewportTexture(Viewport), rightSize);
    } else {
      ImGui::InvisibleButton("EditorViewportArea", rightSize);
    }
    bool right_viewport_hovered = ImGui::IsItemHovered();
    bool right_viewport_focused = ImGui::IsItemActive();


    // 相机控制逻辑（仅在右侧视口悬停或聚焦时响应）
    if (right_viewport_hovered || right_viewport_focused) {
      ImGuiIO &io = ImGui::GetIO();

      // 获取当前相机
      auto camera = shine::manager::CameraManager::get().getMainCamera();
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

        // 键盘移动（WASDQE）
        float moveDelta = 0.016f; // 可根据帧率调整
        if (ImGui::IsKeyDown(ImGuiKey_W))
          camera->ProcessKeyboard(shine::gameplay::CameraMovement::FORWARD, moveDelta);
        if (ImGui::IsKeyDown(ImGuiKey_S))
          camera->ProcessKeyboard(shine::gameplay::CameraMovement::BACKWARD, moveDelta);
        if (ImGui::IsKeyDown(ImGuiKey_A))
          camera->ProcessKeyboard(shine::gameplay::CameraMovement::LEFT, moveDelta);
        if (ImGui::IsKeyDown(ImGuiKey_D))
          camera->ProcessKeyboard(shine::gameplay::CameraMovement::RIGHT, moveDelta);
        if (ImGui::IsKeyDown(ImGuiKey_Q))
          camera->ProcessKeyboard(shine::gameplay::CameraMovement::DOWN, moveDelta);
        if (ImGui::IsKeyDown(ImGuiKey_E))
          camera->ProcessKeyboard(shine::gameplay::CameraMovement::UP, moveDelta);
      }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            fmt::println("点击了鼠标左键");
        }

    }
  }

  ImGui::Columns(1);

  // Shader编译进度（每帧推进一次）
  {
    auto& sm = shine::render::ShaderManager::get();
    // 确保入队内置shader（只做一次）
    static bool enqueued = false; if (!enqueued) { shine::render::Material::EnqueueBuiltinsForProgress(); enqueued = true; }
    sm.compileNext();
    float p = sm.getProgress();
    ImGui::Text("Shader Compile Progress");
    ImGui::ProgressBar(p, ImVec2(-1, 0));
  }

  ImGui::Button("hsplitter", ImVec2(-1, 2.0f));
  if (ImGui::IsItemActive()) {
    // h -= ImGui::GetIO().MouseDelta.y;
    // h = ImClamp(h, 10.0f, viewportPanelSize.y - 10.0f);
  }

  // 控制按钮
  if (ImGui::Button("重置视图")) {

    // 重置当前相机
    if (auto cam = shine::manager::CameraManager::get().getMainCamera()) {
      cam->SetPosition(0.0f, 0.0f, 5.0f);
      cam->SetRotationFromEuler(-90.0f, 0.0f, 0.0f);
      cam->SetPerspective(45.0f, (float)1280 / (float)720, 0.1f, 100.0f);
    }
  }

  ImGui::SameLine();
  static bool showFancy = false;
  if (ImGui::Button("应用效果")) {
    showFancy = !showFancy;
  }
  if (showFancy) {
    if (ImGui::Begin("材质参数", &showFancy)) {
      // 实时调节Fancy材质参数
      auto pbr = shine::render::Material::GetPBR();
      auto base = pbr->getBaseColor();
      auto amb  = pbr->getAmbient();
      float metallic  = pbr->getMetallic();
      float roughness = pbr->getRoughness();
      float ao        = pbr->getAo();
      // 显示并修改参数
      ImGui::ColorEdit3("BaseColor", base.data());
      ImGui::ColorEdit3("Ambient", amb.data());
      ImGui::DragFloat("Metallic",  &metallic,  0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Roughness", &roughness, 0.01f, 0.04f, 1.0f);
      ImGui::DragFloat("AO",        &ao,        0.01f, 0.0f, 1.0f);
      // 应用
      pbr->setBaseColor(base[0], base[1], base[2]);
      pbr->setAmbient(amb[0], amb[1], amb[2]);
      pbr->setMetallic(metallic);
      pbr->setRoughness(roughness);
      pbr->setAo(ao);
    }
    ImGui::End();
  }
  ImGui::End();
}
} // namespace shine::editor::EditorView


