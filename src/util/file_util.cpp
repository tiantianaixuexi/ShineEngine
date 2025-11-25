#include "file_util.h"
#include "shine_define.h"
#include "fmt/format.h"

#ifdef SHINE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <fileapi.h>
#include <io.h>
#elif SHINE_PLATFORM_WASM
#include <emscripten.h>
#include <cstdlib>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <cerrno>
#endif

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cerrno>

#ifndef SHINE_PLATFORM_WASM
#include <expected>
#include <span>
#endif

namespace shine::util
{
	// ============================================================================
	// MappedView 实现
	// ============================================================================

	MappedView::MappedView(MappedView&& other) noexcept
		: baseAddress(other.baseAddress)
#ifndef SHINE_PLATFORM_WASM
		, content(other.content)
#else
		, dataPtr(other.dataPtr), dataSize(other.dataSize)
#endif
	{
		other.baseAddress = nullptr;
#ifndef SHINE_PLATFORM_WASM
		other.content = {};
#else
		other.dataPtr = nullptr;
		other.dataSize = 0;
#endif
	}

	MappedView& MappedView::operator=(MappedView&& other) noexcept
	{
		if (this != &other)
		{
			clear();
			baseAddress = other.baseAddress;
#ifndef SHINE_PLATFORM_WASM
			content = other.content;
			other.content = {};
#else
			dataPtr = other.dataPtr;
			dataSize = other.dataSize;
			other.dataPtr = nullptr;
			other.dataSize = 0;
#endif
			other.baseAddress = nullptr;
		}
		return *this;
	}

	void MappedView::clear()
	{
#ifdef SHINE_PLATFORM_WIN
		if (baseAddress)
		{
			UnmapViewOfFile(baseAddress);
			baseAddress = nullptr;
#ifndef SHINE_PLATFORM_WASM
			content = {};
#else
			dataPtr = nullptr;
			dataSize = 0;
#endif
		}
#elif SHINE_PLATFORM_WASM
		if (baseAddress)
		{
			free(baseAddress);
			baseAddress = nullptr;
			dataPtr = nullptr;
			dataSize = 0;
		}
#endif
	}

	const std::byte* MappedView::data() const noexcept
	{
#ifndef SHINE_PLATFORM_WASM
		return content.data();
#else
		return dataPtr;
#endif
	}

	const std::byte& MappedView::operator[](size_t index) const noexcept
	{
		return data()[index];
	}

	size_t MappedView::size() const noexcept
	{
#ifndef SHINE_PLATFORM_WASM
		return content.size();
#else
		return dataSize;
#endif
	}

	bool MappedView::empty() const noexcept
	{
#ifndef SHINE_PLATFORM_WASM
		return content.empty();
#else
		return dataSize == 0;
#endif
	}

	// ============================================================================
	// FileMapping 实现
	// ============================================================================

#ifdef SHINE_PLATFORM_WIN
	FileMapping::FileMapping() noexcept
		: fileHandle(nullptr), mappingHandle(nullptr) {}

	FileMapping::FileMapping(void* fileHdl, void* mappingHdl) noexcept
		: fileHandle(fileHdl), mappingHandle(mappingHdl) {}

	bool FileMapping::IsValidFileHandle() const noexcept
	{
		return fileHandle != nullptr && fileHandle != INVALID_HANDLE_VALUE;
	}

	bool FileMapping::IsValidMapHandle() const noexcept
	{
		return mappingHandle != nullptr && mappingHandle != INVALID_HANDLE_VALUE;
	}

#elif SHINE_PLATFORM_WASM
	FileMapping::FileMapping() noexcept
		: data(nullptr), size(0) {}

	FileMapping::FileMapping(void* fileData, size_t fileSize) noexcept
		: data(fileData), size(fileSize) {}
#endif

	FileMapping::FileMapping(FileMapping&& other) noexcept
	{
#ifdef SHINE_PLATFORM_WIN
		fileHandle = other.fileHandle;
		mappingHandle = other.mappingHandle;
		other.fileHandle = nullptr;
		other.mappingHandle = nullptr;
#elif SHINE_PLATFORM_WASM
		data = other.data;
		size = other.size;
		other.data = nullptr;
		other.size = 0;
#endif
	}

	FileMapping& FileMapping::operator=(FileMapping&& other) noexcept
	{
		if (this != &other)
		{
			clear();
#ifdef SHINE_PLATFORM_WIN
			fileHandle = other.fileHandle;
			mappingHandle = other.mappingHandle;
			other.fileHandle = nullptr;
			other.mappingHandle = nullptr;
#elif SHINE_PLATFORM_WASM
			data = other.data;
			size = other.size;
			other.data = nullptr;
			other.size = 0;
#endif
		}
		return *this;
	}

