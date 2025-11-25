#include "AssetManager.h"
#include "image/png.h"
#include "image/jpeg.h"
#include "image/webp.h"
#include "image/Texture.h"
#include "loader/model/gltfLoader.h"
#include "loader/model/objLoader.h"
#include "fmt/format.h"
#include "util/function_timer.h"
#include <algorithm>
#include <cstring>

namespace shine::manager
{
    AssetManager::AssetManager()
    {
    }

    AssetManager::~AssetManager()
    {
        Shutdown();
    }

    void AssetManager::Initialize()
    {
        // 初始化资源管理器
    }

    void AssetManager::Shutdown()
    {
        UnloadAllAssets();
    }

    AssetHandle AssetManager::LoadImage(const std::string& filePath)
    {
        shine::util::FunctionTimer timer("AssetManager::LoadImage", shine::util::TimerPrecision::Nanoseconds);

        // 检查是否已加载
        auto it = pathToHandle_.find(filePath);
        if (it != pathToHandle_.end())
        {
            AssetHandle handle;
            handle.id = it->second;
            handle.type = EAssetType::Image;
            handle.path = filePath;
            return handle;
        }

        // 创建加载器
        std::filesystem::path path(filePath);
        std::string ext = path.extension().string();
        if (!ext.empty() && ext[0] == '.')
        {
            ext = ext.substr(1);
        }
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        auto loader = CreateImageLoader(ext);
        if (!loader)
        {
            fmt::println("AssetManager: 不支持的图片格式: {}", filePath);
            return AssetHandle{};
        }

        // 加载文件
        if (!loader->loadFromFile(filePath.c_str()))
        {
            fmt::println("AssetManager: 图片文件加载失败: {} - 错误: {}", filePath, static_cast<int>(loader->getLastError()));
            return AssetHandle{};
        }

        // 解码图片（数据存储在加载器中）
        auto decodeResult = loader->decode();
        if (!decodeResult.has_value())
        {
            fmt::println("AssetManager: 图片解码失败: {} - {}", filePath, decodeResult.error());
            return AssetHandle{};
        }

        // 验证数据
        if (!loader->isDecoded() || loader->getImageData().empty())
        {
            fmt::println("AssetManager: 图片数据为空: {}", filePath);
            return AssetHandle{};
        }

        // 创建句柄
        AssetHandle handle;
        handle.id = nextHandleId_++;
        handle.type = EAssetType::Image;
        handle.path = filePath;

        // 保存加载器（数据由加载器持有）
        imageLoaders_[handle.id] = std::move(loader);
        pathToHandle_[filePath] = handle.id;

        fmt::println("AssetManager: 图片加载成功 - {}x{} - {} - {}", 
            imageLoaders_[handle.id]->getWidth(), 
            imageLoaders_[handle.id]->getHeight(), 
            ext, filePath);

        return handle;
    }

    AssetHandle AssetManager::LoadImageFromMemory(const void* data, size_t size, const std::string& formatHint)
    {
        shine::util::FunctionTimer timer("AssetManager::LoadImageFromMemory", shine::util::TimerPrecision::Nanoseconds);

        std::string format = formatHint;
        if (format.empty())
        {
            format = DetectImageFormat(data, size);
        }

        if (format.empty())
        {
            fmt::println("AssetManager: 无法识别图片格式");
            return AssetHandle{};
        }

        auto loader = CreateImageLoader(format);
        if (!loader)
        {
            fmt::println("AssetManager: 不支持的图片格式: {}", format);
            return AssetHandle{};
        }

        // 从内存加载
        if (!loader->loadFromMemory(data, size))
        {
            fmt::println("AssetManager: 从内存加载图片失败");
            return AssetHandle{};
        }

        // 解码图片（数据存储在加载器中）
        auto decodeResult = loader->decode();
        if (!decodeResult.has_value())
        {
            fmt::println("AssetManager: 图片解码失败: {}", decodeResult.error());
            return AssetHandle{};
        }

        // 验证数据
        if (!loader->isDecoded() || loader->getImageData().empty())
        {
            fmt::println("AssetManager: 图片数据为空");
            return AssetHandle{};
        }

        // 创建句柄（内存加载没有路径）
        AssetHandle handle;
        handle.id = nextHandleId_++;
        handle.type = EAssetType::Image;
        handle.path = "";  // 内存加载没有路径

        // 保存加载器（数据由加载器持有）
        imageLoaders_[handle.id] = std::move(loader);

        fmt::println("AssetManager: 从内存加载图片成功 - {}x{} - {}", 
            imageLoaders_[handle.id]->getWidth(), 
            imageLoaders_[handle.id]->getHeight(), 
            format);

        return handle;
    }

