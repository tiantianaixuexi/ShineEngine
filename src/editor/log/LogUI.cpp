#include "LogUI.h"
#include "imgui/imgui.h"

namespace shine::editor::views {


    ShineLogManager* _logManager = nullptr;
    LogUI::LogUI() {}

    void LogUI::Init() {
        _logManager = &ShineLogManager::Get();
    }

    void LogUI::Clear() {
        
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
        

        const auto& Groups = _logManager->GetLogGroups();

        for (const auto& group : Groups) {
            // Filter logic matches category or message
            if (!filter.PassFilter(group.first.c_str())) continue;

            for(const auto& log : group.second->m_Logs) {
                if (!filter.PassFilter(log.message.c_str())) continue;

                ImVec4 color;
                // ImVec4 color;
                switch (log.level) {
                    case ShineLogLevel::Info:  color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break; 
                    case ShineLogLevel::Warn:  color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break; 
                    case ShineLogLevel::Error: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break; 
                    case ShineLogLevel::Debug: color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); break; 
                }

                // ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                // ImGui::Text("[%s]", log.timestamp.c_str());
                // ImGui::SameLine();
                // ImGui::Text("[%s]", log.category.c_str());
                // ImGui::PopStyleColor();

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
