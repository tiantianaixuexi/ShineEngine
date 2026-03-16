#pragma once
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace shine::widget
{

// ---------------------------------------------------------------------------
// IconTreeNode — 可折叠树节点：[箭头] [图标 24×24] [标签]
//
// openId   : 展开时显示的图标 ImTextureID
// closeId  : 折叠时显示的图标 ImTextureID（可与 openId 相同）
// flags    : 标准 ImGuiTreeNodeFlags，常用 ImGuiTreeNodeFlags_OpenOnArrow 等
// 返回值   : 节点当前是否展开（展开时调用方负责渲染子项并调用 ImGui::TreePop()）
// ---------------------------------------------------------------------------
inline bool IconTreeNode(const char* label,
                         ImTextureID openId,
                         ImTextureID closeId,
                         ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None)
{
    ImGuiContext& g      = *ImGui::GetCurrentContext();
    ImGuiWindow*  window = g.CurrentWindow;
    if (window->SkipItems) return false;

    const ImGuiID id      = window->GetID(label);
    const ImVec2  pos     = window->DC.CursorPos;
    constexpr float kIconSz = 20.0f;
    const float rowH  = ImMax(g.FontSize + g.Style.FramePadding.y * 2.0f, kIconSz + g.Style.FramePadding.y * 2.0f);
    const float paddX = g.Style.ItemInnerSpacing.x;
    const float avail = ImGui::GetContentRegionAvail().x;

    ImRect bb(pos, ImVec2(pos.x + avail, pos.y + rowH));

    // ── 计算折叠状态 ──────────────────────────────────────────────────────
    bool opened = ImGui::TreeNodeUpdateNextOpen(id, flags);

    // ── 点击处理（切换展开状态）──────────────────────────────────────────
    bool hovered, held;
    if (ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick))
        window->DC.StateStorage->SetInt(id, opened ? 0 : 1);

    // ── 背景高亮 ──────────────────────────────────────────────────────────
    if (hovered || held)
        window->DrawList->AddRectFilled(
            bb.Min, bb.Max,
            ImGui::GetColorU32(held ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered));

    // ── 箭头（直接用 DrawList，避免移动光标）──────────────────────────────
    const ImU32  textCol = ImGui::GetColorU32(ImGuiCol_Text);
    const float  arrowSz = g.FontSize;
    ImGui::RenderArrow(
        window->DrawList,
        ImVec2(pos.x + paddX, pos.y + (rowH - arrowSz) * 0.5f),
        textCol,
        opened ? ImGuiDir_Down : ImGuiDir_Right,
        0.8f);

    // ── 图标（直接用 DrawList）──────────────────────────────────────────
    const float iconX  = pos.x + arrowSz + paddX * 2.0f;
    const float iconY  = pos.y + (rowH - kIconSz) * 0.5f;
    const ImTextureID iconId = opened ? openId : closeId;
    if (iconId)
        window->DrawList->AddImage(
            iconId,
            ImVec2(iconX, iconY),
            ImVec2(iconX + kIconSz, iconY + kIconSz));

    // ── 文字（直接用 DrawList）──────────────────────────────────────────
    const float textX = iconX + kIconSz + paddX;
    const float textY = pos.y + (rowH - g.FontSize) * 0.5f;
    ImGui::RenderText(ImVec2(textX, textY), label);

    // ── 注册 Item（推进光标 + 碰撞测试，使 IsItemClicked 等 API 可用）────
    ImGui::ItemSize(bb, g.Style.FramePadding.y);
    ImGui::ItemAdd(bb, id);

    if (opened)
        ImGui::TreePush(label);
    return opened;
}

// ---------------------------------------------------------------------------
// IconLeafNode — 叶节点（无箭头）：[图标 16×16] [标签]
//
// 返回 true 表示本帧被点击（左键单击）。
// ---------------------------------------------------------------------------
inline bool IconLeafNode(const char* label, ImTextureID iconId)
{
    ImGuiContext& g      = *ImGui::GetCurrentContext();
    ImGuiWindow*  window = g.CurrentWindow;
    if (window->SkipItems) return false;

    const ImGuiID id      = window->GetID(label);
    const ImVec2  pos     = window->DC.CursorPos;
    constexpr float kIconSz = 16.0f;
    const float rowH  = g.FontSize + g.Style.FramePadding.y * 2.0f;
    const float paddX = g.Style.ItemInnerSpacing.x;
    const float avail = ImGui::GetContentRegionAvail().x;

    ImRect bb(pos, ImVec2(pos.x + avail, pos.y + rowH));

    // 先注册 item（标准顺序：ItemSize → ItemAdd → ButtonBehavior）
    ImGui::ItemSize(bb, g.Style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (hovered || held)
        window->DrawList->AddRectFilled(
            bb.Min, bb.Max,
            ImGui::GetColorU32(held ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered));

    // 图标
    const float iconY = pos.y + (rowH - kIconSz) * 0.5f;
    if (iconId)
        window->DrawList->AddImage(
            iconId,
            ImVec2(pos.x + paddX, iconY),
            ImVec2(pos.x + paddX + kIconSz, iconY + kIconSz));

    // 文字
    ImGui::RenderText(
        ImVec2(pos.x + paddX + kIconSz + paddX, pos.y + (rowH - g.FontSize) * 0.5f),
        label);

    return ImGui::IsItemClicked();
}

} // namespace shine::widget