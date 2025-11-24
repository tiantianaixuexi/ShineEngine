#include "file_util.h"


#include "fmt/format.h"


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


#include <string>
#include <expected>
#include <span>
#include <vector>


namespace shine::util
{

    MappedView::MappedView(MappedView&& other) noexcept
        : baseAddress(other.baseAddress), content(other.content)
    {
        other.baseAddress = nullptr;
        other.content = {}; // 重置为空span
    }

    // 移动赋值运算符
    MappedView& MappedView::operator=(MappedView&& other) noexcept
    {
        if (this != &other)
        {
            clear();
            baseAddress = other.baseAddress;
            content = other.content;
            other.baseAddress = nullptr;
            other.content = {}; // 重置为空span
        }
        return *this;
    }

    // 通过取消映射内存来清理映射视图
    void MappedView::clear()
    {
#if defined(WIN32)
        if (baseAddress)
        {
            UnmapViewOfFile(baseAddress);
            baseAddress = nullptr;
            content = {};
        }
#elif defined(PLATFORM_WASM)
        // WebAssembly环境释放内存
        if (baseAddress)
        {
            free(baseAddress);
            baseAddress = nullptr;
            content = {};
        }
#endif
    }

    // 获取数据指针
    const std::byte* MappedView::data() const noexcept
    {
        return content.data();
    }

    // 数组访问运算符
    const std::byte& MappedView::operator[](size_t index) const noexcept
    {
        return data()[index];
    }

    // 获取数据大小访问器
    size_t MappedView::size() const noexcept
    {
        return content.size();
    }

    // 检查视图是否为空
    bool MappedView::empty() const noexcept
    {
        return content.empty();
    }

    // FileMapping 结构体方法实现

#if defined(WIN32)
    // 默认构造函数
    FileMapping::FileMapping() : fileHandle(nullptr), mappingHandle(nullptr) {}

    // 带文件句柄和映射句柄的构造函数
    FileMapping::FileMapping(void* fileHdl, void* mappingHdl)
        : fileHandle(fileHdl), mappingHandle(mappingHdl) {
    }
#elif defined(PLATFORM_WASM)
    // 默认构造函数
    FileMapping::FileMapping() : data(nullptr), size(0) {}

    // 带数据和大小的构造函数
    FileMapping::FileMapping(void* fileData, size_t fileSize)
        : data(fileData), size(fileSize) {
    }
#endif

    // 移动构造函数
    FileMapping::FileMapping(FileMapping&& other) noexcept
    {
#if defined(WIN32)
        fileHandle = other.fileHandle;
        mappingHandle = other.mappingHandle;
        other.fileHandle = nullptr;
        other.mappingHandle = nullptr;
#elif defined(PLATFORM_WASM)
        data = other.data;
        size = other.size;
        other.data = nullptr;
        other.size = 0;
#endif
    }

    // 移动赋值运算符
    FileMapping& FileMapping::operator=(FileMapping&& other) noexcept
    {
        if (this != &other)
        {
            clear();

#if defined(WIN32)
            fileHandle = other.fileHandle;
            mappingHandle = other.mappingHandle;
            other.fileHandle = nullptr;
            other.mappingHandle = nullptr;
#elif defined(PLATFORM_WASM)
            data = other.data;
            size = other.size;
            other.data = nullptr;
            other.size = 0;
#endif
        }
        return *this;
    }

#if defined(WIN32)
    // 检查文件句柄是否有效
	bool FileMapping::IsValidFileHandle() const noexcept
    {
        return fileHandle != nullptr && fileHandle != INVALID_HANDLE_VALUE;
    }

    // 检查映射句柄是否有效
	bool FileMapping::IsValidMapHandle() const noexcept
    {
        return mappingHandle != nullptr && mappingHandle != INVALID_HANDLE_VALUE;
    }
#endif

    // 检查映射是否有效
	bool FileMapping::IsValid() const noexcept
    {
#if defined(WIN32)
        return IsValidFileHandle() && IsValidMapHandle();
#elif defined(PLATFORM_WASM)
        return data != nullptr && size > 0;
#else
        return false;
#endif
    }

    // 清理资源
    void FileMapping::clear()
    {
#if defined(WIN32)
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
#elif defined(PLATFORM_WASM)
        if (data != nullptr)
        {
            free(data);
            data = nullptr;
            size = 0;
        }
#endif
    }

    // FileMapView 结构体方法实现

    // 默认构造函数
    FileMapView::FileMapView()
    {

    }

    // 移动构造函数
    FileMapView::FileMapView(FileMapping&& _m, MappedView&& _v) : map(std::move(_m)), view(std::move(_v))
    {

    }

    // 获取系统内存分配粒度
    static  uint32_t allocationGranularity = []() {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return sysInfo.dwAllocationGranularity;
        }();

    // 从给定的文件名中提取文件扩展名
    std::expected<std::string, bool> get_file_suffix(const std::string& filename)
    {
        const auto idx = filename.rfind('.');
        if (idx == std::string::npos)
        {
            return std::unexpected(false);
        }

        return filename.substr(idx);
    }

#ifdef _WIN32

    // 检查文件是否存在
    bool file_exists(std::string_view name)
    {
        auto attribute = GetFileAttributesA(name.data());
        return attribute != INVALID_FILE_ATTRIBUTES && !(attribute & FILE_ATTRIBUTE_DIRECTORY);
    }

