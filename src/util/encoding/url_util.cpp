#include "url_util.h"

#include <algorithm>
#include <vector>
#include <expected>

#include "util/string_util.ixx"
#include "util/path_util.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::util {

    bool isDataURI(STextView uri) {
        return uri.size() > 5 && uri.starts_with("data:");
    }

    // 检查URI是否是本地文件路径
    bool isFileURI(STextView uri) {
        return uri.size() > 7 &&
            (uri.starts_with("file://") ||
                uri.starts_with("file:///"));
    }

    // 检查URI是否是HTTP/HTTPS URL
    bool isHttpURI(STextView uri) {
        return (uri.size() > 7 && uri.starts_with("http://")) ||
            (uri.size() > 8 && uri.starts_with("https://"));
    }

    // 辅助函数实现
    SString getParentPath(STextView path) {
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash == STextView::npos) {
            return {};
        }
        return SString::from_view(path.substr(0, lastSlash));
    }

    // 将本地文件路径转换为文件URI
    SString pathToFileURI(const SString& path) {
        SString absolutePath = path;

        // 如果不是绝对路径，假设相对于当前目录
        if (!is_absolute_path(STextView(absolutePath.sv()))) {
            absolutePath = path;
        }

        SString result = "file://";

        // 在Windows中，路径需要特殊处理
#ifdef _WIN32
        // 移除驱动器前的斜杠，添加额外斜杠
        if (absolutePath.size() > 2 && absolutePath[1] == ':') {
            // 例如，C:\path 变为 file:///C:/path
            result.append("/");
            // 替换反斜杠为正斜杠
            std::ranges::replace(absolutePath, '\\', '/');
        }
#endif

        for (char c : absolutePath) {
            if (c == ' ') {
                result.append("%20");
            }
            else {
                result.push_back(c);
            }
        }

        return result;
    }

    // 创建数据URI
    SString createDataURI(const std::vector<uint8_t>& data, STextView mimeType, bool useBase64) {
        SString uri = "data:";

        // 添加MIME类型
        if (!mimeType.empty()) {
            uri.append(SString::from_view(mimeType));
        }
        else {
            uri.append("application/octet-stream");
        }

        if (useBase64) {
            uri.append(";base64,");
        }
        else {
            uri.push_back(',');
            // 对二进制数据进行URL编码
            for (unsigned char byte : data) {
                if (StringUtil::isAlphaNumeric(byte) || byte == '-' || byte == '_' || byte == '.' || byte == '~') {
                    uri.push_back(static_cast<char>(byte));
                }
                else {
                    uri.push_back('%');
                    uri.push_back(StringUtil::toHex(byte >> 4));
                    uri.push_back(StringUtil::toHex(byte & 0xF));
                }
            }
        }

        return uri;
    }

    // URL解码
    SString urlDecode(STextView str) {
        SString result;
        result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size() &&
            StringUtil::isAlphaNumericHex(static_cast<unsigned char>(str[i + 1])) &&
            StringUtil::isAlphaNumericHex(static_cast<unsigned char>(str[i + 2]))) {
            unsigned char high = StringUtil::fromHex(static_cast<unsigned char>(str[i + 1]));
            unsigned char low = StringUtil::fromHex(static_cast<unsigned char>(str[i + 2]));
            result.push_back(static_cast<char>(high * 16 + low));
            i += 2; // 跳过已处理的 % 和两个十六进制字符
        }
        else if (str[i] == '+') {
            result.push_back(' ');
        }
        else {
            result.push_back(str[i]);
        }
    }

        return result;
    }

    // URL编码
    SString urlEncode(STextView str) {
        SString result;
        result.reserve(str.size() * 3);

        for (unsigned char c : str) {
            if (StringUtil::isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                result.push_back(static_cast<char>(c));
            }
            else if (c == ' ') {
                result.append("%20");
            }
            else {
                result.push_back('%');
                result.push_back(StringUtil::toHex(c >> 4));
                result.push_back(StringUtil::toHex(c & 0xF));
            }
        }

        return result;
    }

    std::expected<std::vector<uint8_t>, UriError> decodeDataURIWithMimeType(
        STextView uri,
        SString& mimeType,
        size_t reqBytes)
    {
        if (!isDataURI(uri)) {
            return std::unexpected(UriError::InvalidDataURI);
        }

        SString decoded_uri = urlDecode(uri);

        auto comma_pos = decoded_uri.find(STextView(",", 1));
        if (comma_pos == SString::npos) {
            return std::unexpected(UriError::InvalidDataURI);
        }

        STextView decoded_view(decoded_uri.sv());
        STextView meta_view = decoded_view.substr(5, comma_pos - 5);
        STextView data_view = decoded_view.substr(comma_pos + 1, decoded_view.size() - comma_pos - 1);

        bool is_base64 = meta_view.contains(STextView(";base64", 7));

        STextView mime_view = meta_view;
        if (is_base64) {
            size_t pos = meta_view.find(STextView(";base64", 7));
            mime_view = meta_view.substr(0, pos);
        }

        if (mime_view.empty() || mime_view.front() == ';') {
            mimeType = "text/plain";
        }
        else {
            size_t semicolon_pos = mime_view.find(STextView(";", 1));
            if (semicolon_pos != STextView::npos) {
                mimeType = SString::from_view(mime_view.substr(0, semicolon_pos));
            }
            else {
                mimeType = SString::from_view(mime_view);
            }
        }

        std::vector<uint8_t> result;
        if (is_base64) {
            return std::unexpected(UriError::InvalidDataURI);
        }
        else {
            result.assign(data_view.begin(), data_view.end());
        }

        if (reqBytes > 0 && result.size() != reqBytes) {
            return std::unexpected(UriError::InvalidDataURI);
        }

        return result;
    }

    std::expected<URI, UriError> parseURI(STextView uriString) {
        URI result;

        STextView remaining = uriString;
        if (auto scheme_end = remaining.find(STextView(":", 1)); scheme_end != STextView::npos) {
            result.scheme = SString::from_view(remaining.substr(0, scheme_end));
            remaining = remaining.substr(scheme_end + 1, remaining.size() - scheme_end - 1);
        }

        if (remaining.starts_with(STextView("//", 2))) {
            remaining = remaining.substr(2, remaining.size() - 2);
            if (auto authority_end = remaining.find_first_of(STextView("/?#", 3)); authority_end != STextView::npos) {
                result.authority = SString::from_view(remaining.substr(0, authority_end));
                remaining = remaining.substr(authority_end, remaining.size() - authority_end);
            }
            else {
                result.authority = SString::from_view(remaining);
                remaining = {};
            }
        }

        if (auto path_end = remaining.find_first_of(STextView("?#", 2)); path_end != STextView::npos) {
            result.path = SString::from_view(remaining.substr(0, path_end));
            remaining = remaining.substr(path_end, remaining.size() - path_end);
        }
        else {
            result.path = SString::from_view(remaining);
            remaining = {};
        }

        if (!remaining.empty() && remaining.front() == '?') {
            remaining = remaining.substr(1, remaining.size() - 1);
            if (auto query_end = remaining.find(STextView("#", 1)); query_end != STextView::npos) {
                result.query = SString::from_view(remaining.substr(0, query_end));
                remaining = remaining.substr(query_end, remaining.size() - query_end);
            }
            else {
                result.query = SString::from_view(remaining);
                remaining = {};
            }
        }

        if (!remaining.empty() && remaining.front() == '#') {
            result.fragment = SString::from_view(remaining.substr(1, remaining.size() - 1));
        }

        return result;
    }

    SString uriToString(const URI& uri) {
        SString result;
        if (!uri.scheme.empty()) { result.append(uri.scheme); result.push_back(':'); }
        if (!uri.authority.empty()) { result.append("//"); result.append(uri.authority); }
        result.append(uri.path);
        if (!uri.query.empty()) { result.push_back('?'); result.append(uri.query); }
        if (!uri.fragment.empty()) { result.push_back('#'); result.append(uri.fragment); }
        return result;
    }

    std::expected<SString, UriError> fileURIToPath(STextView uri) {
        if (!isFileURI(uri)) return std::unexpected(UriError::InvalidScheme);

        SString path = urlDecode(uri.starts_with(STextView("file:///", 8)) ? uri.substr(8, uri.size() - 8) : uri.substr(7, uri.size() - 7));

#ifdef _WIN32
        if (path.size() > 2 && path[0] == '/' && path[2] == ':') {
            path = path.substr(1, path.size() - 1);
            std::ranges::replace(path, '/', '\\');
        }
#endif
        return path;
    }

    std::expected<SString, UriError> resolveURI(STextView baseURI, STextView relativeURI) {
        if (relativeURI.find(STextView(":", 1)) != STextView::npos) return SString::from_view(relativeURI);

        auto baseUriResult = parseURI(baseURI);
        if (!baseUriResult) return std::unexpected(UriError::InvalidParameter);

        const URI& base = *baseUriResult;
        if (relativeURI.empty()) return uriToString(base);

        URI result = base;
        if (relativeURI.front() == '/') {
            result.path = SString::from_view(relativeURI);
            result.query = ""; result.fragment = "";
        }
        else if (relativeURI.front() == '?') {
            result.query = SString::from_view(relativeURI.substr(1, relativeURI.size() - 1));
            result.fragment = "";
        }
        else if (relativeURI.front() == '#') {
            result.fragment = SString::from_view(relativeURI.substr(1, relativeURI.size() - 1));
        }
        else {
            SString combinedPath = join_path(getParentPath(STextView(base.path.sv())), relativeURI);
            result.path = combinedPath;
            result.query = ""; result.fragment = "";
        }
        return uriToString(result);
    }

    std::expected<SString, UriError> getURIExtension(STextView uri) {
        auto uriResult = parseURI(uri);
        if (!uriResult) return std::unexpected(UriError::InvalidParameter);

        SString path = uriResult->path;
        size_t dotPos = path.find_last_of(STextView(".", 1));
        if (dotPos == SString::npos || dotPos == path.size() - 1) return std::unexpected(UriError::InvalidPath);
        return path.substr(dotPos, path.size() - dotPos);
    }

    bool isAssetURI(STextView uri) {
        return uri.size() > 8 && uri.starts_with("asset://");
    }

    std::expected<SString, UriError> assetURIToFilePath(STextView uri, STextView assetRootDir) {
        if (!isAssetURI(uri)) return std::unexpected(UriError::InvalidScheme);
        SString assetPath = urlDecode(uri.substr(8, uri.size() - 8));
        return join_path(assetRootDir, STextView(assetPath.sv()));
    }

    std::expected<SString, UriError> normalizeURIPath(STextView uriPath) {
        return normalize_path(SString::from_view(uriPath));
    }

    SString getBaseDir(STextView path)
    {
        const auto _index = path.find_last_of("/\\");

        if(_index == STextView::npos){
            return "";
        }

        return SString::from_view(path.substr(0,_index+1));
    }

    SString getBaseFileName(STextView path)
    {
        const auto _index = path.find_last_of("/\\");
        if(_index == STextView::npos)
        {
            return SString::from_view(path);
        }

        return SString::from_view(path.substr(_index+1));
    }
    
}
