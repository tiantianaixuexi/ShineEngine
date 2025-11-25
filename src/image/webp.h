#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <expected>
#include <span>
#include <optional>
#include <array>
#include <memory>

#include "loader/loader.h"

namespace shine::image
{
	// ============================================================================
	// WebP 数据类型定义
	// ============================================================================

	/**
	 * @brief WebP 格式类型枚举
	 * @see https://developers.google.com/speed/webp/docs/riff_container
	 */
	enum class WebPFormat : uint8_t
	{
		LOSSY = 0,      ///< 有损 WebP (VP8)
		LOSSLESS = 1,   ///< 无损 WebP (VP8L)
		EXTENDED = 2    ///< 扩展 WebP (VP8X)
	};

	/**
	 * @brief WebP 颜色空间枚举
	 */
	enum class WebPColorSpace : uint8_t
	{
		RGB = 0,
		YUV = 1
	};

	/**
	 * @brief WebP 动画信息结构体
	 */
	struct WebPAnimationInfo
	{
		uint32_t frameCount = 0;      ///< 帧数
		uint32_t loopCount = 0;        ///< 循环次数 (0 = 无限循环)
		uint32_t canvasWidth = 0;      ///< 画布宽度
		uint32_t canvasHeight = 0;    ///< 画布高度
	};

	// ============================================================================
	// WebP 解码器类
	// ============================================================================

	/**
	 * @brief WebP 图像解码器
	 * 
	 * 支持 WebP 格式的所有特性：
	 * - Lossy WebP (VP8 编码)
	 * - Lossless WebP (VP8L 编码)
	 * - Extended WebP (VP8X 块，支持动画、ICC、EXIF 等)
	 * - Alpha 通道支持 (有损/无损 Alpha)
	 * - ICC 色彩配置文件
	 * - EXIF 元数据
	 * 
	 * @see https://developers.google.com/speed/webp/docs/riff_container
	 */
	class webp : public shine::loader::IAssetLoader
	{
	public:
		// ========================================================================
		// 构造与析构
		// ========================================================================

		webp()
		{
			addSupportedExtension("webp");
		}

		~webp() override = default;

		// ========================================================================
		// IAssetLoader 接口实现
		// ========================================================================

		bool loadFromFile(const char* filePath) override;
		bool loadFromMemory(const void* data, size_t size) override;
		void unload() override;
		const char* getName() const override { return "webpLoader"; }
		const char* getVersion() const override { return "1.0.0"; }

		// ========================================================================
		// 公共接口：文件信息查询
		// ========================================================================

		/**
		 * @brief 获取文件名
		 * @return 文件名视图
		 */
		std::string_view getFileName() const noexcept;

		/**
		 * @brief 获取图像宽度
		 * @return 宽度（像素）
		 */
		constexpr uint32_t getWidth() const noexcept { return _width; }

		/**
		 * @brief 获取图像高度
		 * @return 高度（像素）
		 */
		constexpr uint32_t getHeight() const noexcept { return _height; }

		/**
		 * @brief 获取格式类型
		 * @return 格式类型枚举值
		 */
		WebPFormat getFormat() const noexcept { return format; }

		/**
		 * @brief 检查是否包含 Alpha 通道
		 * @return true 如果包含 Alpha 通道
		 */
		bool hasAlpha() const noexcept { return _hasAlpha; }

		/**
		 * @brief 检查是否包含动画
		 * @return true 如果包含动画
		 */
		bool hasAnimation() const noexcept { return _hasAnimation; }

		/**
		 * @brief 检查是否已加载
		 * @return true 如果已加载
		 */
		bool isLoaded() const noexcept { return _loaded; }

		// ========================================================================
		// 公共接口：WebP 文件解析
		// ========================================================================

		/**
		 * @brief 解析 WebP 文件
		 * @param filePath 文件路径
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> parseWebPFile(std::string_view filePath);

		/**
		 * @brief 检查数据是否为有效的 WebP 文件
		 * @param content 文件内容
		 * @return true 如果是有效的 WebP 文件
		 */
		constexpr bool IsWebPFile(std::span<const std::byte> content) noexcept;

		// ========================================================================
		// 公共接口：图像解码
		// ========================================================================

