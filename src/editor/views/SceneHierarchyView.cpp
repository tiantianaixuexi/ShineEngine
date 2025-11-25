#include "SceneHierarchyView.h"
#include "imgui.h"
#include "fmt/format.h"

namespace shine::editor::views
{
    SceneHierarchyView::SceneHierarchyView()
    {
    }

    SceneHierarchyView::~SceneHierarchyView()
    {
    }

    void SceneHierarchyView::Render()
    {
        ImGui::Begin("场景层级", &isOpen_);

        // 显示场景中的对象
        // 这里暂时显示一个示例对象，后续可以连接到实际的场景管理器
        if (ImGui::TreeNodeEx("场景根节点", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // 示例：显示一个测试对象
            bool isSelected = (selectedObject_ != nullptr);
            if (ImGui::Selectable("测试对象", isSelected))
            {
                // 这里可以设置选中的对象
                // SetSelectedObject(&g_TestActor);
            }
            
            ImGui::TreePop();
        }

        // 添加对象按钮
        if (ImGui::Button("+ 添加对象"))
        {
            // TODO: 实现添加对象功能
        }

        ImGui::End();
    }

    void SceneHierarchyView::SetSelectedObject(shine::gameplay::SObject* obj)
    {
        selectedObject_ = obj;
    }
}