	bool FileMapping::IsValid() const noexcept
	{
#ifdef SHINE_PLATFORM_WIN
		return IsValidFileHandle() && IsValidMapHandle();
#elif SHINE_PLATFORM_WASM
		return data != nullptr && size > 0;
#else
		return false;
#endif
	}

	void FileMapping::clear()
	{
#ifdef SHINE_PLATFORM_WIN
		if (IsValidMapHandle())
		{
			CloseHandle(mappingHandle);
			mappingHandle = nullptr;
		}
		if (IsValidFileHandle())
		{
			CloseHandle(fileHandle);
			fileHandle = nullptr;
		}
#elif SHINE_PLATFORM_WASM
		if (data != nullptr)
		{
			free(data);
			data = nullptr;
			size = 0;
		}
#endif
	}

	// ============================================================================
	// FileMapView 实现
	// ============================================================================

	FileMapView::FileMapView(FileMapping&& _m, MappedView&& _v) noexcept
		: map(std::move(_m)), view(std::move(_v)) {}

	// ============================================================================
	// 基础文件操作实现
	// ============================================================================

	bool file_exists(SString name)
	{
#ifdef SHINE_PLATFORM_WIN
		std::string pathStr(name);
		DWORD attributes = GetFileAttributesA(pathStr.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
#elif SHINE_PLATFORM_WASM
		// WASM 需要通过 Emscripten FS API
		std::string pathStr(name);
		FILE* file = fopen(pathStr.c_str(), "rb");
		if (file)
		{
			fclose(file);
			return true;
		}
		return false;
#else
		std::string pathStr(name);
		struct stat buffer;
		return (stat(pathStr.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
#endif
	}

	bool directory_exists(SString name)
	{
#ifdef SHINE_PLATFORM_WIN
		std::string pathStr(name);
		DWORD attributes = GetFileAttributesA(pathStr.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
#elif SHINE_PLATFORM_WASM
		std::string pathStr(name);
		// WASM 目录检查
		FILE* file = fopen((pathStr + "/.").c_str(), "rb");
		if (file)
		{
			fclose(file);
			return true;
		}
		return false;
#else
		std::string pathStr(name);
		struct stat buffer;
		return (stat(pathStr.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
#endif
	}

#ifndef SHINE_PLATFORM_WASM
	std::expected<EFileFolderType, std::string> file_or_directory(SString name)
#else
	EFileFolderType file_or_directory(SString name, bool* success)
#endif
	{
		std::string pathStr(name);
#ifdef SHINE_PLATFORM_WIN
		DWORD attributes = GetFileAttributesA(pathStr.c_str());
		if (attributes == INVALID_FILE_ATTRIBUTES)
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("文件或目录不存在");
#else
			if (success) *success = false;
			return EFileFolderType::NONE;
#endif
		}
		if (attributes & FILE_ATTRIBUTE_DIRECTORY)
		{
#ifndef SHINE_PLATFORM_WASM
			return EFileFolderType::DIRECTORY;
#else
			if (success) *success = true;
			return EFileFolderType::DIRECTORY;
#endif
		}
#ifndef SHINE_PLATFORM_WASM
		return EFileFolderType::FILE;
#else
		if (success) *success = true;
		return EFileFolderType::FILE;
#endif
#elif SHINE_PLATFORM_WASM
		FILE* file = fopen(pathStr.c_str(), "rb");
		if (file)
		{
			fclose(file);
#ifndef SHINE_PLATFORM_WASM
			return EFileFolderType::FILE;
#else
			if (success) *success = true;
			return EFileFolderType::FILE;
#endif
		}
		file = fopen((pathStr + "/.").c_str(), "rb");
		if (file)
		{
			fclose(file);
#ifndef SHINE_PLATFORM_WASM
			return EFileFolderType::DIRECTORY;
#else
			if (success) *success = true;
			return EFileFolderType::DIRECTORY;
#endif
		}
#ifndef SHINE_PLATFORM_WASM
		return std::unexpected("文件或目录不存在");
#else
		if (success) *success = false;
		return EFileFolderType::NONE;
#endif
#else
		struct stat buffer;
		if (stat(pathStr.c_str(), &buffer) != 0)
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("文件或目录不存在");
#else
			if (success) *success = false;
			return EFileFolderType::NONE;
#endif
		}
		if (S_ISDIR(buffer.st_mode))
		{
#ifndef SHINE_PLATFORM_WASM
			return EFileFolderType::DIRECTORY;
#else
			if (success) *success = true;
			return EFileFolderType::DIRECTORY;
#endif
		}
#ifndef SHINE_PLATFORM_WASM
		return EFileFolderType::FILE;
#else
		if (success) *success = true;
		return EFileFolderType::FILE;
#endif
#endif
	}

#ifndef SHINE_PLATFORM_WASM
	std::expected<std::string, std::string> get_file_suffix(SString filename)
#else
	std::string get_file_suffix(SString filename, bool* success)
#endif
	{
		std::string filenameStr(filename);
		const auto idx = filenameStr.rfind('.');
		if (idx == std::string::npos || idx == 0 || idx == filenameStr.length() - 1)
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("文件没有扩展名");
#else
			if (success) *success = false;
			return "";
#endif
		}
#ifndef SHINE_PLATFORM_WASM
		return filenameStr.substr(idx);
#else
		if (success) *success = true;
		return filenameStr.substr(idx);
#endif
	}

	std::string get_file_extension(SString filename)
	{
		std::string filenameStr(filename);
		const auto idx = filenameStr.rfind('.');
		if (idx == std::string::npos || idx == 0 || idx == filenameStr.length() - 1)
		{
			return "";
		}
		// 返回不包含点号的扩展名
		return filenameStr.substr(idx + 1);
	}

	std::string get_file_name(SString filepath)
	{
		std::string pathStr(filepath);
#ifdef SHINE_PLATFORM_WIN
		size_t pos = pathStr.find_last_of("\\/");
#else
		size_t pos = pathStr.find_last_of('/');
#endif
		if (pos != std::string::npos)
		{
			return pathStr.substr(pos + 1);
		}
		return pathStr;
	}

	std::string get_file_directory(SString filepath)
	{
		std::string pathStr(filepath);
#ifdef SHINE_PLATFORM_WIN
		size_t pos = pathStr.find_last_of("\\/");
#else
		size_t pos = pathStr.find_last_of('/');
#endif
		if (pos != std::string::npos)
		{
			return pathStr.substr(0, pos);
		}
		return "";
	}

	std::string get_file_stem(SString filepath)
	{
		std::string name = get_file_name(filepath);
		size_t pos = name.rfind('.');
		if (pos != std::string::npos && pos > 0)
		{
			return name.substr(0, pos);
		}
		return name;
	}

	// ============================================================================
	// 文件映射操作实现
	// ============================================================================

#ifdef SHINE_PLATFORM_WIN
	// 获取系统内存分配粒度（Windows）
	static uint32_t allocationGranularity = []() {
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		return sysInfo.dwAllocationGranularity;
	}();
#endif

#ifndef SHINE_PLATFORM_WASM
	std::expected<FileMapping, std::string> open_file_from_mapping(SString filename)
#else
	FileMapping open_file_from_mapping(SString filename, bool* success)
#endif
	{
		std::string filenameStr(filename);
#ifdef SHINE_PLATFORM_WIN
		HANDLE hFile = CreateFileA(
			filenameStr.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			nullptr);

		if (hFile == INVALID_HANDLE_VALUE)
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected(fmt::format("打开文件失败: {}", filenameStr));
#else
			if (success) *success = false;
			return FileMapping();
#endif
		}

		HANDLE hMapping = CreateFileMappingA(
			hFile,
			nullptr,
			PAGE_READONLY,
			0,
			0,
			nullptr);

		if (hMapping == nullptr || hMapping == INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile);
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected(fmt::format("创建文件映射失败: {}", filenameStr));
#else
			if (success) *success = false;
			return FileMapping();
#endif
		}

#ifndef SHINE_PLATFORM_WASM
		return FileMapping{ hFile, hMapping };
#else
		if (success) *success = true;
		return FileMapping{ hFile, hMapping };
#endif

#elif SHINE_PLATFORM_WASM
		// WASM 实现：读取整个文件到内存
		FILE* file = fopen(filenameStr.c_str(), "rb");
		if (!file)
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected(fmt::format("打开文件失败: {}", filenameStr));
#else
			if (success) *success = false;
			return FileMapping();
#endif
		}

		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		void* data = malloc(fileSize);
		if (!data)
		{
			fclose(file);
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("内存分配失败");
#else
			if (success) *success = false;
			return FileMapping();
#endif
		}

		size_t readSize = fread(data, 1, fileSize, file);
		fclose(file);

		if (readSize != static_cast<size_t>(fileSize))
		{
			free(data);
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("读取文件失败");
#else
			if (success) *success = false;
			return FileMapping();
#endif
		}

#ifndef SHINE_PLATFORM_WASM
		return FileMapping{ data, static_cast<size_t>(fileSize) };
#else
		if (success) *success = true;
		return FileMapping{ data, static_cast<size_t>(fileSize) };
#endif
#else
		// Linux/Unix 实现（使用 mmap）
		// 这里简化处理，实际应该使用 mmap
#ifndef SHINE_PLATFORM_WASM
		return std::unexpected("平台不支持文件映射");
#else
		if (success) *success = false;
		return FileMapping();
#endif
#endif
	}

#ifndef SHINE_PLATFORM_WASM
	std::expected<MappedView, std::string> read_data_from_mapping(FileMapping& mapping, uint64_t size, uint64_t offset)
#else
	MappedView read_data_from_mapping(FileMapping& mapping, uint64_t size, uint64_t offset, bool* success)
#endif
	{
#ifdef SHINE_PLATFORM_WIN
		if (!mapping.IsValidMapHandle())
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("文件映射句柄无效");
#else
			if (success) *success = false;
			return MappedView();
#endif
		}

