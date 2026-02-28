#pragma once

#include<string>


#include "imgui/imgui.h"
#include "AssetsItem.h"
#include "editor/views/BaseView.h"
namespace shine::editor::assets_brower 
{

    class AssetsBrower : public views::BaseView
    {
        public:

            std::string         title = "资产浏览器";
            bool                RequestSort = false;        // Deferred sort request


            ImVector<assets_item::AssetsItem> items;
            
            void onInit() override;
            void onRender() override;
            void onShutDown() override;

        private:
            ImGuiID NextItemId = 0;

            void AddItem();

 
    };

} // namespace shine::editor::assets_brower
