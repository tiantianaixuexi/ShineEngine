#ifdef SHINE_USE_MODULE

export module shine.util.file_util;

import <string_view>;

#else

#pragma once

#include "shine_define.h"
#include "string/shine_string.h"
#include "fmt/format.h"

#include <string>
#include <string_view>
#include <vector>

#ifndef SHINE_PLATFORM_WASM
#include <expected>
#include <span>
#endif

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
	bool file_exists(STextView name);

	/**
	 * @brief 检查目录是否存在
	 * @param name 目录路径（UTF-8）
	 * @return 目录存在返回 true，否则返回 false
	 */
	bool directory_exists(STextView name);

	/**
	 * @brief 检查文件或目录类型
	 * @param name 路径（UTF-8）
	 * @return 成功返回类型，失败返回错误
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<EFileFolderType, std::string> file_or_directory(STextView name);
#else
	EFileFolderType file_or_directory(STextView name, bool* success = nullptr);
#endif

	// ============================================================================
	// 文件映射操作（高性能大文件读取）
	// ============================================================================

	/**
	 * @brief 打开文件映射
	 * @param filename 文件路径（UTF-8）
	 * @return 成功返回文件映射，失败返回错误信息
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<FileMapping, std::string> open_file_from_mapping(STextView filename);
	std::expected<FileMapping, std::string> open_file_from_mapping(std::string_view filename);
#else
	FileMapping open_file_from_mapping(STextView filename, bool* success = nullptr);
	FileMapping open_file_from_mapping(std::string_view filename, bool* success = nullptr);
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
	std::expected<FileMapView, std::string> read_full_file(STextView filePath);
	std::expected<FileMapView, std::string> read_full_file(std::string_view filePath);
#else
	FileMapView read_full_file(STextView filePath, bool* success = nullptr);
	FileMapView read_full_file(std::string_view filePath, bool* success = nullptr);
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
	std::expected<std::vector<std::byte>, std::string> read_file_bytes(STextView filePath);
	std::expected<std::vector<std::byte>, std::string> read_file_bytes(std::string_view filePath);
#else
	std::vector<std::byte> read_file_bytes(STextView filePath, bool* success = nullptr);
	std::vector<std::byte> read_file_bytes(std::string_view filePath, bool* success = nullptr);
#endif

	/**
	 * @brief 读取整个文本文件
	 * @param filePath 文件路径（UTF-8）
	 * @return 成功返回文件内容，失败返回错误信息
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<std::string, std::string> read_file_text(STextView filePath);
	std::expected<std::string, std::string> read_file_text(std::string_view filePath);
#else
	std::string read_file_text(STextView filePath, bool* success = nullptr);
	std::string read_file_text(std::string_view filePath, bool* success = nullptr);
#endif

	/**
	 * @brief 写入字节数据到文件
	 * @param path 文件路径（UTF-8）
	 * @param data 数据
	 * @return 成功返回 true，失败返回 false
	 */
#ifndef SHINE_PLATFORM_WASM
	bool SaveData(STextView path, std::span<const std::byte> data);
#endif
	bool SaveData(STextView path, const void* data, size_t size);

	/**
	 * @brief 写入文本数据到文件
	 * @param path 文件路径（UTF-8）
	 * @param text 文本内容
	 * @return 成功返回 true，失败返回 false
	 */
	bool SaveText(STextView path, STextView text);

	/**
	 * @brief 追加文本到文件
	 * @param path 文件路径（UTF-8）
	 * @param text 要追加的文本
	 * @return 成功返回 true，失败返回 false
	 */
	bool AppendText(STextView path, STextView text);

	// ============================================================================
	// 文件管理操作
	// ============================================================================

	/**
	 * @brief 删除文件
	 * @param path 文件路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool DeleteFile(STextView path);

	/**
	 * @brief 复制文件
	 * @param sourcePath 源文件路径（UTF-8）
	 * @param destPath 目标文件路径（UTF-8）
	 * @param overwrite 是否覆盖已存在的文件，默认 true
	 * @return 成功返回 true，失败返回 false
	 */
	bool CopyFile(STextView sourcePath, STextView destPath, bool overwrite = true);

	/**
	 * @brief 移动/重命名文件
	 * @param sourcePath 源文件路径（UTF-8）
	 * @param destPath 目标文件路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool MoveFile(STextView sourcePath, STextView destPath);

	/**
	 * @brief 获取文件大小（字节）
	 * @param path 文件路径（UTF-8）
	 * @return 文件大小，失败返回 0
	 */
	uint64_t GetFileSize(STextView path);

	/**
	 * @brief 获取文件最后修改时间（Unix 时间戳）
	 * @param path 文件路径（UTF-8）
	 * @return 成功返回时间戳，失败返回 0
	 */
	uint64_t GetFileLastModified(STextView path);

	// ============================================================================
	// 目录操作
	// ============================================================================

	/**
	 * @brief 创建目录（单层）
	 * @param path 目录路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool CreateDir(STextView path);

	/**
	 * @brief 递归创建目录（创建所有必要的父目录）
	 * @param path 目录路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool CreateDirRecursive(STextView path);

	/**
	 * @brief 删除目录（空目录）
	 * @param path 目录路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool DeleteDir(STextView path);

	/**
	 * @brief 递归删除目录及其所有内容
	 * @param path 目录路径（UTF-8）
	 * @return 成功返回 true，失败返回 false
	 */
	bool DeleteDirRecursive(STextView path);

	/**
	 * @brief 列出目录中的所有文件和子目录
	 * @param dirPath 目录路径（UTF-8）
	 * @param includeSubdirs 是否包含子目录，默认 false
	 * @return 成功返回文件信息列表，失败返回错误信息
	 */
#ifndef SHINE_PLATFORM_WASM
	std::expected<std::vector<FileInfo>, std::string> ListDirectory(STextView dirPath, bool includeSubdirs = false);
#else
	std::vector<FileInfo> ListDirectory(STextView dirPath, bool includeSubdirs, bool* success = nullptr);
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
	bool SetCurrentDirectory(STextView path);

}
