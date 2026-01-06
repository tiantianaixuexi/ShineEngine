#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <expected>
#include <span>
#include <optional>
#include <array>
#include "loader/core/loader.h"
#include "loader/image/image_loader.h"

namespace shine::image
{
	// ============================================================================
	// PNG 数据类型定义
	// ============================================================================

	/**
	 * @brief PNG 颜色类型枚举
	 * @see https://www.w3.org/TR/png-3/#11IHDR
	 */
	enum class PngColorType : uint8_t
	{
		GERY = 0,        ///< 灰度图像
		RGB = 2,         ///< 真彩色图像
		PALETTE = 3,     ///< 索引色图像（调色板）
		GERY_ALPHA = 4,  ///< 灰度 + Alpha 通道
		RGBA = 6         ///< 真彩色 + Alpha 通道
	};

	/**
	 * @brief PNG 时间信息结构体（tIME 块）
	 */
	struct PngTime
	{
		uint16_t year;    ///< 年（例如：1996）
		uint8_t  month;   ///< 月（1-12）
		uint8_t  day;     ///< 日（1-31）
		uint8_t  hour;    ///< 时（0-23）
		uint8_t  minute;  ///< 分（0-59）
		uint8_t  second;  ///< 秒（0-60，允许闰秒）
	};

	/**
	 * @brief PNG 透明度信息结构体（tRNS 块）
	 * @see https://www.w3.org/TR/png-3/#11tRNS
	 */
	struct PngTransparency
	{
		uint16_t gray = 0;                    ///< 灰度图像的透明色值（颜色类型 0）
		uint16_t red = 0;                     ///< RGB 图像的透明红色值（颜色类型 2）
		uint16_t green = 0;                   ///< RGB 图像的透明绿色值（颜色类型 2）
		uint16_t blue = 0;                    ///< RGB 图像的透明蓝色值（颜色类型 2）
		std::vector<uint8_t> paletteAlpha;    ///< 调色板图像的 Alpha 值数组（颜色类型 3）
		bool hasTransparency = false;         ///< 是否包含透明度信息
		PngColorType colorType = PngColorType::RGBA; ///< 关联的颜色类型
	};

	/**
	 * @brief PNG 背景颜色信息结构体（bKGD 块）
	 * @see https://www.w3.org/TR/png-3/#11bKGD
	 */
	struct PngBackground
	{
		uint16_t bg_red = 0;    ///< 背景红色值
		uint16_t bg_green = 0;  ///< 背景绿色值
		uint16_t bg_blue = 0;   ///< 背景蓝色值
	};

	/**
	 * @brief PNG 文本信息结构体（tEXt, zTXt, iTXt 块）
	 * @see https://www.w3.org/TR/png-3/#11tEXt
	 */
	struct PngTextInfo
	{
		std::string keyword;  ///< 关键字（最大 79 字节）
		std::string text;     ///< 文本内容
	};

	/**
	 * @brief PNG 物理像素尺寸信息结构体（pHYs 块）
	 * @see https://www.w3.org/TR/png-3/#11pHYs
	 */
	struct PngPhysical
	{
		uint32_t x = 0;     ///< 每单位水平像素数
		uint32_t y = 0;     ///< 每单位垂直像素数
		uint8_t unit = 0;   ///< 单位标志：0=未知，1=米
	};

	/**
	 * @brief PNG 伽马值信息结构体（gAMA 块）
	 * @see https://www.w3.org/TR/png-3/#11gAMA
	 */
	struct PngGamma
	{
		unsigned gamma = 0;  ///< 伽马值（乘以 100000）
	};

	/**
	 * @brief PNG 色度信息结构体（cHRM 块）
	 * @see https://www.w3.org/TR/png-3/#11cHRM
	 */
	struct PngChrm
	{
		uint32_t chrm_white_x;  ///< 白点 x 坐标（乘以 100000）
		uint32_t chrm_white_y;  ///< 白点 y 坐标（乘以 100000）
		uint32_t chrm_red_x;    ///< 红色 x 坐标（乘以 100000）
		uint32_t chrm_red_y;    ///< 红色 y 坐标（乘以 100000）
		uint32_t chrm_green_x;  ///< 绿色 x 坐标（乘以 100000）
		uint32_t chrm_green_y;  ///< 绿色 y 坐标（乘以 100000）
		uint32_t chrm_blue_x;   ///< 蓝色 x 坐标（乘以 100000）
		uint32_t chrm_blue_y;   ///< 蓝色 y 坐标（乘以 100000）
	};

	/**
	 * @brief PNG sRGB 色彩空间信息结构体（sRGB 块）
	 * @see https://www.w3.org/TR/png-3/#11sRGB
	 */
	struct PngSrgb
	{
		uint8_t srgb_intent = 0;  ///< 渲染意图：0=感知，1=相对色度，2=饱和度，3=绝对色度
	};

