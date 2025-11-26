#pragma once

#include "shine_define.h"
#include <string>
#include <optional>

namespace shine::util
{
    /**
     * @brief 获取可执行文件的目录路径
     * @return 可执行文件所在目录的路径，如果获取失败返回空
     */
    std::optional<std::string> get_executable_directory();

    /**
     * @brief 获取脚本文件的完整路径
     * @param script_name 脚本文件名（如 "game.js"）
     * @return 脚本文件的完整路径，如果获取失败返回空
     */
    std::optional<std::string> get_script_path(const std::string& script_name);

    /**
     * @brief 规范化路径，将路径分隔符统一为当前平台的格式
     * @param path 输入路径
     * @return 规范化后的路径
     */
    std::string normalize_path(const std::string& path);

    /**
     * @brief 将相对路径转换为绝对路径
     * @param relative_path 相对路径
     * @param base_path 基准路径（默认为可执行文件目录）
     * @return 绝对路径，如果转换失败返回空
     */
    std::optional<std::string> to_absolute_path(const std::string& relative_path, const std::string& base_path = "");
}
