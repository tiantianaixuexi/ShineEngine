#include "MemoryProfiler.h"
#include "imgui/imgui.h"
#include <vector>
#include <string>
#include "fmt/format.h"

// Include Memory System
// Assuming non-module build for now as per previous context
#ifdef SHINE_USE_MODULE
import shine.memory;
#else
#include "../../../memory/memory.ixx"
#endif

namespace shine::editor::views
{
    MemoryProfiler::MemoryProfiler()
    {
    }

    MemoryProfiler::~MemoryProfiler()
    {
    }

    void MemoryProfiler::Render()
    {
        if (!isOpen_) return;

        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Memory Profiler", &isOpen_))
        {
            // Summary
            size_t totalBytes = 0;
            size_t totalAlloc = 0;
            size_t totalFree = 0;

            // Gather data first to calculate totals
            struct TagData {
                const char* name;
                shine::co::MemoryTagStats stats;
            };
            std::vector<TagData> allStats;

            for (size_t i = 0; i < (size_t)shine::co::MemoryTag::Count; ++i) {
                auto tag = (shine::co::MemoryTag)i;
                auto stats = shine::co::Memory::GetTagStats(tag);
                
                // Skip empty tags if desired, or show all
                // if (stats.alloc_count == 0 && stats.bytes_current == 0) continue;

                totalBytes += stats.bytes_current;
                totalAlloc += stats.alloc_count;
                totalFree += stats.free_count;

                allStats.push_back({ shine::co::g_memoryTagNames[i], stats });
            }

            // Draw Summary Header
            ImGui::Text("Total Memory: %.2f MB", totalBytes / (1024.0f * 1024.0f));
            ImGui::Text("Total Allocs: %llu", totalAlloc);
            ImGui::Text("Total Frees:  %llu", totalFree);
            ImGui::Separator();

            // Draw Table
            if (ImGui::BeginTable("MemoryTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable))
            {
                ImGui::TableSetupColumn("Tag", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Current (MB)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Peak (MB)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Allocs", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Frees", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableHeadersRow();

                for (const auto& data : allStats)
                {
                    ImGui::TableNextRow();
                    
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", data.name);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.4f", data.stats.bytes_current / (1024.0f * 1024.0f));

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.4f", data.stats.bytes_peak / (1024.0f * 1024.0f));

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%llu", data.stats.alloc_count);

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%llu", data.stats.free_count);
                }

                ImGui::EndTable();
            }
            
            // Visual Bars (Optional)
            ImGui::Separator();
            ImGui::Text("Memory Distribution");
            for (const auto& data : allStats)
            {
                if (totalBytes > 0 && data.stats.bytes_current > 0) {
                    float fraction = (float)data.stats.bytes_current / (float)totalBytes;
                    ImGui::ProgressBar(fraction, ImVec2(0.0f, 0.0f), fmt::format("{}: {:.1f}%", data.name, fraction * 100.0f).c_str());
                }
            }

        }
        ImGui::End();
    }
}
