#pragma once



#include "fmt/format.h"



#include <string>
#include <expected>
#include <span>




namespace shine::util
{
	/**
	 * @brief 内存映射文件视图结构体
	 */
	struct  MappedView
	{
		void* baseAddress;
		std::span<const std::byte> content;

		// 默认构造函数
		MappedView() : baseAddress(nullptr), content{} {}

		// 带地址和大小的构造函数
		MappedView(void* address, size_t size)
			: baseAddress(address),
			content(static_cast<const std::byte*>(address), size) {
		}

		// 带单独地址和数据指针的构造函数
		MappedView(void* address, const std::byte* data, size_t size)
			: baseAddress(address),
			content(data, size) {
		}

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
	struct  FileMapping
	{
#if defined(WIN32)
		void* fileHandle;
		void* mappingHandle;

		// 默认构造函数
		FileMapping();

		// 带文件句柄和映射句柄的构造函数
		FileMapping(void* fileHdl, void* mappingHdl);

#elif defined(PLATFORM_WASM)
		void* data;
		size_t size;

		// 默认构造函数
		FileMapping();

		// 带数据和大小的构造函数
		FileMapping(void* fileData, size_t fileSize);
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

#if defined(WIN32)
		// 检查文件句柄是否有效
		bool IsValidFileHandle() const noexcept;

		// 检查映射句柄是否有效
		bool IsValidMapHandle() const noexcept;
#endif

		// 检查映射是否有效
		bool IsValid() const noexcept;

		// 清理资源
		void clear();
	};


	/**
	 * @brief 组合文件映射和视图结构体
	 */
	struct  FileMapView
	{
	public:
		FileMapping map;
		MappedView  view;

		// 默认构造函数
		FileMapView();

		// 移动构造函数
		FileMapView(FileMapping&& _m, MappedView&& _v);
	};


	enum class  EFileFolderType {
		NONE,
		FILE,
		DIRECTORY
	};

	// 仅支持UTF-8字符串
	 bool file_exists(std::string_view name);

	/**
	* @brief 从给定的文件名中提取文件扩展名。
	*
	* 此函数从提供的文件名中提取扩展名（后缀）。
	* 如果文件名不包含点'.'，表示没有扩展名，
	* 函数返回包含'false'的expected。
	*
	* @param filename 要从中提取扩展名的文件名。
	* @return 如果成功，返回包含文件扩展名字符串的expected，
	*         如果未找到扩展名，则返回包含'false'的expected。
	*/
	 std::expected<std::string, bool> get_file_suffix(const std::string& filename);

	// 文件映射函数
	 std::expected<FileMapping, std::string> open_file_from_mapping(const std::string& filename);

	// 检查文件/目录类型
	 std::expected<EFileFolderType, bool> file_or_directory(const std::string& name);

	/**
	* @brief 从文件映射中读取数据，支持偏移量。
	*
	* 此函数将文件映射到进程地址空间并返回
	* 引用映射内存的视图。支持从指定的
	* 文件偏移位置开始读取。
	*
	* @param mapping 文件映射对象
	* @param size 要映射的数据大小
	* @param offset 文件中的起始偏移量。默认为0，表示从文件开始读取。
	* @return std::expected<MappedView, std::string>
	*         成功时，包含引用映射内存的视图。
	*         失败时，包含错误消息。
	*/
	 std::expected<MappedView, std::string> read_data_from_mapping(FileMapping& mapping, uint64_t size, uint64_t offset = 0);

	// 从映射中获取文件大小
	 std::expected<uint64_t, bool> get_file_size(FileMapping& mapping);

	// 将整个文件读取到内存映射中
	 std::expected <FileMapView, std::string > read_full_file(std::string_view filePath);






	 unsigned int GetFileSize(std::string_view path);

	 bool SaveData(std::string_view path, std::span<const std::byte> data);
	 bool SaveData(std::string_view path, const void* data, size_t size);

	 bool CreateDir(std::string_view path);

}

