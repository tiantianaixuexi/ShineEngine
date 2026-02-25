#include "LogUI.h"
#include "imgui/imgui.h"

namespace shine::editor::views {

ShineLogManager *_logManager = nullptr;

void LogUI::onInit() {
    _logManager = &ShineLogManager::Get();
}

void LogUI::onShutDown() {
}

void LogUI::ClearLog() {
}

void LogUI::onRender() {
    if (!ImGui::Begin("Console Log")) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear"))
        ClearLog();
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
    ImGui::SameLine();
    static ImGuiTextFilter filter;
    filter.Draw("Filter", -100.0f);

    ImGui::Separator();

    // Reserve space for footer if needed, though we don't have one right now
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // Tighten spacing
        const auto      &Logs = _logManager->GetLogs();
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(Logs.size()));
        while (clipper.Step()) {
            ImVec4 color;
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                const auto &log = Logs[i];
                if (log.level == ShineLogLevel::Info) {
                    color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                } else if (log.level == ShineLogLevel::Warn) {
                    color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                } else if (log.level == ShineLogLevel::Error) {
                    color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                } else if (log.level == ShineLogLevel::Debug) {
                    color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                } else {
                    color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                }
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(log.message.c_str());
                ImGui::PopStyleColor();
            }
        }

        ImGui::PopStyleVar();

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}
} // namespace shine::editor::views
