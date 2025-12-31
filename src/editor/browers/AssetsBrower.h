#pragma once

#include<string>


#include "imgui/imgui.h"
#include "AssetsItem.h"

namespace shine::editor::assets_brower 
{

    class AssetsBrower 
    {
        public:

            
            std::string         title = "Assets border";
            bool                isOpen = true;
            bool                RequestSort = false;        // Deferred sort request



            ImVector<assets_item::AssetsItem> items;
            
            void Start();
            void Render();
            bool SetShow() noexcept;

  

        private:
            ImGuiID NextItemId = 0;

            void AddItem();

 
    };

} // namespace shine::editor::assets_brower
