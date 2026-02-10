#include "LogUI.h"
#include "imgui/imgui.h"

namespace shine::editor::views {

    LogUI::LogUI() {}

    void LogUI::Clear() {
        shine::LogSystem::Get().Clear();
    }

    void LogUI::Render() {
        if (!ImGui::Begin("Console Log")) {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Clear")) Clear();
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
        ImGui::SameLine();
        static ImGuiTextFilter filter;
        filter.Draw("Filter", -100.0f);

        ImGui::Separator();
        
        // Reserve space for footer if needed, though we don't have one right now
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        std::mutex& mutex = shine::LogSystem::Get().GetMutex();
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto& logs = shine::LogSystem::Get().GetLogs();

            for (const auto& log : logs) {
                // Filter logic matches category or message
                if (!filter.PassFilter(log.message.c_str()) && !filter.PassFilter(log.category.c_str())) continue;

                ImVec4 color;
                switch (log.level) {
                    case LogLevel::Info:  color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break; 
                    case LogLevel::Warn:  color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break; 
                    case LogLevel::Error: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break; 
                    case LogLevel::Debug: color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); break; 
                }

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::Text("[%s]", log.timestamp.c_str());
                ImGui::SameLine();
                ImGui::Text("[%s]", log.category.c_str());
                ImGui::PopStyleColor();

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(log.message.c_str());
                ImGui::PopStyleColor();
            }
        }

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
}
