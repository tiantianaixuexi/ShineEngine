#include "SceneHierarchyView.h"
#include "fmt/format.h"
#include "imgui/imgui.h"

namespace shine::editor::views {

void SceneHierarchyView::onInit() {
    SetName("场景层级");
    for (int i = 0; i < 100; i++) {
        testData.emplace_back(fmt::format("测试对象 {}", i));
    }
}

void SceneHierarchyView::onShutDown() {
    selectedObject_ = nullptr;
}

void SceneHierarchyView::onRender() {
    if (ImGui::Begin(name.c_str(), &isOpen)) {

        // 显示场景中的对象
        // 这里暂时显示一个示例对象，后续可以连接到实际的场景管理器
        if (ImGui::TreeNodeEx("Root", ImGuiTreeNodeFlags_DefaultOpen)) {
            // 示例：显示一个测试对象
            const bool isSelected = (selectedObject_ != nullptr);

            for (auto &c : testData) {
                if (ImGui::TreeNodeEx(c.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet)) {
                    ImGui::TreePop();
                }
            }

            // if (ImGui::Selectable("测试对象", isSelected))
            //{
            //     fmt::println("点击了");
            //     if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            //     {
            //         fmt::println("点击了鼠标左键");
            //     }else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            //     {
            //         fmt::println("点击了鼠标右键");
            //     }
            //     // 这里可以设置选中的对象
            //     // SetSelectedObject(&g_TestActor);
            //
            // }
            //
            ImGui::TreePop();
        }
    }

    ImGui::End();
    
}

void SceneHierarchyView::SetSelectedObject(shine::gameplay::SObject *obj) {
    selectedObject_ = obj;
}
} // namespace shine::editor::views
