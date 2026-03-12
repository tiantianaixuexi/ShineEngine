#include "file_util.ixx"

#ifdef SHINE_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <fileapi.h>


#else
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#endif

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "util/encoding_util.ixx"

#include "fmt/format.h"
#include "path_util.h"
#include "shine_define.h"

namespace shine::util {
// ============================================================================
// MappedView 实现
// ============================================================================

MappedView::MappedView(MappedView &&other) noexcept
    : baseAddress(other.baseAddress), content(other.content) {
    other.baseAddress = nullptr;
    other.content     = {};
}

MappedView &MappedView::operator=(MappedView &&other) noexcept {
    if (this != &other) {
        clear();
        baseAddress       = other.baseAddress;
        content           = other.content;
        other.content     = {};
        other.baseAddress = nullptr;
    }
    return *this;
}

void MappedView::clear() {
    if (baseAddress) {
        UnmapViewOfFile(baseAddress);
        baseAddress = nullptr;
        content     = {};
    }
}

const std::byte *MappedView::data() const noexcept {
    return content.data();
}

const std::byte &MappedView::operator[](size_t index) const noexcept {
    return content[index];
}

size_t MappedView::size() const noexcept {
    return content.size();
}

bool MappedView::empty() const noexcept {
    return content.empty();
}

// ============================================================================
// FileMapping 实现
// ============================================================================

FileMapping::FileMapping() noexcept = default;

FileMapping::FileMapping(void *fileHdl, void *mappingHdl) noexcept
    : fileHandle(fileHdl), mappingHandle(mappingHdl) {}

bool FileMapping::IsValidFileHandle() const noexcept {
    return fileHandle != nullptr && fileHandle != INVALID_HANDLE_VALUE;
}

bool FileMapping::IsValidMapHandle() const noexcept {
    return mappingHandle != nullptr && mappingHandle != INVALID_HANDLE_VALUE;
}

FileMapping::FileMapping(FileMapping &&other) noexcept : fileHandle(other.fileHandle), mappingHandle(other.mappingHandle) {

    other.fileHandle    = nullptr;
    other.mappingHandle = nullptr;
}

FileMapping &FileMapping::operator=(FileMapping &&other) noexcept {
    if (this != &other) {
        clear();
        fileHandle          = other.fileHandle;
        mappingHandle       = other.mappingHandle;
        other.fileHandle    = nullptr;
        other.mappingHandle = nullptr;
    }
    return *this;
}

bool FileMapping::IsValid() const noexcept {
    return IsValidFileHandle() && IsValidMapHandle();
}

void FileMapping::clear() {
    if (IsValidMapHandle()) {
        CloseHandle(mappingHandle);
        mappingHandle = nullptr;
    }
    if (IsValidFileHandle()) {
        CloseHandle(fileHandle);
        fileHandle = nullptr;
    }
}

// ============================================================================
// FileMapView 实现
// ============================================================================

FileMapView::FileMapView(FileMapping &&_m, MappedView &&_v) noexcept
    : map(std::move(_m)), view(std::move(_v)) {}

// ============================================================================
// 基础文件操作实现
// ============================================================================

bool file_exists(STextView name) {
    const auto _name = EncodingUtil::UTF8ToWstring(name.sv());
    DWORD       attributes = GetFileAttributesW(_name.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool directory_exists(STextView name) {
    const auto _name = EncodingUtil::UTF8ToWstring(name.sv());
    DWORD       attributes = GetFileAttributesW(_name.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

std::expected<EFileFolderType, SString> file_or_directory(STextView name) {

    const auto _name = EncodingUtil::UTF8ToWstring(name.sv());
    DWORD attributes = GetFileAttributesW(_name.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return std::unexpected("文件或目录不存在");
    }

    if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
        return EFileFolderType::DIRECTORY;
    }

    return EFileFolderType::FILE;
}

// ============================================================================
// 文件映射操作实现
// ============================================================================

// 获取系统内存分配粒度（Windows）
static const uint32_t allocationGranularity = []() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwAllocationGranularity;
}();

std::expected<FileMapping, SString> open_file_from_mapping(STextView filename) {

	auto _file = EncodingUtil::UTF8ToWstring(filename.sv());


    HANDLE hFile = CreateFileW(
        _file.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return std::unexpected(fmt::format("打开文件失败: {}", filename.sv()).c_str());
    }

    HANDLE hMapping = CreateFileMappingW(
        hFile,
        nullptr,
        PAGE_READONLY,
        0,
        0,
        nullptr);

    if (hMapping == nullptr || hMapping == INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        return std::unexpected(fmt::format("创建文件映射失败: {}", filename.sv()).c_str());
    }

    return FileMapping{hFile, hMapping};
}

std::expected<MappedView, SString> read_data_from_mapping(FileMapping &mapping, uint64_t size, uint64_t offset) {

    if (!mapping.IsValidMapHandle()) {
        return std::unexpected("文件映射句柄无效");
    }

    const uint64_t mapOffset  = (offset / allocationGranularity) * allocationGranularity;
    const uint64_t readOffset = offset - mapOffset;

    const auto offsetHigh = static_cast<unsigned long>((mapOffset >> 32) & 0xFFFFFFFF);
    const auto offsetLow  = static_cast<unsigned long>(mapOffset & 0xFFFFFFFF);

    void *pFile = MapViewOfFile(
        mapping.mappingHandle,
        FILE_MAP_READ,
        offsetHigh,
        offsetLow,
        size + readOffset);

    if (pFile == nullptr) {
        return std::unexpected("映射文件到进程地址空间失败");
    }

    const std::byte *pData = static_cast<const std::byte *>(pFile) + readOffset;
    return MappedView(pFile, pData, size);
}

std::expected<u64, SString> get_file_size(FileMapping &mapping) {
    LARGE_INTEGER fileSize{};
    if (!mapping.IsValidFileHandle() || !GetFileSizeEx(mapping.fileHandle, &fileSize)) {
        return std::unexpected("获取文件大小失败");
    }
    return fileSize.QuadPart;
}

std::expected<FileMapView, SString> read_full_file(STextView filePath) {
    std::string filePathStr = filePath.to_string();

    // 使用 STextView 调用 open_file_from_mapping
    // 注意 open_file_from_mapping 已经改为接受 STextView
    auto openResult = open_file_from_mapping(filePath);
    if (!openResult.has_value()) {
        return std::unexpected(openResult.error());
    }

    auto file_mapping = std::move(openResult.value());
    auto file_size    = get_file_size(file_mapping);

    if (!file_size.has_value()) {
        return std::unexpected(file_size.error());
    }

    auto file_data = read_data_from_mapping(file_mapping, file_size.value(), 0);
    if (!file_data.has_value()) {
        return std::unexpected(file_data.error());
    }

    auto file_data_view = std::move(file_data.value());
    return FileMapView{std::move(file_mapping), std::move(file_data_view)};
}

// ============================================================================
// 文件读写操作实现
// ============================================================================

std::expected<std::vector<std::byte>, SString> read_file_bytes(STextView filePath) {
    const std::string &filePathStr = filePath.to_string();
    std::ifstream     file(filePathStr, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return std::unexpected(fmt::format("无法打开文件: {}", filePathStr).c_str());
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
        return std::unexpected("读取文件失败");
    }

    return buffer;
}

std::expected<std::string, SString> read_file_text(STextView filePath) {
    const std::string &filePathStr = filePath.to_string();
    std::ifstream      file(filePathStr);
    if (!file.is_open()) {
        return std::unexpected(fmt::format("无法打开文件: {}", filePathStr).c_str());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool SaveData(STextView path, std::span<const std::byte> data) {
    const void  *Savedata = data.data();
    const size_t Savesize = data.size();

	auto _wpath = EncodingUtil::UTF8ToWstring(path.sv());

    // 使用 Windows API 提升性能
    HANDLE hFile = CreateFileW(
        _wpath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesWritten = 0;
    BOOL  result       = WriteFile(hFile, Savedata, static_cast<DWORD>(Savesize), &bytesWritten, nullptr);
    CloseHandle(hFile);

    return result && bytesWritten == Savesize;
}

bool SaveText(STextView path, STextView text) {
    const auto wpath = EncodingUtil::UTF8ToWstring(path.sv());
    const std::string &textStr = text.to_string();

    HANDLE hFile = CreateFileW(
        wpath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesWritten = 0;
    BOOL  result       = WriteFile(hFile, textStr.c_str(), static_cast<DWORD>(textStr.size()), &bytesWritten, nullptr);
    CloseHandle(hFile);

    return result && bytesWritten == textStr.size();
}

bool AppendText(STextView path, STextView text) {
    const auto wpath = EncodingUtil::UTF8ToWstring(path.sv());
    const std::string &textStr = text.to_string();

    HANDLE hFile = CreateFileW(
        wpath.c_str(),
        FILE_APPEND_DATA,
        0,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesWritten = 0;
    BOOL  result       = WriteFile(hFile, textStr.c_str(), static_cast<DWORD>(textStr.size()), &bytesWritten, nullptr);
    CloseHandle(hFile);

    return result && bytesWritten == textStr.size();
}

// ============================================================================
// 文件管理操作实现
// ============================================================================

bool DeleteFile(STextView path) {
	const auto wpath = EncodingUtil::UTF8ToWstring(path.sv());
    return ::DeleteFileW(wpath.data()) != 0;
}

bool CopyFile(STextView sourcePath, STextView destPath, bool overwrite) {
	const auto wsourcePath = EncodingUtil::UTF8ToWstring(sourcePath.sv());
	const auto wdestPath = EncodingUtil::UTF8ToWstring(destPath.sv());
    return ::CopyFileW(wsourcePath.data(), wdestPath.data(), !overwrite) != 0;
}

bool MoveFile(STextView sourcePath, STextView destPath) {
	const auto wsourcePath = EncodingUtil::UTF8ToWstring(sourcePath.sv());
	const auto wdestPath = EncodingUtil::UTF8ToWstring(destPath.sv());
    return ::MoveFileW(wsourcePath.data(), wdestPath.data()) != 0;
}

uint64_t GetFileSize(STextView path) {

	const auto wPath = EncodingUtil::UTF8ToWstring(path.sv());

    HANDLE hFile = CreateFileW(
        wPath.data(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }

    LARGE_INTEGER fileSize{};
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return 0;
    }

    CloseHandle(hFile);
    return fileSize.QuadPart;
}

uint64_t GetFileLastModified(STextView path) {
	const auto wPath = EncodingUtil::UTF8ToWstring(path.sv());
    HANDLE hFile = CreateFileW(
        wPath.data(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }

    FILETIME writeTime;
    if (!GetFileTime(hFile, nullptr, nullptr, &writeTime)) {
        CloseHandle(hFile);
        return 0;
    }

    CloseHandle(hFile);

    // 转换为 Unix 时间戳
    ULARGE_INTEGER ul;
    ul.LowPart  = writeTime.dwLowDateTime;
    ul.HighPart = writeTime.dwHighDateTime;
    // Windows FILETIME 是从 1601-01-01 开始的 100 纳秒间隔
    // Unix 时间戳是从 1970-01-01 开始的秒数
    uint64_t unixTime = (ul.QuadPart / 10000000ULL) - 11644473600ULL;
    return unixTime;
}

// ============================================================================
// 目录操作实现
// ============================================================================

bool CreateDir(STextView path) {
	const auto wPath = EncodingUtil::UTF8ToWstring(path);
    return CreateDirectoryW(wPath.data(), nullptr) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool CreateDirRecursive(STextView path) {
    const std::string &pathStr = normalize_path(SString::from_view(path)).to_string();
    std::string        currentPath{};

    size_t start = 0;
    if (pathStr.length() >= 2 && pathStr[1] == ':') {
        // Windows 绝对路径，包含驱动器号
        currentPath = pathStr.substr(0, 3);
        start       = 3;
    } else if (pathStr.length() >= 1 && (pathStr[0] == '\\' || pathStr[0] == '/')) {
        // UNC 路径或根路径
        currentPath = pathStr.substr(0, 1);
        start       = 1;
    }

    for (size_t i = start; i < pathStr.length(); ++i) {
        if (pathStr[i] == '\\' || pathStr[i] == '/') {
            // 这里 currentPath 是 std::string，CreateDir 需要 STextView
            // 但 CreateDir 内部又转回 std::string
            // 我们可以直接调用 Win32 API 避免往返转换
            if (!currentPath.empty()) {
                // 使用底层 API 或 SString::from_utf8
				const auto wpath = EncodingUtil::UTF8ToWstring(currentPath);
                if (CreateDirectoryW(wpath.c_str(), nullptr) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
                    return false;
                }
            }
        }
        currentPath += pathStr[i];
    }

    if (!currentPath.empty()) {
		const auto wpath = EncodingUtil::UTF8ToWstring(currentPath);
        return CreateDirectoryW(wpath.c_str(), nullptr) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
    }
    return true;
}

bool DeleteDir(STextView path) {
	const auto wpath = EncodingUtil::UTF8ToWstring(path);
    return RemoveDirectoryW(wpath.data()) != 0;
}

static bool DeleteDirRecursiveImpl(const std::string &pathStr) {
const auto wpath = EncodingUtil::UTF8ToWstring(pathStr);
    WIN32_FIND_DATAA findData;
    std::string      searchPath = pathStr + "\\*";
    HANDLE           hFind      = FindFirstFileA(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        // 目录可能为空或不存在，直接尝试删除目录本身
        return RemoveDirectoryA(pathStr.c_str()) != 0;
    }

    while (true) {
        // 跳过 . 和 ..
        if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
            std::string fullPath = pathStr + "\\" + findData.cFileName;
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // 递归删除子目录
                if (!DeleteDirRecursiveImpl(fullPath)) {
                    FindClose(hFind);
                    return false;
                }
            } else {
                // 删除文件
                if (!::DeleteFileA(fullPath.c_str())) {
                    FindClose(hFind);
                    return false;
                }
            }
        }

        // 尝试查找下一个文件/目录
        if (!FindNextFileA(hFind, &findData)) {
            break; // 没有更多条目或出错
        }
    }

    FindClose(hFind);

    // 最后删除当前目录
    return RemoveDirectoryA(pathStr.c_str()) != 0;
}

bool DeleteDirRecursive(STextView path) {
    return DeleteDirRecursiveImpl(path.to_string());
}

static std::expected<std::vector<FileInfo>, std::string> ListDirectoryImpl(const std::string &dirPathStr, bool includeSubdirs) {
    std::vector<FileInfo> result;

    WIN32_FIND_DATAA findData;
    std::string      searchPath = dirPathStr + "\\*";
    HANDLE           hFind      = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return std::unexpected("无法打开目录");
    }

    do {
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
            continue;
        }

        FileInfo info;
        info.name = findData.cFileName;
        info.path = dirPathStr + "\\" + findData.cFileName;
        info.type = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? EFileFolderType::DIRECTORY : EFileFolderType::FILE;
        info.size = (info.type == EFileFolderType::FILE) ? (static_cast<uint64_t>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow : 0;

        ULARGE_INTEGER ul;
        ul.LowPart        = findData.ftLastWriteTime.dwLowDateTime;
        ul.HighPart       = findData.ftLastWriteTime.dwHighDateTime;
        info.lastModified = (ul.QuadPart / 10000000ULL) - 11644473600ULL;

        result.push_back(info);

        if (includeSubdirs && info.type == EFileFolderType::DIRECTORY) {
            auto subdir = ListDirectoryImpl(info.path, true);
            if (subdir.has_value()) {
                result.insert(result.end(), subdir->begin(), subdir->end());
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    return result;
}

std::expected<std::vector<FileInfo>, std::string> ListDirectory(STextView dirPath, bool includeSubdirs) {
    return ListDirectoryImpl(dirPath.to_string(), includeSubdirs);
}

std::string GetCurrentDirectory() {
    std::array<char, MAX_PATH> buffer{};
    DWORD                      length = (::GetCurrentDirectoryA)(MAX_PATH, &buffer[0]);
    if (length == 0) {
        return "";
    }

    return {&buffer[0], length};
}

bool SetCurrentDirectory(STextView path) {
    return ::SetCurrentDirectoryA(path.data()) != 0;
}

} // namespace shine::util
