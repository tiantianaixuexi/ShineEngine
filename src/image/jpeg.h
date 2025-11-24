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

namespace shine::util
{
	class BitReader;
}

namespace shine::image
{


	// ============================================================================
	// JPEG 数据类型定义
	// ============================================================================

	/**
	 * @brief JPEG 颜色空间枚举
	 */
	enum class JpegColorSpace : uint8_t
	{
		GRAYSCALE = 1,   ///< 灰度图像
		YCbCr = 3,       ///< YCbCr 颜色空间（RGB）
		RGB = 3,         ///< RGB 颜色空间（等同于 YCbCr）
		CMYK = 4,        ///< CMYK 颜色空间
		YCCK = 4         ///< YCCK 颜色空间（等同于 CMYK）
	};

	/**
	 * @brief JPEG 采样因子结构
	 */
	struct JpegSamplingFactor
	{
		uint8_t h = 1;  ///< 水平采样因子
		uint8_t v = 1;  ///< 垂直采样因子
	};

	/**
	 * @brief JPEG 颜色分量信息结构
	 */
	struct JpegComponent
	{
		uint8_t id = 0;                    ///< 分量 ID（1=Y, 2=Cb, 3=Cr）
		JpegSamplingFactor sampling;       ///< 采样因子
		uint8_t quantizationTableId = 0;   ///< 量化表 ID
		uint8_t huffmanDCTableId = 0;      ///< DC Huffman 表 ID
		uint8_t huffmanACTableId = 0;      ///< AC Huffman 表 ID
	};

	/**
	 * @brief JPEG Huffman 表结构
	 */
	struct JpegHuffmanTable
	{
		std::vector<uint8_t> codeLengths;  ///< 每个码长的符号数量
		std::vector<uint8_t> symbols;     ///< Huffman 符号值
		std::vector<uint16_t> codes;      ///< Huffman 码值（用于快速查找）
		bool isValid = false;              ///< 表是否有效
	};

	/**
	 * @brief JPEG 量化表结构
	 */
	struct JpegQuantizationTable
	{
		std::array<uint16_t, 64> coefficients;  ///< 64 个量化系数（8x8 块）
		bool isValid = false;                  ///< 表是否有效
	};

	// ============================================================================
	// JPEG 解码器类
	// ============================================================================

	/**
	 * @brief JPEG 图像解码器
	 * 
	 * 支持 JPEG/JPG 格式的基本解码功能：
	 * - JPEG 文件格式识别
	 * - SOF 段解析（图像尺寸和颜色分量）
	 * - DQT 段解析（量化表）
	 * - DHT 段解析（Huffman 表）
	 * - SOS 段解析（扫描参数）
	 * - Huffman 解码
	 * - 反量化
	 * - IDCT（逆离散余弦变换）
	 * - 颜色空间转换（YCbCr -> RGB）
	 * 
	 * @note 当前实现支持标准 JPEG 格式，渐进式 JPEG 支持待完善
	 */
	class jpeg : public loader::IAssetLoader
	{
	public:
		// ========================================================================
		// 构造与析构
		// ========================================================================

		jpeg()
		{
			addSupportedExtension("jpg");
			addSupportedExtension("jpeg");
		}

		~jpeg() override = default;

		// ========================================================================
		// IAssetLoader 接口实现
		// ========================================================================

		bool loadFromFile(const char* filePath) override;
		bool loadFromMemory(const void* data, size_t size) override;
		void unload() override;
		const char* getName() const override { return "jpegLoader"; }
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
		 * @brief 获取颜色分量数量
		 * @return 颜色分量数量（1=灰度，3=RGB/YCbCr）
		 */
		constexpr uint8_t getComponentCount() const noexcept { return componentCount; }

		/**
		 * @brief 检查是否已加载
		 * @return true 如果已加载
		 */
		bool isLoaded() const noexcept { return _loaded; }

		// ========================================================================
		// 公共接口：JPEG 文件解析
		// ========================================================================

		/**
		 * @brief 解析 JPEG 文件
		 * @param filePath 文件路径
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> parseJpegFile(std::string_view filePath);

		/**
		 * @brief 检查数据是否为有效的 JPEG 文件
		 * @param content 文件内容
		 * @return true 如果是有效的 JPEG 文件
		 */
		constexpr bool IsJpegFile(std::span<const std::byte> content) noexcept;

		// ========================================================================
		// 公共接口：图像解码
		// ========================================================================

		/**
		 * @brief 解码 JPEG 图像数据为 RGBA 格式
		 * 
		 * 解码后的图像数据存储在 imageData 中，格式为 RGBA（每像素 4 字节），
		 * 像素顺序为：R, G, B, A（每个通道 8 位）
		 * 
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> decode();

		/**
		 * @brief 解码 JPEG 图像数据为 RGB 格式
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
		// 私有接口：JPEG 解码核心实现
		// ========================================================================

		/**
		 * @brief JPEG 解码内部实现
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> decodeInternal();

		/**
		 * @brief 解析 JPEG 段
		 * @param data JPEG 文件数据
		 * @return 成功返回 void，失败返回错误信息
		 */
		std::expected<void, std::string> parseSegments(std::span<const std::byte> data);

