#pragma once

#include "gameplay/object.h"
#include "BaseView.h"

namespace shine::script
{
    class ScriptSystem;
}

namespace shine::editor::views
{
    /**
     * @brief 属性面板 - 显示和编辑选中对象的属性
     */
    class PropertiesView : public BaseView
    {
    public:

        virtual ~PropertiesView() {};

        void onInit()    override;
        void onRender()  override; 
        void onShutDown() override;

        void SetSelectedObject(shine::gameplay::SObject* obj);
        void SetScriptSystem(shine::script::ScriptSystem* scriptSystem);

    private:
        void RenderObjectProperties(shine::gameplay::SObject* obj);
        void RenderComponentProperties(shine::gameplay::SObject* obj);
        void RenderScriptProperties(shine::gameplay::SObject* obj);
        
        shine::gameplay::SObject* selectedObject_ = nullptr;
        shine::script::ScriptSystem* scriptSystem_ = nullptr;
        bool isOpen_ = true;
    };
}

