#include "url_util.h"


#include <algorithm>
#include <vector>
#include <string_view>
#include <string>
#include <expected>


#include "util/string_util.h"

namespace shine::util {

    // 辅助函数声明
    bool isAbsolutePath(std::string_view path);
    std::string getParentPath(std::string_view path);
    std::string normalizePath(std::string path);
    std::string combinePath(std::string_view base, std::string_view relative);

    bool isDataURI(std::string_view uri) {
        return uri.size() > 5 && uri.substr(0, 5) == "data:";
    }

    // 检查URI是否是本地文件路径
    bool isFileURI(std::string_view uri) {
        return uri.size() > 7 &&
            (uri.substr(0, 7) == "file://" ||
                uri.substr(0, 8) == "file:///");
    }

    // 检查URI是否是HTTP/HTTPS URL
    bool isHttpURI(std::string_view uri) {
        return (uri.size() > 7 && uri.substr(0, 7) == "http://") ||
            (uri.size() > 8 && uri.substr(0, 8) == "https://");
    }

    // 辅助函数实现
    bool isAbsolutePath(std::string_view path) {
#ifdef _WIN32
        // Windows 下支持盘符路径 (C:\...) 和 UNC 路径 (\\server\share 或 //server/share)
        if (path.size() >= 2 && path[1] == ':') {
            return true;
        }
        if (path.size() >= 2 &&
            ((path[0] == '\\' && path[1] == '\\') || (path[0] == '/' && path[1] == '/'))) {
            return true;
        }
        return false;
#else
        return !path.empty() && path[0] == '/';
#endif
    }

    std::string getParentPath(std::string_view path) {
        auto lastSlash = path.find_last_of("/\\");
        if (lastSlash == std::string_view::npos) {
            return "";
        }
        return std::string(path.substr(0, lastSlash));
    }

    std::string normalizePath(std::string path) {
        // 统一路径分隔符为 '/'
        std::replace(path.begin(), path.end(), '\\', '/');

        // 分割路径组件
        std::vector<std::string> components;

        // 处理绝对路径前缀
        bool isAbsolute = !path.empty() && path.front() == '/';

        // 分隔路径
        size_t start = isAbsolute ? 1 : 0;
        while (start < path.size()) {
            auto pos = path.find('/', start);
            if (pos == std::string::npos) pos = path.size();

            std::string_view component(path.data() + start, pos - start);

            if (!component.empty() && component != ".") {
                if (component == "..") {
                    if (!components.empty() && components.back() != "..") {
                        // 向上返回一层目录
                        components.pop_back();
                    }
                    else if (!isAbsolute) {
                        // 相对路径前导的 ".." 需要保留，例如 "../a/b"
                        components.emplace_back("..");
                    }
                }
                else {
                    components.emplace_back(component);
                }
            }

            start = pos + 1;
        }

        // 重新组合路径
        std::string result;
        if (isAbsolute && !components.empty()) {
            result = "/";
        }

        for (size_t i = 0; i < components.size(); ++i) {
            result += components[i];
            if (i < components.size() - 1) {
                result += "/";
            }
        }

        // 如果路径为空，返回当前目录
        if (result.empty()) {
            result = isAbsolute ? "/" : ".";
        }

        return result;
    }

    std::string combinePath(std::string_view base, std::string_view relative) {
        if (base.empty()) return std::string(relative);
        if (relative.empty()) return std::string(base);

        // 如果相对路径是绝对路径，直接返回
        if (isAbsolutePath(relative)) {
            return std::string(relative);
        }

        std::string result(base);

        // 检查是否需要添加分隔符
        if (!result.empty() && result.back() != '/' && result.back() != '\\') {
            result += '/';
        }
        result += relative;

        return normalizePath(result);
    }

    // 将本地文件路径转换为文件URI
    std::string pathToFileURI(const std::string& path) {
        std::string absolutePath = path;

        // 如果不是绝对路径，假设相对于当前目录
        if (!isAbsolutePath(absolutePath)) {
            // 这里我们不实现实际的绝对路径转换，而是保留原路径
            // 实际应用中可能需要根据具体需求处理
            absolutePath = path;
        }

        std::string result = "file://";

        // 在Windows中，路径需要特殊处理
#ifdef _WIN32
        // 移除驱动器前的斜杠，添加额外斜杠
        if (absolutePath.size() > 2 && absolutePath[1] == ':') {
            // 例如，C:\path 变为 file:///C:/path
            result += "/";
            // 替换反斜杠为正斜杠
            absolutePath = StringUtil::ReplaceAll(absolutePath, "\\", "/");
        }
#endif

        for (char c : absolutePath) {
            if (c == ' ') {
                result += "%20";
            }
            else {
                result += c;
            }
        }

        return result;
    }

