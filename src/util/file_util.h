#pragma once

#include "shine_define.h"
#include "fmt/format.h"

#include <string>
#include <vector>

#ifndef SHINE_PLATFORM_WASM
#include <expected>
#include <span>
#endif

namespace shine::util
{
	/**
	 * @brief 内存映射文件视图结构体
	 */
	struct MappedView
	{
		void* baseAddress;
#ifndef SHINE_PLATFORM_WASM
		std::span<const std::byte> content;
#else
		const std::byte* dataPtr;
		size_t dataSize;
#endif

		// 默认构造函数
		MappedView() noexcept
#ifndef SHINE_PLATFORM_WASM
			: baseAddress(nullptr), content{} {}
#else
			: baseAddress(nullptr), dataPtr(nullptr), dataSize(0) {}
#endif

		// 带地址和大小的构造函数
		MappedView(void* address, size_t size) noexcept
			: baseAddress(address)
#ifndef SHINE_PLATFORM_WASM
			, content(static_cast<const std::byte*>(address), size) {}
#else
			, dataPtr(static_cast<const std::byte*>(address)), dataSize(size) {}
#endif

		// 带单独地址和数据指针的构造函数
		MappedView(void* address, const std::byte* data, size_t size) noexcept
			: baseAddress(address)
#ifndef SHINE_PLATFORM_WASM
			, content(data, size) {}
#else
			, dataPtr(data), dataSize(size) {}
#endif

		// 禁用复制操作，仅移动
		MappedView(const MappedView&) = delete;
		MappedView& operator=(const MappedView&) = delete;

		// 移动构造函数
		MappedView(MappedView&& other) noexcept;

		// 移动赋值运算符
		MappedView& operator=(MappedView&& other) noexcept;

		// 析构函数
		~MappedView()
		{
			clear();
		}

		/**
		 * @brief 通过取消映射内存来清理映射视图
		 */
		void clear();

		// 获取数据指针
		const std::byte* data() const noexcept;

		// 数组访问运算符
		const std::byte& operator[](size_t index) const noexcept;

		// 获取数据大小访问器
		size_t size() const noexcept;

		// 检查视图是否为空
		bool empty() const noexcept;
	};

	/**
	 * @brief 文件映射结构体，根据平台提供不同的实现
	 */
	struct FileMapping
	{
#ifdef SHINE_PLATFORM_WIN
		void* fileHandle;
		void* mappingHandle;

		// 默认构造函数
		FileMapping() noexcept;

		// 带文件句柄和映射句柄的构造函数
		FileMapping(void* fileHdl, void* mappingHdl) noexcept;

		// 检查文件句柄是否有效
		bool IsValidFileHandle() const noexcept;

		// 检查映射句柄是否有效
		bool IsValidMapHandle() const noexcept;

#elif SHINE_PLATFORM_WASM
		void* data;
		size_t size;

		// 默认构造函数
		FileMapping() noexcept;

		// 带数据和大小的构造函数
		FileMapping(void* fileData, size_t fileSize) noexcept;
#endif

		// 移动构造函数
		FileMapping(FileMapping&& other) noexcept;

		// 禁用复制操作，仅移动
		FileMapping(const FileMapping&) = delete;
		FileMapping& operator=(const FileMapping&) = delete;

		// 移动赋值运算符
		FileMapping& operator=(FileMapping&& other) noexcept;

		// 析构函数
		~FileMapping()
		{
			clear();
		}

		// 检查映射是否有效
		bool IsValid() const noexcept;

		// 清理资源
		void clear();
	};

	/**
	 * @brief 组合文件映射和视图结构体
	 */
	struct FileMapView
	{
	public:
		FileMapping map;
		MappedView  view;

		// 默认构造函数
		FileMapView() = default;

		// 移动构造函数
		FileMapView(FileMapping&& _m, MappedView&& _v) noexcept;
	};

	enum class EFileFolderType {
		NONE,
		FILE,
		DIRECTORY
	};

	/**
	 * @brief 文件信息结构体
	 */
	struct FileInfo
	{
		std::string name;
		std::string path;
		EFileFolderType type;
		uint64_t size;
		uint64_t lastModified;
	};

	// ============================================================================
	// 基础文件操作
	// ============================================================================

	/**
	 * @brief 检查文件是否存在
	 * @param name 文件路径（UTF-8）
	 * @return 文件存在返回 true，否则返回 false
	 */
	bool file_exists(SString name);

