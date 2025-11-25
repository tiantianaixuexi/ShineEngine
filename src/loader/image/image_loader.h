#pragma once

#include "loader/core/loader.h"
#include <vector>
#include <string>
#include <expected>

namespace shine::loader
{
    /**
     * @brief 图片加载器接口 - 统一图片格式加载器的抽象接口
     * 继承自 IAssetLoader，提供图片特定的接口
     */
    class IImageLoader : public IAssetLoader
    {
    public:
        virtual ~IImageLoader() = default;

        /**
         * @brief 获取文件名
         * @return 文件名视图
         */
        virtual std::string_view getFileName() const noexcept = 0;

        /**
         * @brief 获取图像宽度
         * @return 宽度（像素）
         */
        virtual uint32_t getWidth() const noexcept = 0;

        /**
         * @brief 获取图像高度
         * @return 高度（像素）
         */
        virtual uint32_t getHeight() const noexcept = 0;

        /**
         * @brief 检查是否已加载
         * @return true 如果已加载
         */
        virtual bool isLoaded() const noexcept = 0;

        /**
         * @brief 解码图片数据为 RGBA 格式
         * 
         * 解码后的图像数据存储在内部，格式为 RGBA（每像素 4 字节），
         * 像素顺序为：R, G, B, A（每个通道 8 位）
         * 
         * @return 成功返回 void，失败返回错误信息
         */
        virtual std::expected<void, std::string> decode() = 0;

        /**
         * @brief 解码图片数据为 RGB 格式（可选）
         * 
         * 解码后的图像数据格式为 RGB（每像素 3 字节），
         * 像素顺序为：R, G, B（每个通道 8 位）
         * 
         * @return 成功返回 RGB 数据向量，失败返回错误信息
         */
        virtual std::expected<std::vector<uint8_t>, std::string> decodeRGB() = 0;

        /**
         * @brief 获取解码后的图像数据（RGBA 格式）
         * @return RGBA 图像数据向量引用（每像素 4 字节）
         */
        virtual const std::vector<uint8_t>& getImageData() const noexcept = 0;

        /**
         * @brief 检查是否已解码
         * @return true 如果图像已解码
         */
        virtual bool isDecoded() const noexcept = 0;
    };
}

