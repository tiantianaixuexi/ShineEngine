#include "PropertiesView.h"
#include "imgui/imgui.h"
#include "fmt/format.h"
#include "EngineCore/reflection/Reflection.h"
#include "../util/InspectorBuilder.h"
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

        // Basic Info (Name/Active/Visible)
        // Ideally, SObject itself should be reflected so we can just call DrawInspector(obj).
        // For now, keep this manual part or assume SObject fields are not reflected yet.
        
        char nameBuffer[256];
        strncpy_s(nameBuffer, obj->getName().c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        if (ImGui::InputText("名称", nameBuffer, sizeof(nameBuffer))) {
            obj->setName(nameBuffer);
        }

        bool active = obj->isActive();
        if (ImGui::Checkbox("激活", &active)) {
            obj->setActive(active);
        }

        bool visible = obj->isVisible();
        if (ImGui::Checkbox("可见", &visible)) {
            obj->setVisible(visible);
        }

        ImGui::Separator();

        RenderComponentProperties(obj);
    }

    void PropertiesView::RenderComponentProperties(shine::gameplay::SObject* obj)
    {
        if (ImGui::CollapsingHeader("组件", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto& components = obj->getComponents();
            if (components.empty())
            {
                ImGui::Text("无组件");
            }
            else
            {
                for (size_t i = 0; i < components.size(); ++i)
                {
                    auto* comp = components[i].get();
                    if (!comp) continue;

                    // Use RTTI or Reflection to get the component name
                    // Assuming we have TypeId for components via Reflection
                    // If Component is Reflected, we can find its TypeInfo.
                    
                    // Fallback to RTTI name if no TypeInfo found (though InspectorBuilder requires TypeInfo)
                    // Let's try to find TypeInfo using a helper (requires Component to have GetTypeId or similar)
                    // For now, use typeid name for header, and assume we can't draw fields unless reflected.
                    
                    std::string compName = typeid(*comp).name(); 
                    // Clean up "class " prefix if present (MSVC)
                    if (compName.starts_with("class ")) compName = compName.substr(6);
                    if (compName.starts_with("struct ")) compName = compName.substr(7);

                    if (ImGui::TreeNode((void*)comp, "%s", compName.c_str())) {
                        
                        // Try to find TypeInfo via TypeRegistry
                        // This relies on component classes having registered reflection with matching names or IDs
                        // Since we don't have the compile-time type T here, we need runtime lookup.
                        // Assuming we can get TypeId from instance or name.
                        // If components don't have virtual GetTypeId(), we are stuck unless we use RTTI hash map.
                        
                        // Workaround: We can't easily get TypeInfo from base pointer without virtual GetTypeId().
                        // Let's assume for this task that we just print a placeholder or try to use a hypothetical lookup.
                        // ImGui::Text("Inspector logic here...");
                        
                        // FUTURE: util::InspectorBuilder::DrawInspector(comp, typeInfo);
                        
                        ImGui::TreePop();
                    }
                }
            }
        }
    }
}