	/**
	 * @brief PNG CICP 色彩信息结构体（cICP 块）
	 * @see https://www.w3.org/TR/png-3/#11cICP
	 */
	struct PngCicp
	{
		uint8_t cicp_color_primaries = 0;          ///< 颜色原色
		uint8_t cicp_transfer_function = 0;       ///< 传输函数
		uint8_t cicp_matrix_coefficients = 0;      ///< 矩阵系数
		uint8_t cicp_video_full_range_flag = 0;   ///< 视频全范围标志
	};

	/**
	 * @brief PNG 主显示色彩体积信息结构体（mDCV 块）
	 * @see https://www.w3.org/TR/png-3/#mDCV-chunk
	 */
	struct PngMdcv
	{
		uint16_t r_x = 0;           ///< 红色 x 坐标
		uint16_t r_y = 0;           ///< 红色 y 坐标
		uint16_t g_x = 0;           ///< 绿色 x 坐标
		uint16_t g_y = 0;           ///< 绿色 y 坐标
		uint16_t b_x = 0;           ///< 蓝色 x 坐标
		uint16_t b_y = 0;           ///< 蓝色 y 坐标
		uint16_t w_x = 0;           ///< 白点 x 坐标
		uint16_t w_y = 0;           ///< 白点 y 坐标
		uint32_t max_luminance = 0; ///< 最大亮度
		uint32_t min_luminance = 0; ///< 最小亮度
	};

	/**
	 * @brief PNG 颜色亮度信息结构体（cLLI 块）
	 * @see https://www.w3.org/TR/png-3/#cLLI-chunk
	 */
	struct PngClli
	{
		uint32_t max_cll = 0;   ///< 最大内容亮度级别
		uint32_t max_fall = 0;  ///< 最大帧平均亮度级别
	};

	/**
	 * @brief PNG 有效位深度信息结构体（sBIT 块）
	 * @see https://www.w3.org/TR/png-3/#11sBIT
	 */
	struct PngSbit
	{
		uint8_t r = 0;  ///< 红色有效位深度
		uint8_t g = 0;  ///< 绿色有效位深度
		uint8_t b = 0;  ///< 蓝色有效位深度
		uint8_t a = 0;  ///< Alpha 有效位深度
	};

	// ============================================================================
	// PNG 解码器类
	// ============================================================================

	/**
	 * @brief PNG 图像解码器
	 * 
	 * 支持 PNG 规范的所有特性：
	 * - 所有位深度（1, 2, 4, 8, 16 位）
	 * - 所有颜色类型（灰度、RGB、RGBA、调色板）
	 * - Adam7 交织解码
	 * - 所有滤波器类型（None, Sub, Up, Average, Paeth）
	 * - Zlib/Deflate 解压缩
	 * - 完整的元数据支持（tEXt, zTXt, iTXt, tRNS, bKGD, pHYs, gAMA, cHRM, sRGB, cICP, mDCV, cLLI, sBIT）
	 * 
	 * @see https://www.w3.org/TR/png-3/
	 */
	class png : public shine::loader::IImageLoader
	{
	public:
		// ========================================================================
		// 构造与析构
		// ========================================================================

		png()
		{
			addSupportedExtension("png");
		}

		~png() override = default;

		// ========================================================================
		// IAssetLoader 接口实现
		// ========================================================================

		bool loadFromFile(const char* filePath) override;
		bool loadFromMemory(const void* data, size_t size) override;
		void unload() override;
		const char* getName() const override { return "pngLoader"; }
		const char* getVersion() const override { return "2.0.0"; }

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
		 * @brief 获取颜色类型
		 * @return 颜色类型枚举值
		 */
		PngColorType getColorType() const noexcept { return colorType; }

		/**
		 * @brief 检查是否已加载
		 * @return true 如果已加载
		 */
		bool isLoaded() const noexcept { return _loaded; }

		// ========================================================================
		// 公共接口：透明度查询
		// ========================================================================

		/**
		 * @brief 获取透明度信息
		 * @return 透明度信息结构体引用
		 */
		const PngTransparency& getTransparency() const noexcept { return transparency; }

		/**
		 * @brief 检查图像是否包含透明度信息
		 * @return true 如果包含透明度
		 */
		bool hasTransparency() const noexcept { return transparency.hasTransparency; }

		/**
		 * @brief 检查灰度像素是否透明
		 * @param x X 坐标
		 * @param y Y 坐标
		 * @param grayValue 灰度值
		 * @return true 如果像素透明
		 */
		bool isPixelTransparent(uint32_t x, uint32_t y, uint16_t grayValue) const noexcept;

