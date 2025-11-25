#pragma once

#include "gameplay/object.h"
#include <vector>
#include <memory>

namespace shine::editor::views
{
    /**
     * @brief 场景层级视图 - 显示场景中的对象层级结构
     */
    class SceneHierarchyView
    {
    public:
        SceneHierarchyView();
        ~SceneHierarchyView();

        void Render();
        void SetSelectedObject(shine::gameplay::SObject* obj);
        shine::gameplay::SObject* GetSelectedObject() const { return selectedObject_; }

    private:
        void RenderObjectNode(shine::gameplay::SObject* obj);
        
        shine::gameplay::SObject* selectedObject_ = nullptr;
        bool isOpen_ = true;
    };
}

