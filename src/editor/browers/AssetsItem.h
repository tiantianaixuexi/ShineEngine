#pragma once

#include <cstdlib>


#include "imgui.h"


namespace shine::editor::assets_item
{

    class AssetsItem
    {
        public:

            ImGuiID ID;
            int Type;
            static const ImGuiTableSortSpecs* s_current_sort_specs;
            
            AssetsItem(ImGuiID id,int type){ID = id; Type = type;}


                
        static void SortWithSortSpecs(ImGuiTableSortSpecs* sort_specs, AssetsItem* items, int items_count)
        {
            s_current_sort_specs = sort_specs; // Store in variable accessible by the sort function.
            if (items_count > 1)
                qsort(items, (size_t)items_count,  sizeof(items[0]), CompareWithSortSpecs);
            s_current_sort_specs = nullptr;
        }

        // Compare function to be used by qsort()
        static int  CompareWithSortSpecs(const void* lhs, const void* rhs)
        {
            const AssetsItem* a = static_cast<const AssetsItem*>(lhs);
            const AssetsItem* b = static_cast<const AssetsItem*>(rhs);
            for (int n = 0; n < s_current_sort_specs->SpecsCount; n++)
            {
                const ImGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[n];
                unsigned int  delta = 0;
                if (sort_spec->ColumnIndex == 0)
                    delta = a->ID - b->ID;
                else if (sort_spec->ColumnIndex == 1)
                    delta = (a->Type - b->Type);
                if (delta > 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? +1 : -1;
                if (delta < 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? -1 : +1;
            }
            return ((int)a->ID - (int)b->ID);
        }

    };
}