		/**
		 * @brief 检查 RGB 像素是否透明
		 * @param x X 坐标
		 * @param y Y 坐标
		 * @param r 红色值
		 * @param g 绿色值
		 * @param b 蓝色值
		 * @return true 如果像素透明
		 */
		bool isPixelTransparent(uint32_t x, uint32_t y, uint16_t r, uint16_t g, uint16_t b) const noexcept;

		// ========================================================================
		// 公共接口：文本信息查询
		// ========================================================================

		/**
		 * @brief 获取所有文本信息
		 * @return 文本信息向量引用
		 */
		const std::vector<PngTextInfo>& getTextInfos() const noexcept { return textInfos; }

		// ========================================================================
		// 公共接口：PNG 文件解析
		// ========================================================================

		/**
		 * @brief 解析 PNG 文件
		 * @param filePath 文件路径
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> parsePngFile(std::string_view filePath);

		/**
		 * @brief 检查数据是否为有效的 PNG 文件（只验证文件头和IHDR块，不解析后续块）
		 * @param content 文件内容
		 * @return true 如果是有效的 PNG 文件
		 */
		bool IsPngFile(std::span<const std::byte> content) noexcept;

		// ========================================================================
		// 公共接口：PNG 块解析（内部使用，但保留为公共接口以便扩展）
		// ========================================================================

		bool read_plte(std::span<const std::byte> data, size_t length);   ///< 解析调色板块（PLTE）
		bool read_trns(std::span<const std::byte> data, size_t length);   ///< 解析透明度块（tRNS）
		bool read_bkgd(std::span<const std::byte> data, size_t length);   ///< 解析背景颜色块（bKGD）
		bool read_text(std::span<const std::byte> data, size_t length);   ///< 解析文本块（tEXt）
		bool read_ztxt(std::span<const std::byte> data, size_t length);   ///< 解析压缩文本块（zTXt）
		bool read_itxt(std::span<const std::byte> data, size_t length);   ///< 解析国际化文本块（iTXt）
		bool read_phys(std::span<const std::byte> data, size_t length);   ///< 解析物理像素块（pHYs）
		bool read_gama(std::span<const std::byte> data, size_t length);   ///< 解析伽马值块（gAMA）
		bool read_chrm(std::span<const std::byte> data, size_t length);   ///< 解析色度块（cHRM）
		bool read_srgb(std::span<const std::byte> data, size_t length);   ///< 解析 sRGB 块（sRGB）
		bool read_cicp(std::span<const std::byte> data, size_t length);   ///< 解析 CICP 块（cICP）
		bool read_mdcv(std::span<const std::byte> data, size_t length);   ///< 解析 mDCV 块（mDCV）
		bool read_clli(std::span<const std::byte> data, size_t length);   ///< 解析 cLLI 块（cLLI）
		bool read_exif(std::span<const std::byte> data, size_t length);   ///< 解析 EXIF 块（eXIf）
		bool read_sbit(std::span<const std::byte> data, size_t length);   ///< 解析有效位深度块（sBIT）

		// ========================================================================
		// 公共接口：图像解码
		// ========================================================================

		/**
		 * @brief 解码 PNG 图像数据为 RGBA 格式
		 * 
		 * 解码后的图像数据存储在 imageData 中，格式为 RGBA（每像素 4 字节），
		 * 像素顺序为：R, G, B, A（每个通道 8 位）
		 * 
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> decode();

		/**
		 * @brief 解码 PNG 图像数据为 RGB 格式
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

	private:
		// ========================================================================
		// 私有接口：PNG 解码核心实现
		// ========================================================================

		/**
		 * @brief PNG 解码内部实现
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> decodeInternal();

		/**
		 * @brief Zlib/Deflate 解压缩
		 * @param compressed 压缩的数据
		 * @return 成功返回解压缩数据，失败返回错误信息
		 */
		std::expected<std::vector<uint8_t>, std::string> zlibDecompress(
			std::span<const uint8_t> compressed) const;

		/**
		 * @brief PNG 扫描线滤波器处理
		 * @param recon 重建的扫描线（输出）
		 * @param scanline 当前扫描线（输入）
		 * @param prevline 前一条扫描线（可为 nullptr）
		 * @param bytewidth 每像素字节数
		 * @param filterType 滤波器类型（0-4）
		 */
		void unfilterScanline(
			std::span<uint8_t> recon,
			std::span<const uint8_t> scanline,
			const uint8_t* prevline,
			size_t bytewidth,
			uint8_t filterType) const;

		/**
		 * @brief Adam7 交织解码
		 * @param decompressed 解压缩的图像数据
		 * @param bytesPerPixel 每像素字节数
		 * @param scanlineWidth 扫描线宽度（字节）
		 * @return 成功返回去交织后的数据，失败返回错误信息
		 */
		std::expected<std::vector<uint8_t>, std::string> decodeAdam7(
			std::span<const uint8_t> decompressed,
			size_t bytesPerPixel,
			size_t scanlineWidth) const;