		uint64_t mapOffset = (offset / allocationGranularity) * allocationGranularity;
		uint64_t readOffset = offset - mapOffset;

		unsigned long offsetHigh = static_cast<unsigned long>((mapOffset >> 32) & 0xFFFFFFFF);
		unsigned long offsetLow = static_cast<unsigned long>(mapOffset & 0xFFFFFFFF);

		void* pFile = MapViewOfFile(
			mapping.mappingHandle,
			FILE_MAP_READ,
			offsetHigh,
			offsetLow,
			size + readOffset);

		if (pFile == nullptr)
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("映射文件到进程地址空间失败");
#else
			if (success) *success = false;
			return MappedView();
#endif
		}

		const std::byte* pData = static_cast<const std::byte*>(pFile) + readOffset;
#ifndef SHINE_PLATFORM_WASM
		return MappedView(pFile, pData, size);
#else
		if (success) *success = true;
		return MappedView(pFile, pData, size);
#endif

#elif SHINE_PLATFORM_WASM
		if (!mapping.IsValid())
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("文件映射无效");
#else
			if (success) *success = false;
			return MappedView();
#endif
		}

		if (offset + size > mapping.size)
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("读取范围超出文件大小");
#else
			if (success) *success = false;
			return MappedView();
#endif
		}

		const std::byte* pData = static_cast<const std::byte*>(mapping.data) + offset;
