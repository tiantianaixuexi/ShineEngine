#pragma once

#include "string/shine_string.h"
#include "string/shine_text_view.h"
#include <optional>
#include <vector>

namespace shine::util
{
    /**
     * @brief 获取可执行文件的目录路径
     * @return 可执行文件所在目录的路径，如果获取失败返回空
     */
    std::optional<SString> get_executable_directory();

    /**
     * @brief 获取脚本文件的完整路径
     * @param script_name 脚本文件名（如 "game.js"）
     * @return 脚本文件的完整路径，如果获取失败返回空
     */
    std::optional<SString> get_script_path(const SString& script_name);

    /**
     * @brief 规范化路径，将路径分隔符统一为当前平台的格式
     * @param path 输入路径
     * @return 规范化后的路径
     */
    SString normalize_path(const SString& path);

    /**
     * @brief 检查路径是否为绝对路径
     * @param path 输入路径
     * @return 为绝对路径返回 true
     */
    bool is_absolute_path(STextView path);

    /**
     * @brief 拼接路径并做规范化
     * @param base 基础路径
     * @param part 子路径
     * @return 规范化后的拼接路径
     */
    SString join_path(STextView base, STextView part);

    /**
     * @brief 规范化资源逻辑路径，统一分隔符并转小写
     * @param path 输入路径
     * @return 规范化后的资源路径
     */
    SString normalize_asset_path(STextView path);

    /**
     * @brief 将相对路径转换为绝对路径
     * @param relative_path 相对路径
     * @param base_path 基准路径（默认为可执行文件目录）
     * @return 绝对路径，如果转换失败返回空
     */
    std::optional<SString> to_absolute_path(const SString& relative_path, STextView base_path = "");

    std::vector<SString> split_path_components(STextView path);
    STextView get_file_extension(STextView path);
    SString normalize_file_extension(STextView path);
    STextView get_directory(STextView path);
    SString trim_file_extension(STextView path);
    SString get_file_name_without_extension(STextView path);
    SString to_standard_path(STextView path);
    SString to_windows_path(STextView path);
    SString to_platform_path(STextView path);
}