		/**
		 * @brief 解码 WebP 图像数据为 RGBA 格式
		 * 
		 * 解码后的图像数据存储在 imageData 中，格式为 RGBA（每像素 4 字节），
		 * 像素顺序为：R, G, B, A（每个通道 8 位）
		 * 
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> decode();

		/**
		 * @brief 解码 WebP 图像数据为 RGB 格式
		 * 
		 * 解码后的图像数据格式为 RGB（每像素 3 字节），
		 * 像素顺序为：R, G, B（每个通道 8 位）
		 * 
		 * @return 成功返回 RGB 数据向量，失败返回错误信息
		 */
		std::expected<std::vector<uint8_t>, std::string> decodeRGB();

		/**
		 * @brief 获取解码后的图像数据（RGBA 格式）
		 * @return RGBA 图像数据向量引用（每像素 4 字节）
		 */
		const std::vector<uint8_t>& getImageData() const noexcept { return imageData; }

		/**
		 * @brief 检查是否已解码
		 * @return true 如果图像已解码
		 */
		bool isDecoded() const noexcept { return !imageData.empty(); }

		// ========================================================================
		// 公共接口：元数据查询
		// ========================================================================

		/**
		 * @brief 获取 ICC 配置文件
		 * @return ICC 配置文件数据向量引用
		 */
		const std::vector<uint8_t>& getICCProfile() const noexcept { return iccProfile; }

		/**
		 * @brief 检查是否包含 ICC 配置文件
		 * @return true 如果包含 ICC 配置文件
		 */
		bool hasICCProfile() const noexcept { return !iccProfile.empty(); }

		/**
		 * @brief 获取 EXIF 数据
		 * @return EXIF 数据向量引用
		 */
		const std::vector<uint8_t>& getEXIFData() const noexcept { return exifData; }

		/**
		 * @brief 检查是否包含 EXIF 数据
		 * @return true 如果包含 EXIF 数据
		 */
		bool hasEXIFData() const noexcept { return !exifData.empty(); }

		/**
		 * @brief 获取动画信息
		 * @return 动画信息结构体引用
		 */
		const WebPAnimationInfo& getAnimationInfo() const noexcept { return animationInfo; }

	private:
		// ========================================================================
		// 私有接口：WebP 解码核心实现
		// ========================================================================

		/**
		 * @brief WebP 解码内部实现
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> decodeInternal();

		/**
		 * @brief 解析 WebP 数据
		 * @param data WebP 文件数据
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> parseWebPData(std::span<const std::byte> data);

		/**
		 * @brief 提取 WebP 格式特征
		 * @return true 如果提取成功
		 */
		bool extractFeatures();

		/**
		 * @brief 提取 WebP 块中的元数据（ICC、EXIF 等）
		 * @return true 如果提取成功
		 */
		bool extractChunks();

		/**
		 * @brief 解码 VP8L 无损格式
		 * @param data VP8L 块数据
		 * @param size VP8L 块大小
		 * @param output 输出缓冲区（RGBA 格式）
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> decodeVP8L(const uint8_t* data, size_t size, std::vector<uint8_t>& output);

		/**
		 * @brief 解码 VP8 有损格式
		 * @param data VP8 块数据
		 * @param size VP8 块大小
		 * @param output 输出缓冲区（RGBA 格式）
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> decodeVP8(const uint8_t* data, size_t size, std::vector<uint8_t>& output);

		/**
		 * @brief 解码 ALPH Alpha 通道块
		 * @param data ALPH 块数据
		 * @param size ALPH 块大小
		 * @param output 输出缓冲区（Alpha 通道数据）
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> decodeALPH(const uint8_t* data, size_t size, std::vector<uint8_t>& output);

		// ========================================================================
		// 成员变量：基本图像信息
		// ========================================================================

		std::string _name;              ///< 文件名
		bool _loaded = false;          ///< 是否已加载

		uint32_t _width = 0;           ///< 图像宽度（像素）
		uint32_t _height = 0;          ///< 图像高度（像素）

		WebPFormat format = WebPFormat::LOSSY;  ///< 格式类型
		bool _hasAlpha = false;        ///< 是否包含 Alpha 通道
		bool _hasAnimation = false;    ///< 是否包含动画

		// ========================================================================
		// 成员变量：图像数据
		// ========================================================================

		std::vector<uint8_t> rawWebPData;    ///< 原始 WebP 文件数据
		std::vector<uint8_t> imageData;      ///< 解码后的图像数据（RGBA 格式，每像素 4 字节）

		// ========================================================================
		// 成员变量：元数据
		// ========================================================================

		std::vector<uint8_t> iccProfile;     ///< ICC 色彩配置文件
		std::vector<uint8_t> exifData;       ///< EXIF 元数据
		WebPAnimationInfo animationInfo;      ///< 动画信息
	};

} // namespace shine::image

