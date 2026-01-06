#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>


namespace shine::loader
{

    // 错误类型定义
    enum class  EAssetLoaderError {
        NONE = 0,
        FILE_NOT_FOUND = 1,
        FILE_ACCESS_DENIED = 2,
        INVALID_FORMAT = 3,
        UNSUPPORTED_FEATURE = 4,
        PARSE_ERROR = 5,
        MEMORY_ALLOCATION_FAILED = 6,
        INVALID_PARAMETER = 7,
        ALREADY_LOADED = 8,
        LOAD_TIMEOUT = 9,
        DEPENDENCY_MISSING = 10,
        VERSION_MISMATCH = 11,
        CORRUPTION_DETECTED = 12,
        UNKNOWN_ERROR = 0xFF
    };

    // 加载状态定义
    enum class  EAssetLoadState {
        NONE = 0,
        QUEUED = 1,
        READING_FILE = 2,
        PARSING_DATA = 3,
        PROCESSING = 4,
        FINALIZING = 5,
        COMPLETE = 6,
        FAILD = 7,
        CANCELLED = 8,
        UNLOADING = 9
    };



    // 基本资源加载器接口
    class  IAssetLoader {
    public:
        IAssetLoader() {};
        virtual ~IAssetLoader() = default;

        // 异步加载方法 - 必须由子类实现
        virtual bool loadFromFile(const char* filePath) = 0;
        virtual bool loadFromMemory(const void* data, size_t size) = 0;

        // 卸载资源
        virtual void unload() = 0;

        // 获取加载器信息
        virtual const char* getName() const = 0;
        virtual const char* getVersion() const = 0;

        // 支持的扩展名
        const std::unordered_set<std::string>& getSupportedExtensions() const noexcept  {
            return _supportedExtensions;
        }

        const bool supportsExtension(const std::string& ext) const noexcept {
            return _supportedExtensions.count(ext) > 0;
        }

        // 状态查询
        const EAssetLoadState& getState() const  noexcept { return _currentState; }
        const EAssetLoaderError& getLastError() const noexcept  { return _lastError; }

        

    protected:
        // 工具方法
        void setState(EAssetLoadState state)noexcept  { _currentState = state; }
        void setError(EAssetLoaderError error, const std::string& message = "") noexcept {
            _lastError = error;
        }
        void addSupportedExtension(const char* ext) {
            if (ext) _supportedExtensions.insert(ext);
        }

        // 验证资源数据
        virtual bool validateAssetData(const void* data, size_t size) const;


    protected:
        std::unordered_set<std::string> _supportedExtensions;
        EAssetLoadState     _currentState{ EAssetLoadState::NONE };
        EAssetLoaderError    _lastError{ EAssetLoaderError::NONE };

    };


}