	/**
	 * @brief 检查目录是否存在
	 * @param name 目录路径（UTF-8）
	 * @return 目录存在返回 true，否则返回 false
	 */
	bool directory_exists(SString name);

	/**
	 * @brief 检查文件或目录类型
	 * @param name 路径（UTF-8）
	 * @return 成功返回类型，失败返回错误
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<EFileFolderType, std::string> file_or_directory(SString name);
#else
	EFileFolderType file_or_directory(SString name, bool* success = nullptr);
#endif

	/**
	 * @brief 从给定的文件名中提取文件扩展名
	 * @param filename 文件名
	 * @return 成功返回扩展名（包含点），失败返回错误
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<std::string, std::string> get_file_suffix(SString filename);
#else
	std::string get_file_suffix(SString filename, bool* success = nullptr);
#endif

	/**
	 * @brief 获取文件名（不含路径）
	 * @param filepath 完整文件路径
	 * @return 文件名
	 */
	std::string get_file_name(SString filepath);

	/**
	 * @brief 获取文件目录路径（不含文件名）
	 * @param filepath 完整文件路径
	 * @return 目录路径
	 */
	std::string get_file_directory(SString filepath);

	/**
	 * @brief 获取文件基础名（不含扩展名）
	 * @param filepath 完整文件路径
	 * @return 基础名
	 */
	std::string get_file_stem(SString filepath);

	// ============================================================================
	// 文件映射操作（高性能大文件读取）
	// ============================================================================

	/**
	 * @brief 打开文件映射
	 * @param filename 文件路径（UTF-8）
	 * @return 成功返回文件映射，失败返回错误信息
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<FileMapping, std::string> open_file_from_mapping(SString filename);
#else
	FileMapping open_file_from_mapping(SString filename, bool* success = nullptr);
#endif

	/**
	 * @brief 从文件映射中读取数据，支持偏移量
	 * @param mapping 文件映射对象
	 * @param size 要映射的数据大小
	 * @param offset 文件中的起始偏移量，默认为0
	 * @return 成功返回映射视图，失败返回错误信息
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<MappedView, std::string> read_data_from_mapping(FileMapping& mapping, uint64_t size, uint64_t offset = 0);
#else
	MappedView read_data_from_mapping(FileMapping& mapping, uint64_t size, uint64_t offset, bool* success = nullptr);
#endif

	/**
	 * @brief 从映射中获取文件大小
	 * @param mapping 文件映射对象
	 * @return 成功返回文件大小，失败返回错误
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<uint64_t, std::string> get_file_size(FileMapping& mapping);
#else
	uint64_t get_file_size(FileMapping& mapping, bool* success = nullptr);
#endif

	/**
	 * @brief 将整个文件读取到内存映射中
	 * @param filePath 文件路径（UTF-8）
	 * @return 成功返回文件映射视图，失败返回错误信息
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<FileMapView, std::string> read_full_file(SString filePath);
#else
	FileMapView read_full_file(SString filePath, bool* success = nullptr);
#endif

	// ============================================================================
	// 文件读写操作
	// ============================================================================

	/**
	 * @brief 读取整个文件到字节数组
	 * @param filePath 文件路径（UTF-8）
	 * @return 成功返回文件内容，失败返回错误信息
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<std::vector<std::byte>, std::string> read_file_bytes(SString filePath);
#else
	std::vector<std::byte> read_file_bytes(SString filePath, bool* success = nullptr);
#endif

	/**
	 * @brief 读取整个文本文件
	 * @param filePath 文件路径（UTF-8）
	 * @return 成功返回文件内容，失败返回错误信息
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<std::string, std::string> read_file_text(SString filePath);
#else
	std::string read_file_text(SString filePath, bool* success = nullptr);
#endif

	/**
	 * @brief 写入字节数据到文件
	 * @param path 文件路径（UTF-8）
	 * @param data 数据
	 * @return 成功返回 true，失败返回 false
	 */
#ifndef SHINE_PLATFORM_WASM
	bool SaveData(SString path, std::span<const std::byte> data);
#endif
	bool SaveData(SString path, const void* data, size_t size);

	/**
	 * @brief 写入文本数据到文件
	 * @param path 文件路径（UTF-8）
	 * @param text 文本内容
	 * @return 成功返回 true，失败返回 false
	 */
	bool SaveText(SString path, SString text);