    loader::IImageLoader* AssetManager::GetImageLoader(const AssetHandle& handle) const
    {
        if (!handle.isValid() || handle.type != EAssetType::Image)
        {
            return nullptr;
        }

        auto it = imageLoaders_.find(handle.id);
        if (it != imageLoaders_.end())
        {
            return it->second.get();
        }

        return nullptr;
    }

    std::shared_ptr<image::STexture> AssetManager::LoadTexture(const std::string& filePath)
    {
        // 先加载图片资源
        auto assetHandle = LoadImage(filePath);
        if (!assetHandle.isValid())
        {
            return nullptr;
        }

        // 创建 STexture 并初始化
        auto texture = std::make_shared<image::STexture>();
        if (!texture->InitializeFromAsset(assetHandle))
        {
            return nullptr;
        }

        return texture;
    }

    AssetHandle AssetManager::LoadModel(const std::string& filePath)
    {
        shine::util::FunctionTimer timer("AssetManager::LoadModel", shine::util::TimerPrecision::Nanoseconds);

        // 检查是否已加载
        auto it = pathToHandle_.find(filePath);
        if (it != pathToHandle_.end())
        {
            AssetHandle handle;
            handle.id = it->second;
            handle.type = EAssetType::Model;
            handle.path = filePath;
            return handle;
        }

        // 创建加载器
        std::filesystem::path path(filePath);
        std::string ext = path.extension().string();
        if (!ext.empty() && ext[0] == '.')
        {
            ext = ext.substr(1);
        }
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        auto loader = CreateModelLoader(ext);
        if (!loader)
        {
            fmt::println("AssetManager: 不支持的模型格式: {}", filePath);
            return AssetHandle{};
        }

        // 加载文件
        if (!loader->loadFromFile(filePath.c_str()))
        {
            fmt::println("AssetManager: 模型文件加载失败: {} - 错误: {}", filePath, static_cast<int>(loader->getLastError()));
            return AssetHandle{};
        }

        // 创建句柄
        AssetHandle handle;
        handle.id = nextHandleId_++;
        handle.type = EAssetType::Model;
        handle.path = filePath;

        // 保存加载器（数据由加载器持有）
        modelLoaders_[handle.id] = std::move(loader);
        pathToHandle_[filePath] = handle.id;

        fmt::println("AssetManager: 模型加载成功 - {} - 网格数: {}", filePath, modelLoaders_[handle.id]->getMeshCount());

        return handle;
    }

    loader::IModelLoader* AssetManager::GetModelLoader(const AssetHandle& handle) const
    {
        if (!handle.isValid() || handle.type != EAssetType::Model)
        {
            return nullptr;
        }

        auto it = modelLoaders_.find(handle.id);
        if (it != modelLoaders_.end())
        {
            return it->second.get();
        }

        return nullptr;
    }


    void AssetManager::UnloadAsset(const AssetHandle& handle)
    {
        if (!handle.isValid())
        {
            return;
        }

        if (handle.type == EAssetType::Image)
        {
            imageLoaders_.erase(handle.id);
        }
        else if (handle.type == EAssetType::Model)
        {
            modelLoaders_.erase(handle.id);
        }

        // 移除路径映射
        if (!handle.path.empty())
        {
            pathToHandle_.erase(handle.path);
        }
    }

    void AssetManager::UnloadAllAssets()
    {
        // 清空所有加载器（加载器析构时会自动清理数据）
        imageLoaders_.clear();
        modelLoaders_.clear();
        pathToHandle_.clear();
    }