    // 从文件映射中打开文件
    std::expected<FileMapping, std::string> open_file_from_mapping(const std::string& filename)
    {

        void* hFile = CreateFileA(
            filename.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            return std::unexpected(fmt::format("打开文件失败: {}", filename));
        }

        // 创建文件映射对象
        void* hMapping = CreateFileMappingA(
            hFile,
            nullptr,
            PAGE_READONLY,
            0,
            0,
            nullptr);

        if (hMapping == nullptr || hMapping == INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            return std::unexpected(fmt::format("创建文件映射失败: {}", filename));
        }

        // 返回成功结果
        return FileMapping{ hFile, hMapping };
    }

    std::expected<MappedView, std::string> read_data_from_mapping(FileMapping& mapping, uint64_t size, uint64_t offset)
    {

        if (!mapping.IsValidMapHandle())
        {
            return std::unexpected("文件映射句柄为空");
        }

        // 计算实际映射偏移和读取偏移
        uint64_t mapOffset = (offset / allocationGranularity) * allocationGranularity;
        uint64_t readOffset = offset - mapOffset;

        // 将64位偏移量拆分为高32位和低32位
        unsigned long offsetHigh = static_cast<unsigned long>((mapOffset >> 32) & 0xFFFFFFFF);
        unsigned long offsetLow = static_cast<unsigned  long>(mapOffset & 0xFFFFFFFF);

        // 映射文件视图，考虑读取偏移调整映射大小
        void* pFile = MapViewOfFile(
            mapping.mappingHandle,
            FILE_MAP_READ,
            offsetHigh,
            offsetLow,
            size + readOffset);

        if (pFile == nullptr)
        {
            return std::unexpected("映射文件到进程地址空间失败");
        }

        // 调整指针位置以考虑实际读取偏移
        const std::byte* pData = static_cast<const std::byte*>(pFile) + readOffset;

        // 返回映射视图，包含正确的地址和数据段
        return MappedView(pFile, pData, size);

    }

    // 检查文件/目录类型
    std::expected<EFileFolderType, bool> file_or_directory(const std::string& name)
    {
        // 确保进程代码页设置为UTF-8

        uint32_t attribute = GetFileAttributesA(name.c_str());

        if (attribute == INVALID_FILE_ATTRIBUTES)
        {
            return std::unexpected(false);
        }
        if (attribute & FILE_ATTRIBUTE_DIRECTORY)
        {
            return EFileFolderType::DIRECTORY;
        }
        return EFileFolderType::FILE;
    }

    // 从映射中获取文件大小
    std::expected<uint64_t, bool> get_file_size(FileMapping& mapping)
    {

        LARGE_INTEGER fileSize{};

        if (!mapping.IsValidFileHandle() || !GetFileSizeEx(mapping.fileHandle, &fileSize))
        {
            mapping.clear();
            return std::unexpected(false);
        }

        return fileSize.QuadPart;
    }

    // 将整个文件读取到内存映射中
    std::expected <FileMapView, std::string > read_full_file(std::string_view filePath)
    {
        auto openResult = util::open_file_from_mapping(filePath.data());
        if (!openResult.has_value())
        {
            return std::unexpected(openResult.error());
        }

        // 获取Map和文件大小
        auto file_mapping = std::move(openResult.value());
        auto file_size = util::get_file_size(file_mapping);

        fmt::println("file_size:{}", file_size.value());

        if (!file_size.has_value())
        {
            return std::unexpected("get file size error");
        }

        // 直接读取文件数据
        auto file_data = util::read_data_from_mapping(file_mapping, file_size.value(), 0);
        if (!file_data.has_value())
        {
            return std::unexpected(file_data.error());
        }

        auto file_data_view = std::move(file_data.value());

        return FileMapView{std::move(file_mapping),std::move(file_data_view) };

    }

#endif

    unsigned int GetFileSize(std::string_view path)
    {
        unsigned int fileLen = 0;
        FILE* file = nullptr;
        fopen_s(&file, path.data(), "rb");

        if (file != nullptr)
        {
            fseek(file, 0, SEEK_END);
            fileLen = ftell(file);
            fclose(file);
        }

        return fileLen;
    }

    bool SaveData(std::string_view path, const void* data, size_t size)
    {

        std::string tmpPath(path);
        FILE* file = nullptr;
        errno_t err = fopen_s(&file, tmpPath.c_str(), "wb");
        if (err != 0 || file == nullptr)
            return false;

        size_t written = fwrite(data, 1, size, file);
        fclose(file);
        return written == size;
    }

    bool SaveData(std::string_view path, std::span<const std::byte> data)
    {
        return SaveData(path, data.data(), data.size());
    }



    bool CreateDir(const std::string& path)
    {



        size_t start = 0;
        size_t found = path.find_first_of("/\\", start);

        std::string subPath = "";
        std::vector<std::string> dirs;

        if (found != std::string::npos)
        {
            while (true)
            {
                subPath = path.substr(start, found - start + 1);
                if (!subPath.empty())
                    dirs.push_back(subPath);

                start = found + 1;
                found = path.find_first_of("/\\", start);
                if (found == std::string::npos)
                {
                    if (start < path.length())
                    {
                        dirs.push_back(path.substr(start));
                    }
                    break;
                }
            }
        }
        else {
            dirs.push_back(path);
        }
#ifdef SHINE_PLATFORM_WIN64

        if ((GetFileAttributesA(path.c_str())) == INVALID_FILE_ATTRIBUTES)
        {
            subPath = "";
            for (unsigned int i = 0; i < dirs.size(); ++i)
            {
                subPath += dirs[i];
                if (GetFileAttributesA(subPath.c_str()) == INVALID_FILE_ATTRIBUTES)
                {
                    BOOL ret = CreateDirectoryA(subPath.c_str(), NULL);
                    if (!ret && ERROR_ALREADY_EXISTS != GetLastError())
                    {
                        return false;
                    }
                }
            }
        }
        return true;

#endif


        return false;
    }

}