	/**
	 * @brief 追加文本到文件
	 * @param path 文件路径（UTF-8）
	 * @param text 要追加的文本
	 * @return 成功返回 true，失败返回 false
	 */
	bool AppendText(SString path, SString text);

	// ============================================================================
	// 文件管理操作
	// ============================================================================

	/**
	 * @brief 删除文件
	 * @param path 文件路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool DeleteFile(SString path);

	/**
	 * @brief 复制文件
	 * @param sourcePath 源文件路径（UTF-8）
	 * @param destPath 目标文件路径（UTF-8）
	 * @param overwrite 是否覆盖已存在的文件，默认 true
	 * @return 成功返回 true，失败返回 false
	 */
	bool CopyFile(SString sourcePath, SString destPath, bool overwrite = true);

	/**
	 * @brief 移动/重命名文件
	 * @param sourcePath 源文件路径（UTF-8）
	 * @param destPath 目标文件路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool MoveFile(SString sourcePath, SString destPath);

	/**
	 * @brief 获取文件大小（字节）
	 * @param path 文件路径（UTF-8）
	 * @return 文件大小，失败返回 0
	 */
	uint64_t GetFileSize(SString path);

	/**
	 * @brief 获取文件最后修改时间（Unix 时间戳）
	 * @param path 文件路径（UTF-8）
	 * @return 成功返回时间戳，失败返回 0
	 */
	uint64_t GetFileLastModified(SString path);

	// ============================================================================
	// 目录操作
	// ============================================================================

	/**
	 * @brief 创建目录（单层）
	 * @param path 目录路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool CreateDir(SString path);

	/**
	 * @brief 递归创建目录（创建所有必要的父目录）
	 * @param path 目录路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool CreateDirRecursive(SString path);

	/**
	 * @brief 删除目录（空目录）
	 * @param path 目录路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool DeleteDir(SString path);

	/**
	 * @brief 递归删除目录及其所有内容
	 * @param path 目录路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool DeleteDirRecursive(SString path);

	/**
	 * @brief 列出目录中的所有文件和子目录
	 * @param dirPath 目录路径（UTF-8）
	 * @param includeSubdirs 是否包含子目录，默认 false
	 * @return 成功返回文件信息列表，失败返回错误信息
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<std::vector<FileInfo>, std::string> ListDirectory(SString dirPath, bool includeSubdirs = false);
#else
	std::vector<FileInfo> ListDirectory(SString dirPath, bool includeSubdirs, bool* success = nullptr);
#endif

	/**
	 * @brief 获取当前工作目录
	 * @return 当前工作目录路径
	 */
	std::string GetCurrentDirectory();

	/**
	 * @brief 设置当前工作目录
	 * @param path 目录路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool SetCurrentDirectory(SString path);

	// ============================================================================
	// 路径操作
	// ============================================================================

	/**
	 * @brief 规范化路径（统一路径分隔符）
	 * @param path 路径
	 * @return 规范化后的路径
	 */
	std::string NormalizePath(SString path);

	/**
	 * @brief 连接路径
	 * @param base 基础路径
	 * @param part 要添加的路径部分
	 * @return 连接后的路径
	 */
	std::string JoinPath(SString base, SString part);

	/**
	 * @brief 连接多个路径部分（基础版本：单个参数）
	 * @param first 路径部分
	 * @return 路径字符串
	 */
	inline std::string JoinPath(SString first)
	{
		return std::string(first);
	}

	/**
	 * @brief 连接多个路径部分
	 * @param parts 路径部分列表
	 * @return 连接后的路径
	 */
	template<typename... Args>
	std::string JoinPath(SString first, Args... rest)
	{
		std::string result = std::string(first);
		// 使用迭代方式避免递归，确保调用非模板版本的 JoinPath
		std::string parts[] = { std::string(rest)... };
		for (const auto& part : parts)
		{
			result = JoinPath(result, part);
		}
		return result;
	}

	/**
	 * @brief 检查路径是否为绝对路径
	 * @param path 路径
	 * @return 是绝对路径返回 true，否则返回 false
	 */
	bool IsAbsolutePath(SString path);

	/**
	 * @brief 将相对路径转换为绝对路径
	 * @param path 相对路径
	 * @return 绝对路径
	 */
	std::string GetAbsolutePath(SString path);
}