		/**
		 * @brief 解析 SOF（Start of Frame）段
		 * @param data 段数据
		 * @param length 段长度
		 * @return true 如果解析成功
		 */
		bool parseSOF(std::span<const std::byte> data, size_t length);

		/**
		 * @brief 解析 DQT（Define Quantization Table）段
		 * @param data 段数据
		 * @param length 段长度
		 * @return true 如果解析成功
		 */
		bool parseDQT(std::span<const std::byte> data, size_t length);

		/**
		 * @brief 解析 DHT（Define Huffman Table）段
		 * @param data 段数据
		 * @param length 段长度
		 * @return true 如果解析成功
		 */
		bool parseDHT(std::span<const std::byte> data, size_t length);

		/**
		 * @brief 解析 SOS（Start of Scan）段
		 * @param data 段数据
		 * @param length 段长度
		 * @return true 如果解析成功
		 */
		bool parseSOS(std::span<const std::byte> data, size_t length);

		/**
		 * @brief 构建 Huffman 查找表
		 * @param table Huffman 表引用
		 */
		void buildHuffmanLookupTable(JpegHuffmanTable& table);

		/**
		 * @brief Huffman 解码符号
		 * @param reader 位读取器
		 * @param table Huffman 表
		 * @return 解码的符号值
		 */
		int32_t decodeHuffmanSymbol(util::BitReader& reader, const JpegHuffmanTable& table);

		/**
		 * @brief 解码 JPEG 扫描数据
		 * @param scanData 扫描数据
		 * @return 成功返回解码后的 MCU 数据，失败返回错误信息
		 */
		std::expected<std::vector<int16_t>, std::string> decodeScanData(
			std::span<const uint8_t> scanData);

		/**
		 * @brief 反量化 DCT 系数
		 * @param mcuData MCU 数据
		 * @param componentId 分量 ID
		 * @return 反量化后的 DCT 系数
		 */
		std::vector<int16_t> dequantize(const std::vector<int16_t>& mcuData, uint8_t componentId);

		/**
		 * @brief IDCT（逆离散余弦变换）
		 * @param coefficients DCT 系数（8x8 块）
		 * @return IDCT 后的像素值（8x8 块）
		 */
		std::array<int16_t, 64> idct(const std::array<int16_t, 64>& coefficients);

		/**
		 * @brief 颜色空间转换：YCbCr -> RGB
		 * @param yData Y 分量数据
		 * @param cbData Cb 分量数据
		 * @param crData Cr 分量数据
		 * @param output 输出 RGBA 数据
		 */
		void convertYCbCrToRGBA(
			const std::vector<int16_t>& yData,
			const std::vector<int16_t>& cbData,
			const std::vector<int16_t>& crData,
			std::vector<uint8_t>& output);

		/**
		 * @brief 颜色空间转换：灰度 -> RGBA
		 * @param grayData 灰度数据
		 * @param output 输出 RGBA 数据
		 */
		void convertGrayscaleToRGBA(
			const std::vector<int16_t>& grayData,
			std::vector<uint8_t>& output);

		// ========================================================================
		// 成员变量：基本图像信息
		// ========================================================================

		std::string _name;              ///< 文件名
		bool _loaded = false;           ///< 是否已加载

		uint32_t _width = 0;            ///< 图像宽度（像素）
		uint32_t _height = 0;           ///< 图像高度（像素）
		uint8_t precision = 8;          ///< 采样精度（通常为 8 位）
		uint8_t componentCount = 0;      ///< 颜色分量数量（1=灰度，3=RGB/YCbCr）

		// ========================================================================
		// 成员变量：图像数据
		// ========================================================================

		std::vector<uint8_t> rawJpegData;    ///< 原始 JPEG 文件数据
		std::vector<uint8_t> imageData;      ///< 解码后的图像数据（RGBA 格式，每像素 4 字节）
		std::vector<uint8_t> scanData;       ///< 扫描数据（压缩的图像数据）

		// ========================================================================
		// 成员变量：颜色分量信息
		// ========================================================================

		std::vector<JpegComponent> components;  ///< 颜色分量信息列表

		// ========================================================================
		// 成员变量：量化表和 Huffman 表
		// ========================================================================

		std::array<JpegQuantizationTable, 4> quantizationTables;  ///< 量化表（最多 4 个）
		std::array<JpegHuffmanTable, 4> dcHuffmanTables;          ///< DC Huffman 表（最多 4 个）
		std::array<JpegHuffmanTable, 4> acHuffmanTables;          ///< AC Huffman 表（最多 4 个）

		// ========================================================================
		// 成员变量：扫描参数
		// ========================================================================

		uint8_t scanComponentCount = 0;  ///< 扫描中的分量数量
		std::vector<uint8_t> scanComponentIds;  ///< 扫描中的分量 ID 列表
	};

} // namespace shine::image