    // 创建数据URI
    std::string createDataURI(const std::vector<uint8_t>& data, std::string_view mimeType, bool useBase64) {
        std::string uri = "data:";

        // 添加MIME类型
        if (!mimeType.empty()) {
            uri += mimeType;
        }
        else {
            uri += "application/octet-stream";
        }

        if (useBase64) {
            uri += ";base64,";
            // uri += base64::base64_encode(data);  // 暂时注释，避免模块冲突
            // TODO: 实现base64编码或使用其他方法
        }
        else {
            uri += ",";
            // 对二进制数据进行URL编码
            for (unsigned char byte : data) {
                if (StringUtil::isAlphaNumeric(byte) || byte == '-' || byte == '_' || byte == '.' || byte == '~') {
                    uri += static_cast<char>(byte);
                }
                else {
                    uri += '%';
                    uri += StringUtil::toHex(byte >> 4);
                    uri += StringUtil::toHex(byte & 0xF);
                }
            }
        }

        return uri;
    }

    // URL解码
    std::string urlDecode(std::string_view str) {
        std::string result;
        result.reserve(str.size());

        // 使用ranges处理字符
        for (auto it = str.begin(); it != str.end(); ++it) {
            if (*it == '%' && std::distance(it, str.end()) > 2) {
                // 处理URL编码的字符，例如%20表示空格
                unsigned char high = StringUtil::fromHex(static_cast<unsigned char>(*(it + 1)));
                unsigned char low = StringUtil::fromHex(static_cast<unsigned char>(*(it + 2)));
                result += static_cast<char>(high * 16 + low);
                it += 2;
            }
            else if (*it == '+') {
                // '+'在URL编码中表示空格
                result += ' ';
            }
            else {
                result += *it;
            }
        }

        return result;
    }

    // URL编码
    std::string urlEncode(std::string_view str) {
        std::string result;
        result.reserve(str.size() * 3); // 预留足够空间

        // 使用ranges算法处理
        for (unsigned char c : str) {
            if (StringUtil::isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                // 不需要编码的字符
                result += c;
            }
            else if (c == ' ') {
                // 空格可以编码为'+'或'%20'，这里使用'%20'更通用
                result += "%20";
            }
            else {
                // 其他字符需要百分号编码
                result += '%';
                result += StringUtil::toHex(c >> 4);
                result += StringUtil::toHex(c & 0xF);
            }
        }

        return result;
    }

    std::expected<std::vector<uint8_t>, UriError> decodeDataURIWithMimeType(
        std::string_view uri,
        std::string& mimeType,
        size_t reqBytes)
    {
        if (!isDataURI(uri)) {
            return std::unexpected(UriError::InvalidDataURI);
        }

        std::string decoded_uri = urlDecode(uri);

        // data:[<mime_type>][;charset=<charset>][;base64],<data>
        auto comma_pos = decoded_uri.find(',', 5);
        if (comma_pos == std::string::npos) {
            return std::unexpected(UriError::InvalidDataURI);
        }

        std::string_view meta_view = std::string_view(decoded_uri).substr(5, comma_pos - 5);
        std::string_view data_view = std::string_view(decoded_uri).substr(comma_pos + 1);

        // 检查是否指定了base64编码
        bool is_base64 = meta_view.find(";base64") != std::string_view::npos;

        // 提取MIME类型
        std::string_view mime_view = meta_view;
        if (is_base64) {
            mime_view = meta_view.substr(0, meta_view.find(";base64"));
        }

        if (mime_view.empty() || mime_view.starts_with(';')) {
            // 默认MIME类型
            mimeType = "text/plain";
        }
        else {
            auto semicolon_pos = mime_view.find(';');
            if (semicolon_pos != std::string_view::npos) {
                mimeType = std::string(mime_view.substr(0, semicolon_pos));
            }
            else {
                mimeType = std::string(mime_view);
            }
        }

        // 解码数据
        std::vector<uint8_t> result;
        if (is_base64) {
            // result = base64::base64_decode(data);  // 暂时注释，避免模块冲突
            // TODO: 实现base64解码或使用其他方法
            return std::unexpected(UriError::InvalidDataURI);
        }
        else {
            result.assign(data_view.begin(), data_view.end());
        }

        // 检查所需字节数
        if (reqBytes > 0 && result.size() != reqBytes) {
            return std::unexpected(UriError::InvalidDataURI);
        }

        return result;
    }