		/**
		 * @brief 移除填充位（用于低位深度图像）
		 * @param paddedData 包含填充位的数据
		 * @param actualBitsPerLine 每行实际位数
		 * @param paddedBitsPerLine 每行填充后的位数
		 * @param height 图像高度（行数）
		 * @return 移除填充位后的数据
		 */
		std::vector<uint8_t> removePaddingBits(
			const std::vector<uint8_t>& paddedData,
			size_t actualBitsPerLine,
			size_t paddedBitsPerLine,
			size_t height) const;

		/**
		 * @brief 从位流读取位（MSB 优先，用于低位深度）
		 * @param bitPos 位位置（输入/输出）
		 * @param data 数据指针
		 * @return 读取的位值（0 或 1）
		 */
		static inline uint8_t readBitFromStream(size_t& bitPos, const uint8_t* data) noexcept;

		/**
		 * @brief 写入位到位流（MSB 优先，用于低位深度）
		 * @param bitPos 位位置（输入/输出）
		 * @param data 数据指针
		 * @param bit 要写入的位值（0 或 1）
		 */
		static inline void writeBitToStream(size_t& bitPos, uint8_t* data, uint8_t bit) noexcept;

		/**
		 * @brief 颜色转换：将原始数据转换为 RGBA 格式
		 * @param rawData 原始图像数据
		 * @param output 输出 RGBA 数据（每像素 4 字节）
		 */
		void convertToRGBA(std::span<const uint8_t> rawData, std::vector<uint8_t>& output) const;

		/**
		 * @brief Paeth 预测器（用于 PNG 滤波器）
		 * @param a 左像素值
		 * @param b 上像素值
		 * @param c 左上像素值
		 * @return 预测值
		 */
		static constexpr uint8_t paethPredictor(int a, int b, int c) noexcept
		{
			int p = a + b - c;
			// Use constexpr-compatible abs implementation
			auto abs_val = [](int x) constexpr -> int { return x < 0 ? -x : x; };
			int pa = abs_val(p - a);
			int pb = abs_val(p - b);
			int pc = abs_val(p - c);
			if (pa <= pb && pa <= pc) return static_cast<uint8_t>(a);
			if (pb <= pc) return static_cast<uint8_t>(b);
			return static_cast<uint8_t>(c);
		}

		// ========================================================================
		// 成员变量：基本图像信息
		// ========================================================================

		std::string _name;              ///< 文件名
		bool _loaded = false;           ///< 是否已加载

		uint32_t _width = 0;            ///< 图像宽度（像素）
		uint32_t _height = 0;           ///< 图像高度（像素）

		uint8_t bitDepth = 0;           ///< 位深度（1, 2, 4, 8, 16）
		uint8_t compressionMethod = 0;  ///< 压缩方法（0 = deflate）
		uint8_t filterMethod = 0;       ///< 滤波器方法（0 = 标准）
		uint8_t interlaceMethod = 0;     ///< 交织方法（0 = 无交织，1 = Adam7）

		PngColorType colorType = PngColorType::RGBA;  ///< 颜色类型

		// ========================================================================
		// 成员变量：图像数据
		// ========================================================================

		std::vector<uint8_t> idatData;      ///< IDAT 块数据（压缩的图像数据）
		std::vector<uint8_t> rawPngData;    ///< 原始 PNG 文件数据
		std::vector<uint8_t> imageData;     ///< 解码后的图像数据（RGBA 格式，每像素 4 字节）

		// ========================================================================
		// 成员变量：调色板
		// ========================================================================

		size_t palettesize = 0;                                    ///< 调色板大小
		std::vector<std::array<uint8_t, 4>> palettColors;          ///< 调色板颜色（RGBA 格式）

		// ========================================================================
		// 成员变量：元数据
		// ========================================================================

		PngTime time;                                              ///< 时间信息（tIME）
		PngTransparency transparency;                              ///< 透明度信息（tRNS）
		std::optional<PngBackground> background;                   ///< 背景颜色（bKGD）
		std::optional<PngPhysical> phy;                            ///< 物理像素尺寸（pHYs）
		std::optional<PngGamma> gama;                              ///< 伽马值（gAMA）
		std::optional<PngChrm> chrm;                                ///< 色度信息（cHRM）
		std::optional<PngSrgb> srgb;                                ///< sRGB 信息（sRGB）
		std::optional<PngCicp> cicp;                                ///< CICP 信息（cICP）
		std::optional<PngMdcv> mdcv;                                ///< mDCV 信息（mDCV）
		std::optional<PngClli> clli;                                ///< cLLI 信息（cLLI）
		std::optional<PngSbit> sbit;                                ///< 有效位深度（sBIT）
		std::vector<PngTextInfo> textInfos;                        ///< 文本信息列表（tEXt, zTXt, iTXt）
	};

} // namespace shine::image