#ifndef SHINE_PLATFORM_WASM
		return MappedView(mapping.data, pData, size);
#else
		if (success) *success = true;
		return MappedView(mapping.data, pData, size);
#endif
#else
#ifndef SHINE_PLATFORM_WASM
		return std::unexpected("平台不支持文件映射");
#else
		if (success) *success = false;
		return MappedView();
#endif
#endif
	}

#ifndef SHINE_PLATFORM_WASM
	std::expected<uint64_t, std::string> get_file_size(FileMapping& mapping)
#else
	uint64_t get_file_size(FileMapping& mapping, bool* success)
#endif
	{
#ifdef SHINE_PLATFORM_WIN
		LARGE_INTEGER fileSize{};
		if (!mapping.IsValidFileHandle() || !GetFileSizeEx(mapping.fileHandle, &fileSize))
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("获取文件大小失败");
#else
			if (success) *success = false;
			return 0;
#endif
		}
#ifndef SHINE_PLATFORM_WASM
		return fileSize.QuadPart;
#else
		if (success) *success = true;
		return fileSize.QuadPart;
#endif
#elif SHINE_PLATFORM_WASM
		if (!mapping.IsValid())
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("文件映射无效");
#else
			if (success) *success = false;
			return 0;
#endif
		}
#ifndef SHINE_PLATFORM_WASM
		return mapping.size;
#else
		if (success) *success = true;
		return mapping.size;
#endif
#else
#ifndef SHINE_PLATFORM_WASM
		return std::unexpected("平台不支持");
#else
		if (success) *success = false;
		return 0;
#endif
#endif
	}

#ifndef SHINE_PLATFORM_WASM
	std::expected<FileMapView, std::string> read_full_file(SString filePath)
#else
	FileMapView read_full_file(SString filePath, bool* success)
#endif
	{
		std::string filePathStr(filePath);
		auto openResult = open_file_from_mapping(filePathStr);
#ifndef SHINE_PLATFORM_WASM
		if (!openResult.has_value())
		{
			return std::unexpected(openResult.error());
		}

		auto file_mapping = std::move(openResult.value());
		auto file_size = get_file_size(file_mapping);

		if (!file_size.has_value())
		{
			return std::unexpected(file_size.error());
		}

		auto file_data = read_data_from_mapping(file_mapping, file_size.value(), 0);
		if (!file_data.has_value())
		{
			return std::unexpected(file_data.error());
		}

		auto file_data_view = std::move(file_data.value());
		return FileMapView{ std::move(file_mapping), std::move(file_data_view) };
#else
		bool tempSuccess = false;
		auto file_mapping = open_file_from_mapping(filePathStr, &tempSuccess);
		if (!tempSuccess)
		{
			if (success) *success = false;
			return FileMapView();
		}

		uint64_t file_size = get_file_size(file_mapping, &tempSuccess);
		if (!tempSuccess)
		{
			if (success) *success = false;
			return FileMapView();
		}

		auto file_data_view = read_data_from_mapping(file_mapping, file_size, 0, &tempSuccess);
		if (!tempSuccess)
		{
			if (success) *success = false;
			return FileMapView();
		}

		if (success) *success = true;
		return FileMapView{ std::move(file_mapping), std::move(file_data_view) };
#endif
	}

	// ============================================================================
	// 文件读写操作实现
	// ============================================================================

