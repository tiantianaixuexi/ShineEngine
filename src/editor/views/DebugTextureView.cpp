#include "DebugTextureView.h"

#include "imgui/imgui.h"
#include "render/debug/pass_texture_manager.h"
#include "render/renderer_service.h"
#include "EngineCore/engine_context.h"
#include <algorithm>
#include <string>
#include <cstdint>

namespace shine::editor::views
{
    void DebugTextureView::onInit()
    {
        SetName("Debug贴图");
    }

    void DebugTextureView::onShutDown()
    {
    }

    void DebugTextureView::onRender()
    {
        ImGui::Begin("Debug贴图");

        auto& registry = shine::render::PassTextureManager::get();
        auto* renderer = shine::EngineContext::Get().GetSystem<shine::render::RendererService>();
        const auto pipeline = renderer ? renderer->GetPipeline() : nullptr;
        registry.RefreshFromPipeline(pipeline ? pipeline.get() : nullptr);

        auto names = registry.GetNames();
        if (names.empty())
        {
            ImGui::Text("没有可用贴图");
            ImGui::End();
            return;
        }

        std::sort(names.begin(), names.end());

        ImGui::SliderFloat("缩略图大小", &m_TileSize, 80.0f, 320.0f, "%.0f");
        ImGui::SliderInt("列数", &m_Columns, 1, 6);
        ImGui::Separator();

        if (ImGui::BeginTable("DebugTextureGrid", m_Columns, ImGuiTableFlags_SizingFixedFit))
        {
            for (const auto& name : names)
            {
                const auto entry = shine::render::PassTextureManager::get().GetTextureEntry(name);
                if (entry.id == 0) continue;
                const float w = entry.width > 0 ? static_cast<float>(entry.width) : 256.0f;
                const float h = entry.height > 0 ? static_cast<float>(entry.height) : 256.0f;
                const float aspect = w / h;
                ImVec2 imageSize = { m_TileSize, m_TileSize };
                if (aspect > 1.0f) {
                    imageSize.y = m_TileSize / aspect;
                } else {
                    imageSize.x = m_TileSize * aspect;
                }

                ImGui::TableNextColumn();
                ImGui::Text("%s", name.c_str());
                ImVec2 cursor = ImGui::GetCursorScreenPos();
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 cellMax = ImVec2(cursor.x + m_TileSize, cursor.y + m_TileSize);
                drawList->AddRectFilled(cursor, cellMax, IM_COL32(30, 30, 30, 255));
                ImVec2 offset = { (m_TileSize - imageSize.x) * 0.5f, (m_TileSize - imageSize.y) * 0.5f };
                ImGui::SetCursorScreenPos(ImVec2(cursor.x + offset.x, cursor.y + offset.y));
                ImVec4 tint = ImVec4(1, 1, 1, 1);
                if (m_ChannelMode == 1) tint = ImVec4(1, 0, 0, 1);
                if (m_ChannelMode == 2) tint = ImVec4(0, 1, 0, 1);
                if (m_ChannelMode == 3) tint = ImVec4(0, 0, 1, 1);
                if (m_ChannelMode == 4)
                {
                    const ImU32 dark = IM_COL32(40, 40, 40, 255);
                    const ImU32 light = IM_COL32(70, 70, 70, 255);
                    const float step = 12.0f;
                    for (float y = 0; y < imageSize.y; y += step)
                    {
                        for (float x = 0; x < imageSize.x; x += step)
                        {
                            bool odd = static_cast<int>((x + y) / step) % 2 == 1;
                            ImVec2 p0 = ImVec2(cursor.x + offset.x + x, cursor.y + offset.y + y);
                            ImVec2 p1 = ImVec2(p0.x + step, p0.y + step);
                            drawList->AddRectFilled(p0, p1, odd ? light : dark);
                        }
                    }
                }
                ImGui::Image(reinterpret_cast<void*>(static_cast<uintptr_t>(entry.id)), imageSize, ImVec2(0, 1), ImVec2(1, 0), tint, ImVec4(0, 0, 0, 0));
                if (ImGui::IsItemClicked()) {
                    m_Selected = name;
                    m_ShowDetail = true;
                }
                ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y + m_TileSize));
                ImGui::Dummy(ImVec2(m_TileSize, 2.0f));
            }
            ImGui::EndTable();
        }

        if (m_ShowDetail && !m_Selected.empty())
        {
            std::string title = "贴图查看 - " + m_Selected;
            ImGui::Begin(title.c_str(), &m_ShowDetail);
            const auto entry = shine::render::PassTextureManager::get().GetTextureEntry(m_Selected);
            if (entry.id == 0) {
                ImGui::Text("贴图无效");
            } else {
                const float w = entry.width > 0 ? static_cast<float>(entry.width) : 256.0f;
                const float h = entry.height > 0 ? static_cast<float>(entry.height) : 256.0f;
                ImGui::SliderFloat("缩放", &m_DetailZoom, 0.25f, 8.0f, "%.2f");
                const char* modes[] = { "RGBA", "R", "G", "B", "A" };
                ImGui::Combo("通道", &m_ChannelMode, modes, 5);
                ImVec2 imageSize = ImVec2(w * m_DetailZoom, h * m_DetailZoom);
                ImVec4 tint = ImVec4(1, 1, 1, 1);
                if (m_ChannelMode == 1) tint = ImVec4(1, 0, 0, 1);
                if (m_ChannelMode == 2) tint = ImVec4(0, 1, 0, 1);
                if (m_ChannelMode == 3) tint = ImVec4(0, 0, 1, 1);
                if (m_ChannelMode == 4)
                {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    const ImU32 dark = IM_COL32(40, 40, 40, 255);
                    const ImU32 light = IM_COL32(70, 70, 70, 255);
                    const float step = 14.0f;
                    for (float y = 0; y < imageSize.y; y += step)
                    {
                        for (float x = 0; x < imageSize.x; x += step)
                        {
                            bool odd = static_cast<int>((x + y) / step) % 2 == 1;
                            ImVec2 p0 = ImVec2(pos.x + x, pos.y + y);
                            ImVec2 p1 = ImVec2(p0.x + step, p0.y + step);
                            drawList->AddRectFilled(p0, p1, odd ? light : dark);
                        }
                    }
                }
                ImGui::Image(reinterpret_cast<void*>(static_cast<uintptr_t>(entry.id)), imageSize, ImVec2(0, 1), ImVec2(1, 0), tint, ImVec4(0, 0, 0, 0));
            }
            ImGui::End();
        }

        ImGui::End();
    }
}
