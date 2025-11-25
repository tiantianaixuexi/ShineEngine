#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include "loader/core/loader.h"
#include "loader/image/image_loader.h"
#include "loader/model/model_loader.h"
#include <cstdint>

// 前向声明
namespace shine::image
{
    class STexture;
}

namespace shine::manager
{
    /**
     * @brief 资源类型枚举
     */
    enum class EAssetType
    {
        Image,
        Model,
        Texture,
        Audio,
        Unknown
    };

    /**
     * @brief 资源句柄（类似UE5的FAssetHandle）
     */
    struct AssetHandle
    {
        uint64_t id = 0;
        EAssetType type = EAssetType::Unknown;
        std::string path;

        bool isValid() const { return id != 0; }
    };

    /**
     * @brief 统一的资源管理器 - 类似UE5的AssetManager
     * 只负责资源加载和生命周期管理，数据由加载器本身持有
     */
    class AssetManager
    {
    public:
        static AssetManager& Get() noexcept
        {
            static AssetManager instance;
            return instance;
        }

        AssetManager();
        ~AssetManager();

        /**
         * @brief 初始化资源管理器
         */
        void Initialize();

        /**
         * @brief 关闭资源管理器，释放所有资源
         */
        void Shutdown();

        // ========================================================================
        // 图片资源管理
        // ========================================================================

        /**
         * @brief 加载图片资源（自动识别格式）
         * @param filePath 图片文件路径
         * @return 资源句柄，失败返回无效句柄
         */
        AssetHandle LoadImage(const std::string& filePath);

        /**
         * @brief 从内存加载图片资源
         * @param data 图片数据
         * @param size 数据大小
         * @param formatHint 格式提示（可选）
         * @return 资源句柄，失败返回无效句柄
         */
        AssetHandle LoadImageFromMemory(const void* data, size_t size, const std::string& formatHint = "");

        /**
         * @brief 获取图片加载器
         * @param handle 资源句柄
         * @return 图片加载器指针，失败返回nullptr
         */
        loader::IImageLoader* GetImageLoader(const AssetHandle& handle) const;

        /**
         * @brief 加载图片并创建 STexture 资源（便利方法，类似 UE5 的 LoadObject）
         * 一步完成：加载图片 -> 创建 STexture -> 返回共享指针
         * @param filePath 图片文件路径
         * @return STexture 共享指针，失败返回空指针
         */
        std::shared_ptr<image::STexture> LoadTexture(const std::string& filePath);

        /**
         * @brief 加载图片并创建 STexture 资源（便利方法，类似 UE5 的 LoadObject）
         * @param filePath 图片文件路径
         * @return STexture 共享指针，失败返回空指针
         */
        std::shared_ptr<image::STexture> LoadTexture(const std::string& filePath);

        // ========================================================================
        // 模型资源管理
        // ========================================================================

        /**
         * @brief 加载模型资源（自动识别格式）
         * @param filePath 模型文件路径
         * @return 资源句柄，失败返回无效句柄
         */
        AssetHandle LoadModel(const std::string& filePath);

        /**
         * @brief 获取模型加载器
         * @param handle 资源句柄
         * @return 模型加载器指针，失败返回nullptr
         */
        loader::IModelLoader* GetModelLoader(const AssetHandle& handle) const;

        // ========================================================================
        // 通用资源管理
        // ========================================================================

        /**
         * @brief 卸载资源
         * @param handle 资源句柄
         */
        void UnloadAsset(const AssetHandle& handle);

        /**
         * @brief 卸载所有资源
         */
        void UnloadAllAssets();

        /**
         * @brief 检查资源是否存在
         * @param handle 资源句柄
         * @return true如果资源存在
         */
        bool IsAssetLoaded(const AssetHandle& handle) const;

        /**
         * @brief 根据文件路径获取资源句柄
         * @param filePath 文件路径
         * @return 资源句柄，如果不存在返回无效句柄
         */
        AssetHandle GetAssetHandleByPath(const std::string& filePath) const;

        /**
         * @brief 获取支持的图片格式列表
         */
        static std::vector<std::string> GetSupportedImageFormats();

        /**
         * @brief 获取支持的模型格式列表
         */
        static std::vector<std::string> GetSupportedModelFormats();

    private:
        /**
         * @brief 根据文件扩展名检测资源类型
         */
        EAssetType DetectAssetType(const std::string& filePath) const;

        /**
         * @brief 创建图片加载器
         */
        std::unique_ptr<loader::IImageLoader> CreateImageLoader(const std::string& format) const;

        /**
         * @brief 创建模型加载器
         */
        std::unique_ptr<loader::IModelLoader> CreateModelLoader(const std::string& format) const;

        /**
         * @brief 从内存数据检测图片格式
         */
        std::string DetectImageFormat(const void* data, size_t size) const;

        // 资源存储：只存储加载器，数据由加载器本身持有
        std::unordered_map<uint64_t, std::unique_ptr<loader::IImageLoader>> imageLoaders_;
        std::unordered_map<uint64_t, std::unique_ptr<loader::IModelLoader>> modelLoaders_;
        std::unordered_map<std::string, uint64_t> pathToHandle_;  // 路径到句柄的映射

        uint64_t nextHandleId_ = 1;

    private:
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager(AssetManager&&) = delete;
        AssetManager& operator=(AssetManager&&) = delete;
    };
}
