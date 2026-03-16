#pragma once
// ============================================================
//  AssetDropQueue — 跨线程安全的外部文件拖放队列接口
//
//  仅声明 EnqueueExternalDrop()，不引入 ImGui / 编辑器重量级头文件，
//  可在平台层安全 include。
//
//  实现位于 AssetsBorwer.cpp。
// ============================================================

#include <filesystem>
#include <vector>

namespace shine::editor::assets_brower
{
    /// 将操作系统拖入的文件路径列表推入导入队列。
    /// 线程安全：可在 WM_DROPFILES 消息线程调用。
    /// AssetsBrower 在下一帧 onRender() 中逐条弹出并触发导入弹窗。
    void EnqueueExternalDrop(std::vector<std::filesystem::path> paths);

} // namespace shine::editor::assets_brower
