#pragma once


#include <vector>
#include <string>
#include <expected>
#include <string_view>

namespace shine::util {

	// URI工具的错误枚举
	enum class  UriError {
		InvalidDataURI,  // 无效的数据URI
		InvalidBase64,   // 无效的Base64编码
		InvalidParameter, // 无效的参数
		FileNotFound,    // 文件找不到
		InvalidPath,     // 无效的路径
		InvalidScheme,   // 无效的协议
		AccessDenied     // 访问被拒绝
	};


	// URI结构体
	struct  URI {
		std::string scheme;        // 协议部分 (http, file, asset等)
		std::string authority;     // 权限部分 (user:password@host:port)
		std::string path;          // 路径部分
		std::string query;         // 查询部分 (?key=value&key2=value2)
		std::string fragment;      // 片段部分 (#section)
	};


	// 检查字符串是否为数据URI
	bool isDataURI(std::string_view uri);


	// URL解码字符串
	std::string urlDecode(std::string_view str);

	// URL编码字符串
	std::string urlEncode(std::string_view str);

	// 将数据URI解码为二进制数据
	std::expected<std::vector<uint8_t>, UriError> decodeDataURIWithMimeType(std::string_view uri,std::string& mimeType,size_t reqBytes = 0);

	// 创建数据URI
	std::string createDataURI(const std::vector<uint8_t>& data, std::string_view mimeType, bool useBase64 = true);


	// 解析URI字符串为URI结构体
	std::expected<URI, UriError> parseURI(std::string_view uriString);

	// 将URI结构体转换为字符串
	std::string uriToString(const URI& uri);

	// 检查URI是否是本地文件路径
	bool isFileURI(std::string_view uri);

	// 检查URI是否是HTTP/HTTPS URL
	 bool isHttpURI(std::string_view uri);

	// 将本地文件路径转换为文件URI
	 std::string pathToFileURI(const std::string& path);

	// 将文件URI转换为本地文件路径
	 std::expected<std::string, UriError> fileURIToPath(std::string_view uri);

	// 组合基础URI和相对路径
	 std::expected<std::string, UriError> resolveURI(std::string_view baseURI, std::string_view relativeURI);

	// 获取URI的文件扩展名，例如.png, .json等
	 std::expected<std::string, UriError> getURIExtension(std::string_view uri);

	// 判断URI是否为资源或资产URI
	 bool isAssetURI(std::string_view uri);

	// 将资产URI转换为实际文件路径
	 std::expected<std::string, UriError> assetURIToFilePath(
		std::string_view uri,
		std::string_view assetRootDir);

	// 规范化URI路径，处理./和/
	  std::expected<std::string, UriError> normalizeURIPath(std::string_view uriPath);

}; // namespace shine::util

