#include "PropertiesView.h"
#include "imgui.h"
#include "fmt/format.h"
#include <typeinfo>

namespace shine::editor::views
{
    PropertiesView::PropertiesView()
    {
    }

    PropertiesView::~PropertiesView()
    {
    }

    void PropertiesView::Render()
    {
        ImGui::Begin("属性", &isOpen_);

        if (selectedObject_ == nullptr)
        {
            ImGui::Text("未选择任何对象");
        }
        else
        {
            RenderObjectProperties(selectedObject_);
        }

        ImGui::End();
    }

    void PropertiesView::SetSelectedObject(shine::gameplay::SObject* obj)
    {
        selectedObject_ = obj;
    }

    void PropertiesView::RenderObjectProperties(shine::gameplay::SObject* obj)
    {
        if (obj == nullptr)
            return;

        // 显示对象名称
        char nameBuffer[256];
        strncpy_s(nameBuffer, obj->getName().c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        
        if (ImGui::InputText("名称", nameBuffer, sizeof(nameBuffer)))
        {
            obj->setName(nameBuffer);
        }

        // 显示激活状态
        bool active = obj->isActive();
        if (ImGui::Checkbox("激活", &active))
        {
            obj->setActive(active);
        }

        // 显示可见性
        bool visible = obj->isVisible();
        if (ImGui::Checkbox("可见", &visible))
        {
            obj->setVisible(visible);
        }

        ImGui::Separator();

        // 显示组件属性
        RenderComponentProperties(obj);
    }

    void PropertiesView::RenderComponentProperties(shine::gameplay::SObject* obj)
    {
        if (ImGui::CollapsingHeader("组件", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // 显示对象的组件列表
            const auto& components = obj->getComponents();
            if (components.empty())
            {
                ImGui::Text("无组件");
            }
            else
            {
                for (size_t i = 0; i < components.size(); ++i)
                {
                    ImGui::Text("组件 %zu: %s", i, typeid(*components[i]).name());
                }
            }
        }
    }
}