    std::expected<URI, UriError> parseURI(std::string_view uriString) {
        URI result;

        // 1. scheme
        if (auto scheme_end = uriString.find(':'); scheme_end != std::string_view::npos) {
            result.scheme = std::string(uriString.substr(0, scheme_end));
            uriString.remove_prefix(scheme_end + 1);
        }

        // 2. authority
        if (uriString.starts_with("//")) {
            uriString.remove_prefix(2);
            if (auto authority_end = uriString.find_first_of("/?#"); authority_end != std::string_view::npos) {
                result.authority = std::string(uriString.substr(0, authority_end));
                uriString.remove_prefix(authority_end);
            }
            else {
                result.authority = std::string(uriString);
                uriString = {};
            }
        }

        // 3. path
        if (auto path_end = uriString.find_first_of("?#"); path_end != std::string_view::npos) {
            result.path = std::string(uriString.substr(0, path_end));
            uriString.remove_prefix(path_end);
        }
        else {
            result.path = std::string(uriString);
            uriString = {};
        }

        // 4. query
        if (!uriString.empty() && uriString.front() == '?') {
            uriString.remove_prefix(1);
            if (auto query_end = uriString.find('#'); query_end != std::string_view::npos) {
                result.query = std::string(uriString.substr(0, query_end));
                uriString.remove_prefix(query_end);
            }
            else {
                result.query = std::string(uriString);
                uriString = {};
            }
        }

        // 5. fragment
        if (!uriString.empty() && uriString.front() == '#') {
            uriString.remove_prefix(1);
            result.fragment = std::string(uriString);
        }

        return result;
    }

    // 将URI结构体转换为字符串
    std::string uriToString(const URI& uri) {
        std::string result;

        if (!uri.scheme.empty()) {
            result += uri.scheme;
            result += ":";
        }

        if (!uri.authority.empty()) {
            result += "//";
            result += uri.authority;
        }

        result += uri.path;

        if (!uri.query.empty()) {
            result += "?";
            result += uri.query;
        }

        if (!uri.fragment.empty()) {
            result += "#";
            result += uri.fragment;
        }

        return result;
    }

    // 将文件URI转换为本地文件路径
    std::expected<std::string, UriError> fileURIToPath(std::string_view uri) {
        if (!isFileURI(uri)) {
            return std::unexpected(UriError::InvalidScheme);
        }

        std::string path;

        if (uri.substr(0, 8) == "file:///") {
            path = std::string(uri.substr(8));
        }
        else {
            path = std::string(uri.substr(7));
        }

        path = urlDecode(path);

#ifdef _WIN32
        // 在Windows中处理驱动器号
        if (path.size() > 2 && path[0] == '/' && path[2] == ':') {
            // 例如，C:/path 变为 C:\path
            path = path.substr(1);
            path = StringUtil::ReplaceAll(path, "/", "\\");
        }
#endif


        return path;
    }

    // 组合基础URI和相对路径
    std::expected<std::string, UriError> resolveURI(std::string_view baseURI, std::string_view relativeURI) {
        // 如果相对URI是绝对路径，直接返回
        if (relativeURI.find(':') != std::string_view::npos) {
            return std::string(relativeURI);
        }

        auto baseUriResult = parseURI(baseURI);
        if (!baseUriResult) {
            return std::unexpected(UriError::InvalidParameter);
        }

        URI base = *baseUriResult;

        if (relativeURI.empty()) {
            return uriToString(base);
        }

        URI result = base;

        if (relativeURI[0] == '/') {
            // 绝对路径
            result.path = std::string(relativeURI);
            result.query = "";
            result.fragment = "";
        }
        else if (relativeURI[0] == '?') {
            // 只有查询部分
            result.query = std::string(relativeURI.substr(1));
            result.fragment = "";
        }
        else if (relativeURI[0] == '#') {
            // 只有片段部分
            result.fragment = std::string(relativeURI.substr(1));
        }
        else {
            // 相对路径
            std::string parentPath = getParentPath(base.path);
            std::string combinedPath = combinePath(parentPath, std::string(relativeURI));

            result.path = combinedPath;
            result.query = "";
            result.fragment = "";
        }

        return uriToString(result);
    }

    // 获取URI的文件扩展名
    std::expected<std::string, UriError> getURIExtension(std::string_view uri) {
        auto uriResult = parseURI(uri);
        if (!uriResult) {
            return std::unexpected(UriError::InvalidParameter);
        }

        URI parsedUri = *uriResult;
        std::string path = parsedUri.path;

        // 查找最后一个点的位
        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string::npos || dotPos == path.size() - 1) {
            return std::unexpected(UriError::InvalidPath);
        }

        return path.substr(dotPos);
    }

    // 判断URI是否为资源或资产URI
    bool isAssetURI(std::string_view uri) {
        return uri.size() > 8 && uri.substr(0, 8) == "asset://";
    }

    // 将资产URI转换为实际文件路径
    std::expected<std::string, UriError> assetURIToFilePath(
        std::string_view uri,
        std::string_view assetRootDir) {

        if (!isAssetURI(uri)) {
            return std::unexpected(UriError::InvalidScheme);
        }

        // asset://texture/monster.png => assetRootDir/texture/monster.png
        std::string assetPath = std::string(uri.substr(8)); // 移除 "asset://" 前缀
        assetPath = urlDecode(assetPath);

        // 构建完整文件路径
        std::string fullPath = combinePath(std::string(assetRootDir), assetPath);



        return fullPath;
    }

    // 规范化URI路径，处理./和/
    std::expected<std::string, UriError> normalizeURIPath(std::string_view uriPath) {
        std::string path(uriPath);
        return normalizePath(path);
    }
}