    bool AssetManager::IsAssetLoaded(const AssetHandle& handle) const
    {
        if (!handle.isValid())
        {
            return false;
        }

        if (handle.type == EAssetType::Image)
        {
            return imageLoaders_.find(handle.id) != imageLoaders_.end();
        }
        else if (handle.type == EAssetType::Model)
        {
            return modelLoaders_.find(handle.id) != modelLoaders_.end();
        }

        return false;
    }

    AssetHandle AssetManager::GetAssetHandleByPath(const std::string& filePath) const
    {
        auto it = pathToHandle_.find(filePath);
        if (it != pathToHandle_.end())
        {
            AssetHandle handle;
            handle.id = it->second;
            handle.path = filePath;

            // 检测类型
            if (imageLoaders_.find(handle.id) != imageLoaders_.end())
            {
                handle.type = EAssetType::Image;
            }
            else if (modelLoaders_.find(handle.id) != modelLoaders_.end())
            {
                handle.type = EAssetType::Model;
            }
            else
            {
                handle.type = EAssetType::Unknown;
            }

            return handle;
        }

        return AssetHandle{};
    }

    std::vector<std::string> AssetManager::GetSupportedImageFormats()
    {
        return {"png", "jpeg", "jpg", "webp"};
    }

    std::vector<std::string> AssetManager::GetSupportedModelFormats()
    {
        return {"gltf", "glb", "obj"};
    }

    EAssetType AssetManager::DetectAssetType(const std::string& filePath) const
    {
        std::filesystem::path path(filePath);
        std::string ext = path.extension().string();
        if (!ext.empty() && ext[0] == '.')
        {
            ext = ext.substr(1);
        }
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // 图片格式
        if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp")
        {
            return EAssetType::Image;
        }

        // 模型格式
        if (ext == "gltf" || ext == "glb" || ext == "obj")
        {
            return EAssetType::Model;
        }

        return EAssetType::Unknown;
    }

    std::unique_ptr<loader::IImageLoader> AssetManager::CreateImageLoader(const std::string& format) const
    {
        std::string fmt = format;
        std::transform(fmt.begin(), fmt.end(), fmt.begin(), ::tolower);

        if (fmt == "png")
        {
            return std::make_unique<shine::image::png>();
        }
        else if (fmt == "jpeg" || fmt == "jpg")
        {
            return std::make_unique<shine::image::jpeg>();
        }
        else if (fmt == "webp")
        {
            return std::make_unique<shine::image::webp>();
        }

        return nullptr;
    }

    std::unique_ptr<loader::IModelLoader> AssetManager::CreateModelLoader(const std::string& format) const
    {
        std::string fmt = format;
        std::transform(fmt.begin(), fmt.end(), fmt.begin(), ::tolower);

        if (fmt == "gltf" || fmt == "glb")
        {
            return std::make_unique<shine::loader::gltfLoader>();
        }
        else if (fmt == "obj")
        {
            return std::make_unique<shine::loader::objLoader>();
        }

        return nullptr;
    }

    std::string AssetManager::DetectImageFormat(const void* data, size_t size) const
    {
        if (!data || size < 12)
        {
            return "";
        }

        const uint8_t* bytes = static_cast<const uint8_t*>(data);

        // 检测PNG: 89 50 4E 47 0D 0A 1A 0A
        if (size >= 8 && bytes[0] == 0x89 && bytes[1] == 0x50 && bytes[2] == 0x4E && bytes[3] == 0x47 &&
            bytes[4] == 0x0D && bytes[5] == 0x0A && bytes[6] == 0x1A && bytes[7] == 0x0A)
        {
            return "png";
        }

        // 检测JPEG: FF D8
        if (size >= 2 && bytes[0] == 0xFF && bytes[1] == 0xD8)
        {
            return "jpeg";
        }

        // 检测WebP: RIFF ... WEBP
        if (size >= 12 && 
            bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' &&
            bytes[8] == 'W' && bytes[9] == 'E' && bytes[10] == 'B' && bytes[11] == 'P')
        {
            return "webp";
        }

        return "";
    }
}
