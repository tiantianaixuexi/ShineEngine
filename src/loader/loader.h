#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <typeinfo>
#include <utility>


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
        ERROR = 7,
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
        const EAssetLoadState& getState() const { return _currentState; }
        const EAssetLoaderError& getLastError() const { return _lastError; }

        

    protected:
        // 工具方法
        void setState(EAssetLoadState state) { _currentState = state; }
        void setError(EAssetLoaderError error, const std::string& message = "") {
            _lastError = error;
        }
        void addSupportedExtension(const char* ext) {
            if (ext) _supportedExtensions.insert(ext);
        }

        // 验证资源数据
        virtual bool validateAssetData(const void* data, size_t size) const {
            return data != nullptr && size > 0;
        }


    protected:
        std::unordered_set<std::string> _supportedExtensions;
        EAssetLoadState     _currentState{ EAssetLoadState::NONE };
        EAssetLoaderError    _lastError{ EAssetLoaderError::NONE };

    };




    // 资源管理器
    class  AssetManager {
    public:
        static AssetManager& getInstance() noexcept {
            static AssetManager instance;
            return instance;
        }

        template<typename LoaderType, typename... Args>
        std::shared_ptr<LoaderType> getLoader(Args&&... args) {
            std::string type_name = typeid(LoaderType).name();

            if (_loaders.contains(type_name))
            {
                return std::static_pointer_cast<LoaderType>(_loaders[type_name]);
            }

            auto loader = std::make_shared<LoaderType>(std::forward<Args>(args)...);
            _loaders[type_name] = loader;
            return loader;

        }

      

        void shutdown() {
            _loaders.clear();
        }

    private:
        AssetManager() {};
        ~AssetManager() {};

        std::unordered_map<std::string, std::shared_ptr<IAssetLoader>> _loaders;

        std::string getFileExtension(const std::string& file_path) {
            size_t dot_pos = file_path.find_last_of('.');
            if (dot_pos != std::string::npos) {
                return file_path.substr(dot_pos);
            }
            return "";
        }
    };


}
