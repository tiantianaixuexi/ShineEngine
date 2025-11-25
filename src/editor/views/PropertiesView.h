#pragma once

#include "gameplay/object.h"

namespace shine::editor::views
{
    /**
     * @brief 属性面板 - 显示和编辑选中对象的属性
     */
    class PropertiesView
    {
    public:
        PropertiesView();
        ~PropertiesView();

        void Render();
        void SetSelectedObject(shine::gameplay::SObject* obj);

    private:
        void RenderObjectProperties(shine::gameplay::SObject* obj);
        void RenderComponentProperties(shine::gameplay::SObject* obj);
        
        shine::gameplay::SObject* selectedObject_ = nullptr;
        bool isOpen_ = true;
    };
}