#ifndef SHINE_PLATFORM_WASM
	std::expected<std::vector<std::byte>, std::string> read_file_bytes(SString filePath)
#else
	std::vector<std::byte> read_file_bytes(SString filePath, bool* success)
#endif
	{
		std::string filePathStr(filePath);
		std::ifstream file(filePathStr, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected(fmt::format("无法打开文件: {}", filePathStr));
#else
			if (success) *success = false;
			return {};
#endif
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<std::byte> buffer(size);
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("读取文件失败");
#else
			if (success) *success = false;
			return {};
#endif
		}

#ifndef SHINE_PLATFORM_WASM
		return buffer;
#else
		if (success) *success = true;
		return buffer;
#endif
	}

#ifndef SHINE_PLATFORM_WASM
	std::expected<std::string, std::string> read_file_text(SString filePath)
#else
	std::string read_file_text(SString filePath, bool* success)
#endif
	{
		std::string filePathStr(filePath);
		std::ifstream file(filePathStr);
		if (!file.is_open())
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected(fmt::format("无法打开文件: {}", filePathStr));
#else
			if (success) *success = false;
			return "";
#endif
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
#ifndef SHINE_PLATFORM_WASM
		return buffer.str();
#else
		if (success) *success = true;
		return buffer.str();
#endif
	}

#ifndef SHINE_PLATFORM_WASM
	bool SaveData(SString path, std::span<const std::byte> data)
	{
		return SaveData(path, data.data(), data.size());
	}
