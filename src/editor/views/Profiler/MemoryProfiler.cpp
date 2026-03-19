#include "MemoryProfiler.h"
#include "fmt/format.h"
#include "imgui/imgui.h"
#include <array>
#include <vector>

// Include Memory System
// Assuming non-module build for now as per previous context
#ifdef SHINE_USE_MODULE
import shine.memory;
#else
#include "../../../memory/memory.ixx"
#endif

namespace shine::editor::views {

namespace
{
    void RenderMemoryDistributionBar(const char* name, float fraction)
    {
        std::array<char, 128> overlay{};
        const auto result = fmt::format_to_n(overlay.data(), overlay.size() - 1, "{}: {:.1f}%", name, static_cast<double>(fraction) * 100.0);
        const size_t terminatorIndex = result.size < overlay.size() ? result.size : overlay.size() - 1;
        overlay[terminatorIndex] = '\0';
        ImGui::ProgressBar(fraction, ImVec2(0.0f, 0.0f), overlay.data());
    }
}

void MemoryProfiler::onInit() {
    SetName("内存监控");
}

void MemoryProfiler::onShutDown() {
    // Cleanup if needed
}

void MemoryProfiler::onRender() {

    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(name.c_str(), &isOpen)) {
        // Toolbar
        ImGui::Checkbox("Pause", &m_PauseProfiling);
        ImGui::Separator();

        if (!m_PauseProfiling) {
            // Gather data logic...
        }

        // Summary
        size_t totalBytes = 0;
        size_t totalAlloc = 0;
        size_t totalFree  = 0;

        struct TagData {
            const char               *name;
            shine::co::MemoryTagStats stats;
        };
        std::vector<TagData> allStats;

        for (size_t i = 0; i < (size_t)shine::co::MemoryTag::Count; ++i) {
            auto tag   = (shine::co::MemoryTag)i;
            auto stats = shine::co::Memory::GetTagStats(tag);

            // Show all tags
            totalBytes += stats.bytes_current;
            totalAlloc += stats.alloc_count;
            totalFree += stats.free_count;

            allStats.push_back({shine::co::g_memoryTagNames[i], stats});
        }

        // Draw Summary Header
        ImGui::Text("Total Memory: %.2f MB", totalBytes / (1024.0f * 1024.0f));
        ImGui::Text("Total Allocs: %llu", totalAlloc);
        ImGui::Text("Total Frees:  %llu", totalFree);
        ImGui::Separator();

        // Draw Table
        if (ImGui::BeginTable("MemoryTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable)) {
            ImGui::TableSetupColumn("Tag", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Current (MB)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Peak (MB)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Allocs", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Frees", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            for (const auto &data : allStats) {
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

        // Visual Bars
        ImGui::Separator();
        ImGui::Text("Memory Distribution");
        for (const auto &data : allStats) {
            if (totalBytes > 0 && data.stats.bytes_current > 0) {
                float fraction = (float)data.stats.bytes_current / (float)totalBytes;
                if (fraction > 0.001f) {
                    RenderMemoryDistributionBar(data.name, fraction);
                }
            }
        }
    }
    ImGui::End();

    CheckIsOpenChange();
}
} // namespace shine::editor::views
