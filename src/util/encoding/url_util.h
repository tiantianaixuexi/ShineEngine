#pragma once

#include <vector>
#include <expected>
#include <cstdint>

#include "../../string/shine_string.h"
#include "../../string/shine_text_view.h"

namespace shine::util {

    // URI工具的错误枚举
    enum class UriError {
        InvalidDataURI,   // 无效的数据URI
        InvalidBase64,    // 无效的Base64编码
        InvalidParameter, // 无效的参数
        FileNotFound,     // 文件找不到
        InvalidPath,      // 无效的路径
        InvalidScheme,    // 无效的协议
        AccessDenied      // 访问被拒绝
    };

    // URI结构体
    struct URI {
        SString scheme;    // 协议部分 (http, file, asset等)
        SString authority; // 权限部分 (user:password@host:port)
        SString path;      // 路径部分
        SString query;     // 查询部分 (?key=value&key2=value2)
        SString fragment;  // 片段部分 (#section)
    };

    // 检查字符串是否为数据URI
    bool isDataURI(STextView uri);

    // URL解码字符串
    SString urlDecode(STextView str);

    // URL编码字符串
    SString urlEncode(STextView str);

    // 将数据URI解码为二进制数据
    std::expected<std::vector<std::uint8_t>, UriError> decodeDataURIWithMimeType(STextView uri, SString& mimeType, size_t reqBytes = 0);

    // 创建数据URI
    SString createDataURI(const std::vector<std::uint8_t>& data, STextView mimeType, bool useBase64 = true);

    // 解析URI字符串为URI结构体
    std::expected<URI, UriError> parseURI(STextView uriString);

    // 将URI结构体转换为字符串
    SString uriToString(const URI& uri);

    // 检查URI是否是本地文件路径
    bool isFileURI(STextView uri);

    // 检查URI是否是HTTP/HTTPS URL
    bool isHttpURI(STextView uri);

    // 将本地文件路径转换为文件URI
    SString pathToFileURI(const SString& path);

    // 将文件URI转换为本地文件路径
    std::expected<SString, UriError> fileURIToPath(STextView uri);

    // 组合基础URI和相对路径
    std::expected<SString, UriError> resolveURI(STextView baseURI, STextView relativeURI);

    // 获取URI的文件扩展名，例如.png, .json等
    std::expected<SString, UriError> getURIExtension(STextView uri);

    // 判断URI是否为资源或资产URI
    bool isAssetURI(STextView uri);

    // 将资产URI转换为实际文件路径
    std::expected<SString, UriError> assetURIToFilePath(
        STextView uri,
        STextView assetRootDir);

    // 规范化URI路径，处理./和/
    std::expected<SString, UriError> normalizeURIPath(STextView uriPath);

}; // namespace shine::util