#endif

	bool SaveData(SString path, const void* data, size_t size)
	{
		std::string pathStr(path);
#ifdef SHINE_PLATFORM_WIN
		// 使用 Windows API 提升性能
		HANDLE hFile = CreateFileA(
			pathStr.c_str(),
			GENERIC_WRITE,
			0,
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			nullptr);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		DWORD bytesWritten = 0;
		BOOL result = WriteFile(hFile, data, static_cast<DWORD>(size), &bytesWritten, nullptr);
		CloseHandle(hFile);

		return result && bytesWritten == size;
#else
		FILE* file = fopen(pathStr.c_str(), "wb");
		if (!file)
		{
			return false;
		}

		size_t written = fwrite(data, 1, size, file);
		fclose(file);
		return written == size;
#endif
	}

	bool SaveText(SString path, SString text)
	{
		std::string pathStr(path);
		std::string textStr(text);
#ifdef SHINE_PLATFORM_WIN
		HANDLE hFile = CreateFileA(
			pathStr.c_str(),
			GENERIC_WRITE,
			0,
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		DWORD bytesWritten = 0;
		BOOL result = WriteFile(hFile, textStr.c_str(), static_cast<DWORD>(textStr.size()), &bytesWritten, nullptr);
		CloseHandle(hFile);

		return result && bytesWritten == textStr.size();
#else
		std::ofstream file(pathStr);
		if (!file.is_open())
		{
			return false;
		}
		file << textStr;
		return file.good();
#endif
	}

	bool AppendText(SString path, SString text)
	{
		std::string pathStr(path);
		std::string textStr(text);
#ifdef SHINE_PLATFORM_WIN
		HANDLE hFile = CreateFileA(
			pathStr.c_str(),
			FILE_APPEND_DATA,
			0,
			nullptr,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		DWORD bytesWritten = 0;
		BOOL result = WriteFile(hFile, textStr.c_str(), static_cast<DWORD>(textStr.size()), &bytesWritten, nullptr);
		CloseHandle(hFile);

		return result && bytesWritten == textStr.size();
#else
		std::ofstream file(pathStr, std::ios::app);
		if (!file.is_open())
		{
			return false;
		}
		file << textStr;
		return file.good();
#endif
	}

	// ============================================================================
	// 文件管理操作实现
	// ============================================================================

	bool DeleteFile(SString path)
	{
		std::string pathStr(path);
#ifdef SHINE_PLATFORM_WIN
		return ::DeleteFileA(pathStr.c_str()) != 0;
#else
		return std::remove(pathStr.c_str()) == 0;
#endif
	}

	bool CopyFile(SString sourcePath, SString destPath, bool overwrite)
	{
		std::string sourceStr(sourcePath);
		std::string destStr(destPath);
#ifdef SHINE_PLATFORM_WIN
		return ::CopyFileA(sourceStr.c_str(), destStr.c_str(), !overwrite) != 0;
#else
		// 读取源文件并写入目标文件
		auto data = read_file_bytes(sourceStr);
#ifndef SHINE_PLATFORM_WASM
		if (!data.has_value())
		{
			return false;
		}
		return SaveData(destStr, data->data(), data->size());
#else
		bool success = false;
		auto bytes = read_file_bytes(sourceStr, &success);
		if (!success)
		{
			return false;
		}
		return SaveData(destStr, bytes.data(), bytes.size());
#endif
#endif
	}

	bool MoveFile(SString sourcePath, SString destPath)
	{
		std::string sourceStr(sourcePath);
		std::string destStr(destPath);
#ifdef SHINE_PLATFORM_WIN
		return ::MoveFileA(sourceStr.c_str(), destStr.c_str()) != 0;
#else
		return std::rename(sourceStr.c_str(), destStr.c_str()) == 0;
#endif
	}

	uint64_t GetFileSize(SString path)
	{
		std::string pathStr(path);
#ifdef SHINE_PLATFORM_WIN
		HANDLE hFile = CreateFileA(
			pathStr.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return 0;
		}

		LARGE_INTEGER fileSize{};
		if (!GetFileSizeEx(hFile, &fileSize))
		{
			CloseHandle(hFile);
			return 0;
		}

		CloseHandle(hFile);
		return fileSize.QuadPart;
#elif SHINE_PLATFORM_WASM
		FILE* file = fopen(pathStr.c_str(), "rb");
		if (!file)
		{
			return 0;
		}

		fseek(file, 0, SEEK_END);
		long size = ftell(file);
		fclose(file);
		return size >= 0 ? static_cast<uint64_t>(size) : 0;
#else
		struct stat buffer;
		if (stat(pathStr.c_str(), &buffer) != 0)
		{
			return 0;
		}
		return static_cast<uint64_t>(buffer.st_size);
#endif
	}

	uint64_t GetFileLastModified(SString path)
	{
		std::string pathStr(path);
#ifdef SHINE_PLATFORM_WIN
		HANDLE hFile = CreateFileA(
			pathStr.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return 0;
		}

		FILETIME writeTime;
		if (!GetFileTime(hFile, nullptr, nullptr, &writeTime))
		{
			CloseHandle(hFile);
			return 0;
		}

		CloseHandle(hFile);

		// 转换为 Unix 时间戳
		ULARGE_INTEGER ul;
		ul.LowPart = writeTime.dwLowDateTime;
		ul.HighPart = writeTime.dwHighDateTime;
		// Windows FILETIME 是从 1601-01-01 开始的 100 纳秒间隔
		// Unix 时间戳是从 1970-01-01 开始的秒数
		uint64_t unixTime = (ul.QuadPart / 10000000ULL) - 11644473600ULL;
		return unixTime;
#elif SHINE_PLATFORM_WASM
		// WASM 可能无法获取文件时间
		return 0;
#else
		struct stat buffer;
		if (stat(pathStr.c_str(), &buffer) != 0)
		{
			return 0;
		}
		return static_cast<uint64_t>(buffer.st_mtime);
#endif
	}

	// ============================================================================
	// 目录操作实现
	// ============================================================================

	bool CreateDir(SString path)
	{
		std::string pathStr(path);
#ifdef SHINE_PLATFORM_WIN
		return CreateDirectoryA(pathStr.c_str(), nullptr) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
#else
		int result = mkdir(pathStr.c_str(), 0755);
		return result == 0 || errno == EEXIST;
#endif
	}

	bool CreateDirRecursive(SString path)
	{
		std::string pathStr = NormalizePath(path);
		std::string currentPath;

#ifdef SHINE_PLATFORM_WIN
		size_t start = 0;
		if (pathStr.length() >= 2 && pathStr[1] == ':')
		{
			// Windows 绝对路径，包含驱动器号
			currentPath = pathStr.substr(0, 3);
			start = 3;
		}
		else if (pathStr.length() >= 1 && (pathStr[0] == '\\' || pathStr[0] == '/'))
		{
			// UNC 路径或根路径
			currentPath = pathStr.substr(0, 1);
			start = 1;
		}

		for (size_t i = start; i < pathStr.length(); ++i)
		{
			if (pathStr[i] == '\\' || pathStr[i] == '/')
			{
				if (!currentPath.empty() && !CreateDir(currentPath))
				{
					if (GetLastError() != ERROR_ALREADY_EXISTS)
					{
						return false;
					}
				}
			}
			currentPath += pathStr[i];
		}

		if (!currentPath.empty())
		{
			return CreateDir(currentPath) || GetLastError() == ERROR_ALREADY_EXISTS;
		}
		return true;
#else
		size_t start = 0;
		if (pathStr[0] == '/')
		{
			currentPath = "/";
			start = 1;
		}

		for (size_t i = start; i < pathStr.length(); ++i)
		{
			if (pathStr[i] == '/')
			{
				if (!currentPath.empty() && currentPath != "/")
				{
					int result = mkdir(currentPath.c_str(), 0755);
					if (result != 0 && errno != EEXIST)
					{
						return false;
					}
				}
			}
			currentPath += pathStr[i];
		}

		if (!currentPath.empty() && currentPath != "/")
		{
			int result = mkdir(currentPath.c_str(), 0755);
			return result == 0 || errno == EEXIST;
		}
		return true;
#endif
	}

	bool DeleteDir(SString path)
	{
		std::string pathStr(path);
#ifdef SHINE_PLATFORM_WIN
		return RemoveDirectoryA(pathStr.c_str()) != 0;
#else
		return rmdir(pathStr.c_str()) == 0;
#endif
	}

	bool DeleteDirRecursive(SString path)
	{
		std::string pathStr(path);
#ifdef SHINE_PLATFORM_WIN
		WIN32_FIND_DATAA findData;
		std::string searchPath = pathStr + "\\*";
		HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

		if (hFind == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		do
		{
			if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
			{
				continue;
			}

			std::string fullPath = pathStr + "\\" + findData.cFileName;

			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!DeleteDirRecursive(fullPath))
				{
					FindClose(hFind);
					return false;
				}
			}
			else
			{
				if (!::DeleteFileA(fullPath.c_str()))
				{
					FindClose(hFind);
					return false;
				}
			}
		} while (FindNextFileA(hFind, &findData));

		FindClose(hFind);
		return RemoveDirectoryA(pathStr.c_str()) != 0;
#else
		DIR* dir = opendir(pathStr.c_str());
		if (!dir)
		{
			return false;
		}

		struct dirent* entry;
		while ((entry = readdir(dir)) != nullptr)
		{
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			{
				continue;
			}

			std::string fullPath = pathStr + "/" + entry->d_name;
			struct stat statBuf;
			if (stat(fullPath.c_str(), &statBuf) == 0)
			{
				if (S_ISDIR(statBuf.st_mode))
				{
					if (!DeleteDirRecursive(fullPath))
					{
						closedir(dir);
						return false;
					}
				}
				else
				{
					if (std::remove(fullPath.c_str()) != 0)
					{
						closedir(dir);
						return false;
					}
				}
			}
		}

		closedir(dir);
		return rmdir(pathStr.c_str()) == 0;
#endif
	}

#ifndef SHINE_PLATFORM_WASM
	std::expected<std::vector<FileInfo>, std::string> ListDirectory(SString dirPath, bool includeSubdirs)
#else
	std::vector<FileInfo> ListDirectory(SString dirPath, bool includeSubdirs, bool* success)
#endif
	{
		std::vector<FileInfo> result;
		std::string dirPathStr(dirPath);

#ifdef SHINE_PLATFORM_WIN
		WIN32_FIND_DATAA findData;
		std::string searchPath = dirPathStr + "\\*";
		HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

		if (hFind == INVALID_HANDLE_VALUE)
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("无法打开目录");
#else
			if (success) *success = false;
			return result;
#endif
		}

		do
		{
			if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
			{
				continue;
			}

			FileInfo info;
			info.name = findData.cFileName;
			info.path = dirPathStr + "\\" + findData.cFileName;
			info.type = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? EFileFolderType::DIRECTORY : EFileFolderType::FILE;
			info.size = (info.type == EFileFolderType::FILE) ? 
				(static_cast<uint64_t>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow : 0;

			ULARGE_INTEGER ul;
			ul.LowPart = findData.ftLastWriteTime.dwLowDateTime;
			ul.HighPart = findData.ftLastWriteTime.dwHighDateTime;
			info.lastModified = (ul.QuadPart / 10000000ULL) - 11644473600ULL;

			result.push_back(info);

			if (includeSubdirs && info.type == EFileFolderType::DIRECTORY)
			{
				auto subdir = ListDirectory(info.path, true);
#ifndef SHINE_PLATFORM_WASM
				if (subdir.has_value())
				{
					result.insert(result.end(), subdir->begin(), subdir->end());
				}
#else
				bool subSuccess = false;
				auto subdirList = ListDirectory(info.path, true, &subSuccess);
				if (subSuccess)
				{
					result.insert(result.end(), subdirList.begin(), subdirList.end());
				}
#endif
			}
		} while (FindNextFileA(hFind, &findData));

		FindClose(hFind);
#else
		DIR* dir = opendir(dirPathStr.c_str());
		if (!dir)
		{
#ifndef SHINE_PLATFORM_WASM
			return std::unexpected("无法打开目录");
#else
			if (success) *success = false;
			return result;
#endif
		}

		struct dirent* entry;
		while ((entry = readdir(dir)) != nullptr)
		{
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			{
				continue;
			}

			std::string fullPath = dirPathStr + "/" + entry->d_name;
			struct stat statBuf;
			if (stat(fullPath.c_str(), &statBuf) != 0)
			{
				continue;
			}

			FileInfo info;
			info.name = entry->d_name;
			info.path = fullPath;
			info.type = S_ISDIR(statBuf.st_mode) ? EFileFolderType::DIRECTORY : EFileFolderType::FILE;
			info.size = (info.type == EFileFolderType::FILE) ? static_cast<uint64_t>(statBuf.st_size) : 0;
			info.lastModified = static_cast<uint64_t>(statBuf.st_mtime);

			result.push_back(info);

			if (includeSubdirs && info.type == EFileFolderType::DIRECTORY)
			{
				auto subdir = ListDirectory(info.path, true);
#ifndef SHINE_PLATFORM_WASM
				if (subdir.has_value())
				{
					result.insert(result.end(), subdir->begin(), subdir->end());
				}
#else
				bool subSuccess = false;
				auto subdirList = ListDirectory(info.path, true, &subSuccess);
				if (subSuccess)
				{
					result.insert(result.end(), subdirList.begin(), subdirList.end());
				}
#endif
			}
		}

		closedir(dir);
#endif

#ifndef SHINE_PLATFORM_WASM
		return result;
#else
		if (success) *success = true;
		return result;
#endif
	}

	std::string GetCurrentDirectory()
	{
#ifdef SHINE_PLATFORM_WIN
		char buffer[MAX_PATH];
		DWORD length = (::GetCurrentDirectoryA)(MAX_PATH, buffer);
		if (length == 0)
		{
			return "";
		}
		return std::string(buffer, length);
#else
		char* cwd = getcwd(nullptr, 0);
		if (!cwd)
		{
			return "";
		}
		std::string result(cwd);
		free(cwd);
		return result;
#endif
	}

	bool SetCurrentDirectory(SString path)
	{
		std::string pathStr(path);
#ifdef SHINE_PLATFORM_WIN
		return ::SetCurrentDirectoryA(pathStr.c_str()) != 0;
#else
		return chdir(pathStr.c_str()) == 0;
#endif
	}

	// ============================================================================
	// 路径操作实现
	// ============================================================================

	std::string NormalizePath(SString path)
	{
		std::string pathStr(path);
#ifdef SHINE_PLATFORM_WIN
		// 统一使用反斜杠
		std::replace(pathStr.begin(), pathStr.end(), '/', '\\');
		// 移除重复的分隔符
		std::string result;
		bool lastWasSep = false;
		for (char c : pathStr)
		{
			if (c == '\\')
			{
				if (!lastWasSep)
				{
					result += c;
					lastWasSep = true;
				}
			}
			else
			{
				result += c;
				lastWasSep = false;
			}
		}
		return result;
#else
		// 统一使用正斜杠
		std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
		// 移除重复的分隔符
		std::string result;
		bool lastWasSep = false;
		for (char c : pathStr)
		{
			if (c == '/')
			{
				if (!lastWasSep)
				{
					result += c;
					lastWasSep = true;
				}
			}
			else
			{
				result += c;
				lastWasSep = false;
			}
		}
		return result;
#endif
	}

	std::string JoinPath(SString base, SString part)
	{
		std::string baseStr(base);
		std::string partStr(part);

		if (baseStr.empty())
		{
			return partStr;
		}

		if (partStr.empty())
		{
			return baseStr;
		}

#ifdef SHINE_PLATFORM_WIN
		char lastChar = baseStr.back();
		if (lastChar != '\\' && lastChar != '/')
		{
			return baseStr + "\\" + partStr;
		}
		return baseStr + partStr;
#else
		char lastChar = baseStr.back();
		if (lastChar != '/')
		{
			return baseStr + "/" + partStr;
		}
		return baseStr + partStr;
#endif
	}

	bool IsAbsolutePath(SString path)
	{
		std::string pathStr(path);
#ifdef SHINE_PLATFORM_WIN
		// Windows: C:\ 或 \\server\share 格式
		return (pathStr.length() >= 3 && pathStr[1] == ':' && (pathStr[2] == '\\' || pathStr[2] == '/')) ||
			(pathStr.length() >= 2 && pathStr[0] == '\\' && pathStr[1] == '\\');
#else
		// Unix: 以 / 开头
		return !pathStr.empty() && pathStr[0] == '/';
#endif
	}

	std::string GetAbsolutePath(SString path)
	{
		std::string pathStr(path);
		if (IsAbsolutePath(pathStr))
		{
			return NormalizePath(pathStr);
		}

		std::string currentDir = GetCurrentDirectory();
		return NormalizePath(JoinPath(currentDir, pathStr));
	}
}
