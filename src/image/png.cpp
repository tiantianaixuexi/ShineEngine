#include "png.h"

#include <array>
#include <algorithm>
#include <string_view>
#include <string>
#include <expected>
#include <cstring>
#include <cmath>
#include <bit>
#include <climits>
#include <limits>

#ifndef __EMSCRIPTEN__
#if defined(__AVX2__) && (defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__))
#include <immintrin.h>
#endif
#endif

#include "fmt/format.h"
#include "fast_float/fast_float.h"

#include "shine_define.h"
#include "util/timer/function_timer.h"
#include "util/file_util.h"

#include "util/encoding/byte_convert.h"
#include "util/encoding/bit_reader.h"


/**
 * @file png.cpp
 * @brief PNG 图像解码器实现
 * 
 * PNG 格式规范：
 * - 文件头：8 字节签名 (89 50 4E 47 0D 0A 1A 0A)
 * - IHDR 块：13 字节数据 + 4 字节 CRC
 *   - Width (4 字节, 大端序)
 *   - Height (4 字节, 大端序)
 *   - BitDepth (1 字节: 1,2,4,8,16)
 *   - ColorType (1 字节: 0,2,3,4,6)
 *   - CompressionMethod (1 字节: 0 = deflate)
 *   - FilterMethod (1 字节: 0 = 标准)
 *   - InterlaceMethod (1 字节: 0 = 无交织, 1 = Adam7)
 * 
 * @see https://www.w3.org/TR/png-3/
 */

namespace shine::image
{
	using namespace util;

	// ============================================================================
	// PNG 格式常量定义
	// ============================================================================

	/// PNG 文件头签名（8 字节）+ IHDR 块类型标识（4 字节）
	static constexpr std::array png_header{
		std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47},  // PNG 签名
		std::byte{0x0D}, std::byte{0x0A}, std::byte{0x1A}, std::byte{0x0A},  // PNG 签名（续）
		std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x0D},  // IHDR 长度（13 字节）
		std::byte{'I'} , std::byte{'H'} , std::byte{'D'} , std::byte{'R'}   // IHDR 类型标识
	};

	/// PNG 块类型标识符
	static constexpr std::array IDAT{ std::byte{'I'}, std::byte{'D'}, std::byte{'A'}, std::byte{'T'} };  ///< 图像数据块
	static constexpr std::array PLTE{ std::byte{'P'}, std::byte{'L'}, std::byte{'T'}, std::byte{'E'} };  ///< 调色板块
	static constexpr std::array TEXT{ std::byte{'t'}, std::byte{'E'}, std::byte{'X'}, std::byte{'T'} };  ///< 文本块
	static constexpr std::array TIME{ std::byte{'t'}, std::byte{'I'}, std::byte{'M'}, std::byte{'E'} };  ///< 时间块
	static constexpr std::array IEND{ std::byte{'I'}, std::byte{'E'}, std::byte{'N'}, std::byte{'D'} };  ///< 结束块
	static constexpr std::array TRNS{ std::byte{'t'}, std::byte{'R'}, std::byte{'N'}, std::byte{'S'} };  ///< 透明度块
	static constexpr std::array BKGD{ std::byte{'b'}, std::byte{'K'}, std::byte{'G'}, std::byte{'D'} };  ///< 背景颜色块
	static constexpr std::array ZTXT{ std::byte{'z'}, std::byte{'T'}, std::byte{'X'}, std::byte{'T'} };  ///< 压缩文本块
	static constexpr std::array ITXT{ std::byte{'i'}, std::byte{'T'}, std::byte{'X'}, std::byte{'T'} };  ///< 国际化文本块
	static constexpr std::array PHYS{ std::byte{'p'}, std::byte{'H'}, std::byte{'Y'}, std::byte{'S'} };  ///< 物理像素块
	static constexpr std::array GAMA{ std::byte{'g'}, std::byte{'A'}, std::byte{'M'}, std::byte{'A'} };  ///< 伽马值块
	static constexpr std::array CHRM{ std::byte{'c'}, std::byte{'H'}, std::byte{'R'}, std::byte{'M'} };  ///< 色度块
	static constexpr std::array SRGB{ std::byte{'s'}, std::byte{'R'}, std::byte{'G'}, std::byte{'B'} };  ///< sRGB 块
	static constexpr std::array ICCP{ std::byte{'i'}, std::byte{'C'}, std::byte{'C'}, std::byte{'P'} };  ///< ICC 配置文件块
	static constexpr std::array CICP{ std::byte{'c'}, std::byte{'I'}, std::byte{'C'}, std::byte{'P'} };  ///< CICP 块
	static constexpr std::array MDCV{ std::byte{'m'}, std::byte{'D'}, std::byte{'C'}, std::byte{'V'} };  ///< 主显示色彩体积块
	static constexpr std::array CLLI{ std::byte{'c'}, std::byte{'L'}, std::byte{'L'}, std::byte{'I'} };  ///< 颜色亮度块
	static constexpr std::array EXIF{ std::byte{'e'}, std::byte{'X'}, std::byte{'I'}, std::byte{'F'} };  ///< EXIF 块
	static constexpr std::array SBIT{ std::byte{'s'}, std::byte{'B'}, std::byte{'I'}, std::byte{'T'} };  ///< 有效位深度块

	// ============================================================================
	// 辅助函数
	// ============================================================================

	/**
	 * @brief 判断块类型是否匹配
	 * @param content 块类型数据（4 字节）
	 * @param str 期望的块类型标识符
	 * @return true 如果类型匹配
	 */
	template<typename T>
	constexpr bool JudgeChunkType(std::span<const std::byte> content, T str) noexcept
	{
		return std::equal(str.begin(), str.end(), content.begin());
	}

	// ============================================================================
	// IAssetLoader 接口实现
	// ============================================================================

	std::string_view png::getFileName() const noexcept
	{
		return _name;
	}

	bool png::loadFromFile(const char* filePath)
	{
		setState(shine::loader::EAssetLoadState::READING_FILE);
		
		auto result = parsePngFile(filePath);
		if (!result.has_value())
		{
			setError(shine::loader::EAssetLoaderError::PARSE_ERROR, result.error());
			setState(shine::loader::EAssetLoadState::ERROR);
			return false;
		}
		
		_name = filePath;
		_loaded = true;
		setState(shine::loader::EAssetLoadState::COMPLETE);
		return true;
	}
	
	bool png::loadFromMemory(const void* data, size_t size)
	{
		setState(shine::loader::EAssetLoadState::PARSING_DATA);
		
		if (!data || size == 0)
		{
			setError(shine::loader::EAssetLoaderError::INVALID_PARAMETER);
			setState(shine::loader::EAssetLoadState::ERROR);
			return false;
		}
		
		std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(data), size);
		if (!IsPngFile(dataSpan))
		{
			setError(shine::loader::EAssetLoaderError::INVALID_FORMAT);
			setState(shine::loader::EAssetLoadState::ERROR);
			return false;
		}
		
		// 保存原始数据并解析 PNG 结构
		rawPngData.assign(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size);
		
		setState(shine::loader::EAssetLoadState::PROCESSING);
		
		// 解析 PNG 块（类似 parsePngFile 的逻辑，但不从文件读取）
		// 这里需要解析所有块，包括 IDAT
		bool isEnd = false;
		std::span<const std::byte> chunkData = dataSpan.subspan(33); // 跳过文件头和 IHDR
		
		while (!isEnd && chunkData.size() >= 12)
		{
			uint32_t chunkLength = 0;
			read_be_ref(chunkData, chunkLength);
			
			std::span<const std::byte> type = chunkData.subspan(4);
			std::span<const std::byte> chunkDataContent = chunkData.subspan(8, chunkLength);
			
			if (JudgeChunkType(type, IDAT))
			{
				// 收集所有 IDAT 块的数据
				const size_t oldSize = idatData.size();
				idatData.resize(oldSize + chunkLength);
				std::memcpy(idatData.data() + oldSize, chunkDataContent.data(), chunkLength);
			}
			else if (JudgeChunkType(type, PLTE))
			{
				read_plte(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, TIME))
			{
				time.year = 256u * read_u8(chunkDataContent, 0) + read_u8(chunkDataContent, 1);
				read_be_ref(chunkDataContent, time.month, 2);
				read_be_ref(chunkDataContent, time.day, 3);
				read_be_ref(chunkDataContent, time.hour, 4);
				read_be_ref(chunkDataContent, time.minute, 5);
				read_be_ref(chunkDataContent, time.second, 6);
			}
			else if (JudgeChunkType(type, IEND))
			{
				isEnd = true;
			}
			else if (JudgeChunkType(type, TRNS))
			{
				read_trns(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, BKGD))
			{
				read_bkgd(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, TEXT))
			{
				read_text(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, ZTXT))
			{
				read_ztxt(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, ITXT))
			{
				read_itxt(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, PHYS))
			{
				read_phys(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, GAMA))
			{
				read_gama(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, CHRM))
			{
				read_chrm(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, SRGB))
			{
				read_srgb(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, ICCP))
			{
				// TODO: 实现 ICC 配置文件解析和内存管理
				// ICCP 块包含 ICC 颜色配置文件，通常较大，需要特殊处理
			}
			else if (JudgeChunkType(type, CICP))
			{
				read_cicp(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, MDCV))
			{
				read_mdcv(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, CLLI))
			{
				read_clli(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, EXIF))
			{
				read_exif(chunkDataContent, chunkLength);
			}
			else if (JudgeChunkType(type, SBIT))
			{
				read_sbit(chunkDataContent, chunkLength);
			}
			
			// PNG 块结构：4 字节长度 + 4 字节类型 + chunkLength 字节数据 + 4 字节 CRC
			// 移动到下一个块
			if (chunkData.size() < 12 + chunkLength)
			{
				fmt::println("警告: PNG 块数据不完整，停止解析");
				break;
			}
			chunkData = chunkData.subspan(12 + chunkLength);
		}
		
		_loaded = true;
		setState(shine::loader::EAssetLoadState::COMPLETE);
		return true;
	}
	
	void png::unload()
	{
		rawPngData.clear();
		idatData.clear();
		imageData.clear();
		palettColors.clear();
		textInfos.clear();
		transparency.paletteAlpha.clear();
		_width = 0;
		_height = 0;
		_loaded = false;
		setState(shine::loader::EAssetLoadState::NONE);
	}

	// ============================================================================
	// PNG 文件解析
	// ============================================================================

	std::expected<void, std::string> png::parsePngFile(std::string_view filePath)
	{
		util::FunctionTimer timer("解析 PNG 文件", util::TimerPrecision::Nanoseconds);

		setState(shine::loader::EAssetLoadState::READING_FILE);
		
		// 读取文件内容
		auto result = util::read_full_file(filePath);
		if (!result.has_value())
		{
			setError(shine::loader::EAssetLoaderError::FILE_NOT_FOUND, result.error());
			setState(shine::loader::EAssetLoadState::ERROR);
			return std::unexpected(result.error());
		}

		setState(shine::loader::EAssetLoadState::PARSING_DATA);
		
		auto file_data_view = std::move(result->view);

		// 验证 PNG 文件格式
		if (!IsPngFile(file_data_view.content))
		{
			setError(shine::loader::EAssetLoaderError::INVALID_FORMAT);
			setState(shine::loader::EAssetLoadState::ERROR);
			return std::unexpected("无效的 PNG 文件格式");
		}

		setState(shine::loader::EAssetLoadState::PROCESSING);

		// 保存完整的 PNG 文件数据用于后续解码
		rawPngData.assign(
			reinterpret_cast<const uint8_t*>(file_data_view.content.data()),
			reinterpret_cast<const uint8_t*>(file_data_view.content.data()) + file_data_view.content.size()
		);

		return {};
	}

	constexpr bool png::IsPngFile(std::span<const std::byte> content) noexcept
	{
		// 检查文件大小（至少需要文件头 + IHDR 块）
		if (content.size() < 33)
		{
			fmt::println("PNG 文件大小不足（至少需要 33 字节）");
			return false;
		}

		// 验证 PNG 文件头签名（8 字节）+ IHDR 块类型（4 字节）
		if (!std::equal(png_header.begin(), png_header.end(), content.begin()))
		{
			fmt::println("PNG 文件头签名无效");
			return false;
		}

		// 解析 IHDR 块数据（从偏移 16 开始，跳过文件头 8 字节 + 长度 4 字节 + 类型 4 字节）
		read_be_ref(content, _width, 16);            // 图像宽度（4 字节，大端序）
		read_be_ref(content, _height, 20);           // 图像高度（4 字节，大端序）
		read_be_ref(content, bitDepth, 24);          // 位深度（1 字节）
		read_be_ref(content, colorType, 25);          // 颜色类型（1 字节）
		read_be_ref(content, compressionMethod, 26); // 压缩方法（1 字节，0 = deflate）
		read_be_ref(content, filterMethod, 27);      // 滤波器方法（1 字节，0 = 标准）
		read_be_ref(content, interlaceMethod, 28);   // 交织方法（1 字节，0 = 无交织，1 = Adam7）

		fmt::println("PNG 图像信息: {}x{}, 位深度={}, 颜色类型={}, 压缩={}, 滤波={}, 交织={}",
			_width, _height, bitDepth, static_cast<u8>(colorType),
			compressionMethod, filterMethod, interlaceMethod);

		// 验证图像尺寸
		if (_width == 0 || _height == 0)
		{
			fmt::println("PNG 图像尺寸无效（宽度或高度为 0）");
			return false;
		}

		// 验证压缩方法（PNG 规范只支持 deflate）
		if (compressionMethod != 0)
		{
			fmt::println("PNG 压缩方法无效（必须为 0 = deflate）");
			return false;
		}

		// 验证颜色类型和位深度的组合（根据 PNG 规范）
		switch (colorType)
		{
		case PngColorType::GERY:
			if (bitDepth != 1 && bitDepth != 2 && bitDepth != 4 && bitDepth != 8 && bitDepth != 16)
			{
				fmt::println("PNG 灰度图像位深度无效（允许值: 1, 2, 4, 8, 16）");
				return false;
			}
			break;

		case PngColorType::RGB:
			if (bitDepth != 8 && bitDepth != 16)
			{
				fmt::println("PNG RGB 图像位深度无效（允许值: 8, 16）");
				return false;
			}
			break;

		case PngColorType::PALETTE:
			if (bitDepth != 1 && bitDepth != 2 && bitDepth != 4 && bitDepth != 8)
			{
				fmt::println("PNG 调色板图像位深度无效（允许值: 1, 2, 4, 8）");
				return false;
			}
			break;

		case PngColorType::GERY_ALPHA:
			if (bitDepth != 8 && bitDepth != 16)
			{
				fmt::println("PNG 灰度+Alpha 图像位深度无效（允许值: 8, 16）");
				return false;
			}
			break;

		case PngColorType::RGBA:
			if (bitDepth != 8 && bitDepth != 16)
			{
				fmt::println("PNG RGBA 图像位深度无效（允许值: 8, 16）");
				return false;
			}
			break;

		default:
			fmt::println("PNG 颜色类型无效（允许值: 0, 2, 3, 4, 6）");
			return false;
		}

		// 解析后续的 PNG 块（从偏移 33 开始，跳过文件头 8 字节 + IHDR 块 25 字节）
		bool isEnd = false;
		std::span<const std::byte> Data = content.subspan(33);
		while (!isEnd)
		{
			unsigned chunkLength = 0;
			read_be_ref(Data, chunkLength);

			std::span<const std::byte> type = Data.subspan(4);
			std::span<const std::byte> chunkData = Data.subspan(8, chunkLength);

			if (JudgeChunkType(type, IDAT))
			{
				// 收集所有 IDAT 块的数据
				const size_t oldSize = idatData.size();
				idatData.resize(oldSize + chunkLength);
				std::memcpy(idatData.data() + oldSize, chunkData.data(), chunkLength);
				fmt::println("长度: {},类型:IDAT, 累计大小: {}", chunkLength, idatData.size());
			}
			else if (JudgeChunkType(type, PLTE))
			{
				read_plte(chunkData, chunkLength);

				fmt::println("长度: {},类型:PLTE", chunkLength);

			}
			else if (JudgeChunkType(type, TIME))
			{
				time.year = 256u * read_u8(chunkData, 0) + read_u8(chunkData, 1);
				read_be_ref(chunkData, time.month, 2);
				read_be_ref(chunkData, time.day, 3);
				read_be_ref(chunkData, time.hour, 4);
				read_be_ref(chunkData, time.minute, 5);
				read_be_ref(chunkData, time.second, 6);

				fmt::println("PNG 文件包含时间块tIME");
				fmt::println("图片最后修改时间: {:04}-{:02}-{:02} {:02}:{:02}:{:02}", time.year, time.month, time.day, time.hour, time.minute, time.second);
			}
			else if (JudgeChunkType(type, IEND))
			{
				isEnd = true;
			}
			else if (JudgeChunkType(type, TRNS))
			{
				read_trns(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, BKGD))
			{
				read_bkgd(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, TEXT))
			{
				read_text(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, ZTXT))
			{
				read_ztxt(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, ITXT))
			{
				read_itxt(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, PHYS))
			{
				read_phys(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, GAMA))
			{
				read_gama(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, CHRM))
			{
				read_chrm(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, SRGB))
			{
				read_srgb(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, ICCP))
			{
				// TODO: 实现 ICC 配置文件解析和内存管理
				// ICCP 块包含 ICC 颜色配置文件，通常较大，需要特殊处理
			}
			else if (JudgeChunkType(type, CICP))
			{
				read_cicp(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, MDCV))
			{
				read_mdcv(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, CLLI))
			{
				read_clli(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, EXIF))
			{
				read_exif(chunkData, chunkLength);
			}
			else if (JudgeChunkType(type, SBIT))
			{
				read_sbit(chunkData, chunkLength);
			}

			// PNG 块结构：4 字节长度 + 4 字节类型 + chunkLength 字节数据 + 4 字节 CRC
			// 移动到下一个块
			if (Data.size() < 12 + chunkLength)
			{
				fmt::println("警告: PNG 块数据不完整，停止解析");
				break;
			}
			Data = Data.subspan(12 + chunkLength);
		}

		// 如果收集到了 IDAT 数据，尝试自动解码
		if (!idatData.empty())
		{
			auto decodeResult = decode();
			if (!decodeResult.has_value())
			{
				fmt::println("警告: PNG 解码失败: {}", decodeResult.error());
				// 不返回错误，因为元数据解析已经成功
			}
		}

		return true;
	}

	// ============================================================================
	// PNG 块解析函数
	// ============================================================================

	bool png::read_plte(std::span<const std::byte> data, size_t length)
	{
		// PLTE 块：调色板数据
		// 格式：每个颜色 3 字节（RGB），最多 256 种颜色
		// 长度必须是 3 的倍数，且不超过 768 字节（256 * 3）
		if (length == 0 || length % 3 != 0 || length > 768)
		{
			fmt::println("PLTE 块长度无效: {} 字节（必须是 3 的倍数，且 <= 768）", length);
			return false;
		}

		palettesize = length / 3;
		palettColors.clear();
		palettColors.reserve(palettesize);

		// 读取调色板颜色（RGB 格式，每 3 字节一个颜色）
		for (size_t i = 0; i < length; i += 3)
		{
			const uint8_t r = read_u8(data, i);
			const uint8_t g = read_u8(data, i + 1);
			const uint8_t b = read_u8(data, i + 2);

			// 存储为 RGBA 格式，Alpha 通道默认 255（不透明），后续可能由 tRNS 块更新
			palettColors.push_back({ r, g, b, 255 });
		}

		fmt::println("读取调色板: {} 种颜色", palettColors.size());
		return true;
	}

	bool png::read_trns(std::span<const std::byte> data, size_t length)
	{
		transparency.hasTransparency = true;
		transparency.colorType = colorType;

		switch (colorType)
		{
		case PngColorType::GERY:  // 颜色类型 0: 灰度
			if (length >= 2)
			{

				read_be_ref(data, transparency.gray, 0);
				fmt::println("灰度透明值: {}", transparency.gray);
				return true;
			}
			break;

		case PngColorType::RGB:   // 颜色类型 2: RGB
			if (length >= 6)
			{
				read_be_ref(data, transparency.red, 0);
				read_be_ref(data, transparency.green, 2);
				read_be_ref(data, transparency.blue, 4);

				fmt::println("RGB透明色 R={}, G={}, B={}",
					transparency.red, transparency.green, transparency.blue);
				return true;
			}
			break;

		case PngColorType::PALETTE:  // 颜色类型 3: 调色板
			if (length > 0 && length <= 256)
			{
				// 为调色板中的每个颜色指定Alpha值（单字节，直接使用）
				transparency.paletteAlpha.resize(data.size());
				memcpy(transparency.paletteAlpha.data(), data.data(), data.size());

				// 同步更新调色板颜色的Alpha通道
				for (size_t i = 0; i < min(length, palettColors.size()); ++i)
				{
					palettColors[i][3] = transparency.paletteAlpha[i];
				}

				fmt::println("调色板透明度 {} 个Alpha值", length);
				return true;
			}
			break;

		default:
			fmt::println("警告: 颜色类型 {} 不应有tRNS块", static_cast<int>(colorType));
			return false;
		}

		fmt::println("tRNS块长度无效: {} 字节", length);
		return false;
	}

	// ============================================================================
	// 透明度查询方法
	// ============================================================================

	bool png::isPixelTransparent(uint32_t x, uint32_t y, uint16_t grayValue) const noexcept
	{
		if (!hasTransparency() || colorType != PngColorType::GERY)
			return false;

		return (grayValue == transparency.gray);
	}

	bool png::isPixelTransparent(uint32_t x, uint32_t y, uint16_t r, uint16_t g, uint16_t b) const noexcept
	{
		if (!hasTransparency())
			return false;

		if (colorType == PngColorType::RGB)
		{
			return (r == transparency.red &&
				g == transparency.green &&
				b == transparency.blue);
		}

		return false;
	}

	bool png::read_bkgd(std::span<const std::byte> data, size_t length)
	{
		if (colorType == PngColorType::PALETTE)
		{
			if (length != 1)       return false;

			const uint8_t _r = read_u8(data, 0);

			if (_r >= palettesize) return false;

			background = PngBackground(
				_r, _r, _r
			);

		}
		else if (colorType == PngColorType::GERY || colorType == PngColorType::GERY_ALPHA)
		{
			if (length != 2) return false;

			const uint16_t _r = read_u16(data, 0);

			background = PngBackground(
				_r, _r, _r
			);

		}
		else if (colorType == PngColorType::RGB || colorType == PngColorType::RGBA)
		{
			if (length != 6) return false;

			background = PngBackground(
				read_u16(data, 0),
				read_u16(data,2),
				read_u16(data,4)
			);
		}

		return true;
	}

	bool png::read_text(std::span<const std::byte> data, size_t length)
	{
		if (length == 0)
		{
			fmt::println("tEXT 块长度为0");
			return false;
		}

		// tEXt: keyword\0text
		const char* const raw = reinterpret_cast<const char*>(data.data());
		const std::string_view fullView{ raw, length };

		const size_t nulPos = fullView.find('\0');
		if (nulPos == std::string_view::npos)
		{
			fmt::println("tEXT 块格式错误: 找不到关键字结束符");
			return false;
		}
		const std::string_view keywordView{ raw, nulPos };

		// 验证关键字长度
		if (keywordView.empty() || keywordView.size() > 79)
		{
			fmt::println("tEXT 块关键字长度无效: {} (应该为1-79字符)", keywordView.size());
			return false;
		}

		// 联函数优化字符验证
		auto isPrintableLatin1 = [](unsigned char c) noexcept -> bool {
			return (c >= 32 && c <= 126) || (c >= 161);
			};

		if (!std::all_of(keywordView.begin(), keywordView.end(),
			[&](char c) { return isPrintableLatin1(static_cast<unsigned char>(c)); }))
		{
			auto bad = std::find_if_not(keywordView.begin(), keywordView.end(),
				[&](char c) { return isPrintableLatin1(static_cast<unsigned char>(c)); });
			fmt::println("tEXT 块关键字包含无效字符: {}", static_cast<int>(*bad));
			return false;
		}

		// 计算文本内容的视图
		const size_t textStart = nulPos + 1;
		const std::string_view textView = (textStart < length) ?
			std::string_view{ raw + textStart, length - textStart } : std::string_view{};

		// 验证文本内容字符（允许\n, \r, \t 和Latin-1可打印字符）
		if (!textView.empty())
		{
			// 预分配空间优化
			textInfos.reserve(textInfos.size() + 1);

			// 批量验证文本字符
			auto isAllowedTextChar = [&](char c) noexcept -> bool {
				return c == '\n' || c == '\r' || c == '\t' ||
					isPrintableLatin1(static_cast<unsigned char>(c));
				};

			// 严格检查非标准字符
			for (char c : textView)
			{
				if (!isAllowedTextChar(c))
				{
					fmt::println("警告: tEXT 块文本包含非标准字符: {}", static_cast<int>(c));
				}
			}
		}

		// 一次性构造，避免拷贝
		textInfos.emplace_back(std::string{ keywordView }, std::string{ textView });

		// 使用 string_view 直接输出，避免临时字符串创建
		fmt::println("读取 tEXT 块: 关键字'{}', 文本长度={}",
			std::string_view{ keywordView }, textView.size());

		if (!textView.empty())
		{
			fmt::println("文本内容: '{}'", std::string_view{ textView });
		}

		return true;
	}

	bool png::read_ztxt(std::span<const std::byte> data, size_t length)
	{
		if (length == 0)
		{
			fmt::println("zTXt 块长度为0");
			return false;
		}

		const char* const raw = reinterpret_cast<const char*>(data.data());
		const std::string_view fullView{ raw, length };

		// 查找第一个NUL分隔符（关键字结束）
		const size_t firstNulPos = fullView.find('\0');
		if (firstNulPos == std::string_view::npos)
		{
			fmt::println("zTXt 块格式错误: 找不到关键字结束符");
			return false;
		}

		const std::string_view keywordView{ raw, firstNulPos };

		// 验证关键字长度
		if (keywordView.empty() || keywordView.size() > 79)
		{
			fmt::println("zTXt 块关键字长度无效: {} (应该为1-79字符)", keywordView.size());
			return false;
		}

		// 验证关键字字符
		auto isPrintableLatin1 = [](unsigned char c) noexcept -> bool {
			return (c >= 32 && c <= 126) || (c >= 161);
			};

		if (!std::all_of(keywordView.begin(), keywordView.end(),
			[&](char c) { return isPrintableLatin1(static_cast<unsigned char>(c)); }))
		{
			auto bad = std::find_if_not(keywordView.begin(), keywordView.end(),
				[&](char c) { return isPrintableLatin1(static_cast<unsigned char>(c)); });
			fmt::println("zTXt 块关键字包含无效字符: {}", static_cast<int>(*bad));
			return false;
		}

		// 检查是否有足够的字节读取压缩方法
		const size_t compressionMethodPos = firstNulPos + 1;
		if (compressionMethodPos >= length)
		{
			fmt::println("zTXt 块格式错误: 缺少压缩方法字节");
			return false;
		}

		// 读取压缩方法
		const uint8_t compressionMethod = read_u8(data, compressionMethodPos - 1);
		if (compressionMethod != 0)
		{
			fmt::println("zTXt 块使用不支持的压缩方法 {}", compressionMethod);
			return false;
		}

		// 获取压缩的文本数据
		const size_t compressedDataStart = compressionMethodPos + 1;
		if (compressedDataStart >= length)
		{
			// 空文本是允许的
			textInfos.emplace_back(std::string{ keywordView }, std::string{});
			fmt::println("读取 zTXt 块: 关键字'{}', 文本长度=0 (空文本)", keywordView);
			return true;
		}

		const size_t compressedDataLength = length - compressedDataStart;
		const std::span<const std::byte> compressedData = data.subspan(compressedDataStart, compressedDataLength);

		// 解压缩文本数据（zlib）
		std::span<const uint8_t> compressedText(reinterpret_cast<const uint8_t*>(compressedData.data()), compressedDataLength);
		auto decompressResult = zlibDecompress(compressedText);
		
		if (!decompressResult.has_value())
		{
			fmt::println("zTXt 块解压缩失败: {}", decompressResult.error());
			return false;
		}
		
		// 将解压缩的数据转换为字符串（Latin-1 编码）
		const auto& decompressed = decompressResult.value();
		std::string decompressedText(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());
		textInfos.emplace_back(std::string{ keywordView }, decompressedText);

		fmt::println("读取 zTXt 块: 关键字'{}', 压缩文本长度={}, 解压缩后长度={}",
			keywordView, compressedDataLength, decompressed.size());

		return true;
	}

	bool png::read_itxt(std::span<const std::byte> data, size_t length)
	{
		if (length == 0)
		{
			fmt::println("iTXt 块长度为0");
			return false;
		}

		const char* const raw = reinterpret_cast<const char*>(data.data());
		const std::string_view fullView{ raw, length };

		// 查找第一个NUL分隔符（关键字结束）
		const size_t firstNulPos = fullView.find('\0');
		if (firstNulPos == std::string_view::npos)
		{
			fmt::println("iTXt 块格式错误: 找不到关键字结束符");
			return false;
		}

		const std::string_view keywordView{ raw, firstNulPos };

		// 验证关键字长度
		if (keywordView.empty() || keywordView.size() > 79)
		{
			fmt::println("iTXt 块关键字长度无效: {} (应该为1-79字符)", keywordView.size());
			return false;
		}

		// 验证关键字字符（iTXT关键字可以包含UTF-8字符）
		auto isValidKeywordChar = [](unsigned char c) noexcept -> bool {
			// UTF-8字节验证，允许ASCII可打印字符和UTF-8多字节序列的开始字节
			return (c >= 32 && c <= 126) || (c >= 128);
			};

		if (!std::all_of(keywordView.begin(), keywordView.end(),
			[&](char c) { return isValidKeywordChar(static_cast<unsigned char>(c)); }))
		{
			auto bad = std::find_if_not(keywordView.begin(), keywordView.end(),
				[&](char c) { return isValidKeywordChar(static_cast<unsigned char>(c)); });
			fmt::println("iTXt 块关键字包含无效字符: {}", static_cast<int>(*bad));
			return false;
		}

		// 查找压缩标志
		const size_t compressionFlagPos = firstNulPos + 1;
		if (compressionFlagPos >= length)
		{
			fmt::println("iTXt 块格式错误: 缺少压缩标志");
			return false;
		}

		const uint8_t compressionFlag = read_u8(data, compressionFlagPos - 1);

		// 查找压缩方法
		const size_t compressionMethodPos = compressionFlagPos + 1;
		if (compressionMethodPos >= length)
		{
			fmt::println("iTXt 块格式错误: 缺少压缩方法");
			return false;
		}

		const uint8_t compressionMethod = read_u8(data, compressionMethodPos - 1);

		// 查找语言标签
		const size_t languageTagPos = compressionMethodPos + 1;
		const size_t secondNulPos = fullView.find('\0', languageTagPos - 1);
		if (secondNulPos == std::string_view::npos)
		{
			fmt::println("iTXt 块格式错误: 找不到语言标签结束符");
			return false;
		}

		const size_t languageTagLength = secondNulPos - (languageTagPos - 1);
		std::string languageTag;
		if (languageTagLength > 0)
		{
			languageTag.assign(raw + languageTagPos - 1, languageTagLength);
		}

		// 查找翻译关键字
		const size_t translatedKeywordPos = secondNulPos + 1;
		const size_t thirdNulPos = fullView.find('\0', translatedKeywordPos);
		if (thirdNulPos == std::string_view::npos)
		{
			fmt::println("iTXt 块格式错误: 找不到翻译关键字结束符");
			return false;
		}

		const size_t translatedKeywordLength = thirdNulPos - translatedKeywordPos;
		std::string translatedKeyword;
		if (translatedKeywordLength > 0)
		{
			translatedKeyword.assign(raw + translatedKeywordPos, translatedKeywordLength);
		}

		// 获取文本数据
		const size_t textStart = thirdNulPos + 1;
		if (textStart >= length)
		{
			// 空文本
			textInfos.emplace_back(std::string{ keywordView }, std::string{});
			fmt::println("读取 iTXt 块: 关键字'{}', 语言='{}', 翻译='{}', 文本长度=0",
				keywordView, languageTag, translatedKeyword);
			return true;
		}

		const size_t textLength = length - textStart;

		std::string textContent;
		if (compressionFlag == 0)
		{
			// 未压缩文本
			textContent.assign(raw + textStart, textLength);
			textInfos.emplace_back(std::string{ keywordView }, textContent);
			fmt::println("读取 iTXt 块: 关键字'{}', 语言='{}', 翻译='{}', 文本长度={}",
				keywordView, languageTag, translatedKeyword, textLength);
		}
		else if (compressionFlag == 1)
		{
			// 压缩文本
			if (compressionMethod != 0)
			{
				fmt::println("iTXt 块使用不支持的压缩方法 {}", compressionMethod);
				return false;
			}

			// 解压缩文本数据（zlib）
			const std::span<const std::byte> compressedData = data.subspan(textStart, textLength);
			std::span<const uint8_t> compressedText(reinterpret_cast<const uint8_t*>(compressedData.data()), textLength);
			auto decompressResult = zlibDecompress(compressedText);
			
			if (!decompressResult.has_value())
			{
				fmt::println("iTXt 块解压缩失败: {}", decompressResult.error());
				return false;
			}
			
			// 将解压缩的数据转换为字符串（UTF-8 编码）
			const auto& decompressed = decompressResult.value();
			textContent.assign(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());
			textInfos.emplace_back(std::string{ keywordView }, textContent);
			fmt::println("读取 iTXt 块: 关键字'{}', 语言='{}', 翻译='{}', 压缩文本长度={}, 解压缩后长度={}",
				keywordView, languageTag, translatedKeyword, textLength, decompressed.size());
		}
		else
		{
			fmt::println("iTXt 块压缩标志无效 {}", compressionFlag);
			return false;
		}

		return true;
	}

	bool png::read_phys(std::span<const std::byte> data, size_t length)
	{
		if (length != 9) return false;

		phy = PngPhysical(
			read_u32(data, 0),
			read_u32(data, 4),
			read_u8(data, 8)
		);

		return true;

	}

	bool png::read_gama(std::span<const std::byte> data, size_t length)
	{
		if (length != 4) return false;

		gama = PngGamma{
				read_u32(data,0)
		};

		return true;
	}

	bool png::read_chrm(std::span<const std::byte> data, size_t length)
	{
		if (length != 32) return false;

		shine::util::FunctionTimer __function_timer__("解析chrm", shine::util::TimerPrecision::Nanoseconds);

#ifndef __EMSCRIPTEN__
#if defined(__AVX2__) && (defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__))
		// AVX2 优化版本（非 WASM 平台）
		// 直接用AVX2一次性加载32字节并在每个32-bit单元做字节翻转（big-endian -> little-endian）
		const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data.data());
		__m256i bytes = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
		// 每个128-bit区块将4字节组的顺序翻转：[3,2,1,0],[7,6,5,4],...
		const __m256i shuf = _mm256_setr_epi8(
			3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12,
			3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12
		);
		__m256i swapped = _mm256_shuffle_epi8(bytes, shuf);

		// 存到临时数组并检查
		alignas(32) uint32_t vals[8];
		_mm256_storeu_si256(reinterpret_cast<__m256i*>(vals), swapped);

		chrm = PngChrm{
				vals[0],
				vals[1],
				vals[2],
				vals[3],
				vals[4],
				vals[5],
				vals[6],
				vals[7],
		};
#else
		// 标准实现（非 AVX2 或 WASM 平台）
		chrm = PngChrm{
			read_u32(data,0),
			read_u32(data,4),
			read_u32(data,8),
	        read_u32(data,12),
		    read_u32(data,16),
		  	read_u32(data,20),
			read_u32(data,24),
			read_u32(data,28),
		};
#endif
#else
		// WASM 平台使用标准实现
		chrm = PngChrm{
			read_u32(data,0),
			read_u32(data,4),
			read_u32(data,8),
	        read_u32(data,12),
		    read_u32(data,16),
		  	read_u32(data,20),
			read_u32(data,24),
			read_u32(data,28),
		};
#endif

		return true;
	}

	bool png::read_srgb(std::span<const std::byte> data, size_t length)
	{
		if (length != 1) return false;

		srgb = PngSrgb{ read_u8(data,0) };

		return true;
	}

	bool png::read_cicp(std::span<const std::byte> data, size_t length)
	{
		if (length != 4) return false;

		cicp = PngCicp{
		read_u8(data,0),
		read_u8(data,1),
		read_u8(data,2),
		read_u8(data,3)
		};

		return true;
	}

	bool png::read_mdcv(std::span<const std::byte> data, size_t length)
	{
		if (length != 24) return false;

#ifndef __EMSCRIPTEN__
#if defined(__AVX2__) && (defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__))
		// AVX2 优化版本（非 WASM 平台）
		#include <immintrin.h>
		
		alignas(16) uint16_t values[8];
		const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data.data());
		__m128i bytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
		const __m128i shuf = _mm_setr_epi8(
			1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14
		);
		__m128i swapped = _mm_shuffle_epi8(bytes, shuf);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(values), swapped);

		mdcv = PngMdcv{
			values[0],
			values[1],
			values[2],
			values[3],
			values[4],
			values[5],
			values[6],
			values[7],
			read_u32(data,16),
			read_u32(data,20)
		};
#else
		// 标准实现（非 AVX2 或 WASM 平台）
		mdcv = PngMdcv{
			read_u16(data,0),
			read_u16(data,2),
			read_u16(data,4),
			read_u16(data,6),
			read_u16(data,8),
			read_u16(data,10),
			read_u16(data,12),
			read_u16(data,14),
			read_u32(data,16),
			read_u32(data,20)
		};
#endif
#else
		// WASM 平台使用标准实现
		mdcv = PngMdcv{
			read_u16(data,0),
			read_u16(data,2),
			read_u16(data,4),
			read_u16(data,6),
			read_u16(data,8),
			read_u16(data,10),
			read_u16(data,12),
			read_u16(data,14),
			read_u32(data,16),
			read_u32(data,20)
		};
#endif

		return true;
	}

	bool png::read_clli(std::span<const std::byte> data, size_t length)
	{
		if (length != 8) return false;

		clli = PngClli{
			read_u32(data,0),
			read_u32(data,4)
		};

		return true;
	}

	bool png::read_exif(std::span<const std::byte> data, size_t length)
	{
		// TODO: 暂时不处理
		return  true;

	}

	bool png::read_sbit(std::span<const std::byte> data, size_t length)
	{
		// sBIT 块：指定每个颜色通道的有效位深度
		// 根据颜色类型，sBIT 块的长度和内容不同
		switch (colorType)
		{
		case PngColorType::GERY:
			// 灰度图像：1 字节（灰度有效位深度）
			if (length != 1) return false;
			{
				const uint8_t g = read_u8(data, 0);
				sbit = PngSbit{ g, g, g, 0 };
			}
			break;

		case PngColorType::RGB:
		case PngColorType::PALETTE:
			// RGB 或调色板图像：3 字节（R, G, B 有效位深度）
			if (length != 3) return false;
			sbit = PngSbit{
				read_u8(data, 0),
				read_u8(data, 1),
				read_u8(data, 2),
				0
			};
			break;

		case PngColorType::GERY_ALPHA:
			// 灰度 + Alpha 图像：2 字节（灰度有效位深度, Alpha 有效位深度）
			if (length != 2) return false;
			{
				const uint8_t g = read_u8(data, 0);
				sbit = PngSbit{ g, g, g, read_u8(data, 1) };
			}
			break;

		case PngColorType::RGBA:
			// RGBA 图像：4 字节（R, G, B, A 有效位深度）
			if (length != 4) return false;
			sbit = PngSbit{
				read_u8(data, 0),
				read_u8(data, 1),
				read_u8(data, 2),
				read_u8(data, 3)
			};
			break;

		default:
			return false;
		}

		return true;
	}

	// ============================================================================
	// PNG 图像解码
	// ============================================================================

	std::expected<void, std::string> png::decode()
	{
		return decodeInternal();
	}

	std::expected<void, std::string> png::decodeInternal()
	{
		if (idatData.empty())
		{
			return std::unexpected("没有 IDAT 数据可解码");
		}

		if (_width == 0 || _height == 0)
		{
			return std::unexpected("图像尺寸无效");
		}

		// 1. Zlib 解压缩 IDAT 数据
		auto decompressedResult = zlibDecompress(idatData);
		if (!decompressedResult.has_value())
		{
			return std::unexpected("Zlib 解压缩失败: " + decompressedResult.error());
		}
		
		auto& decompressed = decompressedResult.value();
		
		// 2. 计算每像素字节数（仅用于滤波器，低位深度按位计算）
		size_t bytesPerPixel = 0;
		switch (colorType)
		{
		case PngColorType::GERY:
			bytesPerPixel = (bitDepth == 16) ? 2 : ((bitDepth >= 8) ? 1 : 1);
			break;
		case PngColorType::RGB:
			bytesPerPixel = (bitDepth == 16) ? 6 : ((bitDepth >= 8) ? 3 : 1);
			break;
		case PngColorType::PALETTE:
			bytesPerPixel = 1; // 调色板总是1字节索引，但实际位深度可能小于8
			break;
		case PngColorType::GERY_ALPHA:
			bytesPerPixel = (bitDepth == 16) ? 4 : ((bitDepth >= 8) ? 2 : 1);
			break;
		case PngColorType::RGBA:
			bytesPerPixel = (bitDepth == 16) ? 8 : ((bitDepth >= 8) ? 4 : 1);
			break;
		default:
			return std::unexpected("不支持的颜色类型");
		}
		
		// 3. 计算扫描线宽度（包括滤波器字节）
		// 对于低位深度，需要按位计算；对于8位及以上，按字节计算
		size_t scanlineWidth = 0;
		if (bitDepth >= 8)
		{
			scanlineWidth = _width * bytesPerPixel;
		}
		else
		{
			// 对于低位深度，扫描线宽度 = ceil(width * bitDepth / 8)
			// PNG 规范：扫描线必须向上取整到字节边界
			scanlineWidth = ((_width * bitDepth) + 7) / 8;
		}
		
		// 4. 处理交织（Adam7 或非交织）
		std::vector<uint8_t> unfilteredData;
		
		// 优化：预分配内存（考虑可能的填充位）
		size_t estimatedSize = _height * scanlineWidth;
		if (bitDepth < 8)
		{
			// 低位深度可能需要额外的填充位空间
			size_t actualBitsPerLine = _width * bitDepth;
			size_t paddedBitsPerLine = ((actualBitsPerLine + 7) / 8) * 8;
			estimatedSize = _height * ((paddedBitsPerLine + 7) / 8);
		}
		unfilteredData.reserve(estimatedSize);
		
		if (interlaceMethod == 1)
		{
			// Adam7 交织解码
			auto adam7Result = decodeAdam7(decompressed, bytesPerPixel, scanlineWidth);
			if (!adam7Result.has_value())
			{
				return std::unexpected("Adam7 交织解码失败: " + adam7Result.error());
			}
			unfilteredData = std::move(adam7Result.value());
		}
		else
		{
			// 非交织：直接解滤波器
			const uint8_t* prevline = nullptr;
			for (size_t y = 0; y < _height; ++y)
			{
				size_t offset = y * (scanlineWidth + 1);
				if (offset >= decompressed.size())
				{
					return std::unexpected(fmt::format("解压缩数据大小不足: 需要至少 {} 字节，但只有 {} 字节", 
						offset + scanlineWidth + 1, decompressed.size()));
				}
				
				uint8_t filterType = decompressed[offset];
				if (filterType > 4)
				{
					return std::unexpected(fmt::format("无效的滤波器类型: {} (第 {} 行)", filterType, y));
				}
				
				if (offset + 1 + scanlineWidth > decompressed.size())
				{
					return std::unexpected(fmt::format("扫描线数据不足: 第 {} 行需要 {} 字节，但只有 {} 字节", 
						y, scanlineWidth, decompressed.size() - offset - 1));
				}
				
				std::span<const uint8_t> scanline(decompressed.data() + offset + 1, scanlineWidth);
				
				size_t reconOffset = unfilteredData.size();
				unfilteredData.resize(unfilteredData.size() + scanlineWidth);
				std::span<uint8_t> recon(unfilteredData.data() + reconOffset, scanlineWidth);
				
				unfilterScanline(recon, scanline, prevline, bytesPerPixel, filterType);
				prevline = recon.data();
			}
			
			// 对于低位深度，移除填充位
			if (bitDepth < 8 && bitDepth > 0)
			{
				size_t actualBitsPerLine = _width * bitDepth;
				size_t paddedBitsPerLine = scanlineWidth * 8;
				if (paddedBitsPerLine != actualBitsPerLine)
				{
					unfilteredData = removePaddingBits(unfilteredData, actualBitsPerLine, paddedBitsPerLine);
				}
			}
		}
		
		// 5. 转换为 RGBA
		convertToRGBA(unfilteredData, imageData);
		
		return {};
	}

	std::expected<std::vector<uint8_t>, std::string> png::decodeRGB() const
	{
		// 先解码为 RGBA，然后转换为 RGB
		auto result = const_cast<png*>(this)->decodeInternal();
		if (!result.has_value())
		{
			return std::unexpected(result.error());
		}
		
		std::vector<uint8_t> rgbData;
		rgbData.reserve(_width * _height * 3);
		
		for (size_t i = 0; i < imageData.size(); i += 4)
		{
			rgbData.push_back(imageData[i]);     // R
			rgbData.push_back(imageData[i + 1]); // G
			rgbData.push_back(imageData[i + 2]); // B
		}
		
		return rgbData;
	}
	
	// ========================================================================
	// Deflate/Zlib 解压缩实现
	// ========================================================================
	
	// Deflate 常量定义
	namespace {
		constexpr uint32_t FIRST_LENGTH_CODE_INDEX = 257;
		constexpr uint32_t LAST_LENGTH_CODE_INDEX = 285;
		constexpr uint32_t NUM_DEFLATE_CODE_SYMBOLS = 288; // 256 literals + end code + length codes
		constexpr uint32_t NUM_DISTANCE_SYMBOLS = 32;
		constexpr uint32_t NUM_CODE_LENGTH_CODES = 19;
		constexpr uint32_t FIRSTBITS = 9; // 用于 Huffman 查找表的根位数
		constexpr uint32_t INVALIDSYMBOL = 65535;
		
		// 长度码的基础值（代码 257-285）
		constexpr uint16_t LENGTHBASE[29] = {
			3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
			67, 83, 99, 115, 131, 163, 195, 227, 258
		};
		
		// 长度码的额外位数
		constexpr uint8_t LENGTHEXTRA[29] = {
			0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
			4, 4, 4, 4, 5, 5, 5, 5, 0
		};
		
		// 距离码的基础值
		constexpr uint16_t DISTANCEBASE[30] = {
			1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
			769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
		};
		
		// 距离码的额外位数
		constexpr uint8_t DISTANCEEXTRA[30] = {
			0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
			8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
		};
		
		// 代码长度码的顺序
		constexpr uint8_t CLCL_ORDER[NUM_CODE_LENGTH_CODES] = {
			16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
		};
		
		// 反转位顺序（用于 Huffman 解码）
		constexpr uint32_t reverseBits(uint32_t bits, uint32_t num) noexcept
		{
			uint32_t result = 0;
			for (uint32_t i = 0; i < num; ++i)
			{
				result |= ((bits >> (num - i - 1)) & 1) << i;
			}
			return result;
		}
	}
	
	// Huffman 树结构
	struct HuffmanTree
	{
		std::vector<uint32_t> codes;      // Huffman 码
		std::vector<uint32_t> lengths;    // 码长度
		std::vector<uint8_t> table_len;   // 查找表：长度
		std::vector<uint16_t> table_value; // 查找表：值
		uint32_t maxbitlen = 0;
		uint32_t numcodes = 0;
		
		void init(uint32_t numcodes_val, uint32_t maxbitlen_val)
		{
			numcodes = numcodes_val;
			maxbitlen = maxbitlen_val;
			codes.clear();
			lengths.clear();
			table_len.clear();
			table_value.clear();
		}
	};
	
	// 从码长度构建 Huffman 树
	static std::expected<HuffmanTree, std::string> buildHuffmanTree(
		const uint32_t* bitlen, size_t numcodes, uint32_t maxbitlen)
	{
		HuffmanTree tree;
		tree.init(static_cast<uint32_t>(numcodes), maxbitlen);
		
		// 分配空间
		tree.codes.resize(numcodes);
		tree.lengths.resize(numcodes);
		
		// 复制长度
		for (size_t i = 0; i < numcodes; ++i)
		{
			tree.lengths[i] = bitlen[i];
		}
		
		// 步骤1：统计每个长度的数量
		std::vector<uint32_t> blcount(maxbitlen + 1, 0);
		std::vector<uint32_t> nextcode(maxbitlen + 1, 0);
		
		for (size_t i = 0; i < numcodes; ++i)
		{
			if (tree.lengths[i] <= maxbitlen)
			{
				++blcount[tree.lengths[i]];
			}
		}
		
		// 步骤2：生成下一个码值
		uint32_t code = 0;
		for (uint32_t bits = 1; bits <= maxbitlen; ++bits)
		{
			code = (code + blcount[bits - 1]) << 1;
			nextcode[bits] = code;
		}
		
		// 步骤3：生成所有码
		for (size_t n = 0; n < numcodes; ++n)
		{
			if (tree.lengths[n] != 0)
			{
				tree.codes[n] = nextcode[tree.lengths[n]]++;
				tree.codes[n] &= ((1u << tree.lengths[n]) - 1u);
			}
		}
		
		// 步骤4：构建查找表
		constexpr uint32_t headsize = 1u << FIRSTBITS;
		constexpr uint32_t mask = headsize - 1u;
		
		// 计算最大长度
		std::vector<uint32_t> maxlens(headsize, 0);
		for (size_t i = 0; i < numcodes; ++i)
		{
			uint32_t symbol = tree.codes[i];
			uint32_t l = tree.lengths[i];
			if (l <= FIRSTBITS) continue;
			
			uint32_t index = reverseBits(symbol >> (l - FIRSTBITS), FIRSTBITS);
			if (index < headsize)
			{
				maxlens[index] = std::max(maxlens[index], l);
			}
		}
		
		// 计算总表大小
		size_t size = headsize;
		for (uint32_t i = 0; i < headsize; ++i)
		{
			uint32_t l = maxlens[i];
			if (l > FIRSTBITS)
			{
				size += (1u << (l - FIRSTBITS));
			}
		}
		
		tree.table_len.resize(size, 16); // 16 表示未使用
		tree.table_value.resize(size, INVALIDSYMBOL);
		
		// 填充第一层表（长符号）
		size_t pointer = headsize;
		for (uint32_t i = 0; i < headsize; ++i)
		{
			uint32_t l = maxlens[i];
			if (l > FIRSTBITS)
			{
				tree.table_len[i] = static_cast<uint8_t>(l);
				tree.table_value[i] = static_cast<uint16_t>(pointer);
				pointer += (1u << (l - FIRSTBITS));
			}
		}
		
		// 填充符号
		for (size_t i = 0; i < numcodes; ++i)
		{
			uint32_t l = tree.lengths[i];
			if (l == 0) continue;
			
			uint32_t symbol = tree.codes[i];
			uint32_t reverse = reverseBits(symbol, l);
			
			if (l <= FIRSTBITS)
			{
				// 短符号，完全在第一层表中
				uint32_t num = 1u << (FIRSTBITS - l);
				for (uint32_t j = 0; j < num; ++j)
				{
					uint32_t index = reverse | (j << l);
					if (index < headsize && tree.table_len[index] == 16)
					{
						tree.table_len[index] = static_cast<uint8_t>(l);
						tree.table_value[index] = static_cast<uint16_t>(i);
					}
				}
			}
			else
			{
				// 长符号，需要第二层查找
				uint32_t index = reverse & mask;
				if (index < headsize)
				{
					uint32_t maxlen = tree.table_len[index];
					if (maxlen >= l)
					{
						uint32_t tablelen = maxlen - FIRSTBITS;
						uint32_t start = tree.table_value[index];
						uint32_t num = 1u << (tablelen - (l - FIRSTBITS));
						
						for (uint32_t j = 0; j < num; ++j)
						{
							uint32_t reverse2 = reverse >> FIRSTBITS;
							uint32_t index2 = start + (reverse2 | (j << (l - FIRSTBITS)));
							if (index2 < size)
							{
								tree.table_len[index2] = static_cast<uint8_t>(l);
								tree.table_value[index2] = static_cast<uint16_t>(i);
							}
						}
					}
				}
			}
		}
		
		// 填充无效符号
		for (size_t i = 0; i < size; ++i)
		{
			if (tree.table_len[i] == 16)
			{
				tree.table_len[i] = (i < headsize) ? 1 : (FIRSTBITS + 1);
				tree.table_value[i] = INVALIDSYMBOL;
			}
		}
		
		return tree;
	}
	
	// 从固定树构建 Huffman 树（BTYPE=1）
	static std::expected<std::pair<HuffmanTree, HuffmanTree>, std::string> buildFixedHuffmanTrees()
	{
		// 构建 literal/length 树
		std::vector<uint32_t> bitlen_ll(NUM_DEFLATE_CODE_SYMBOLS);
		for (uint32_t i = 0; i <= 143; ++i) bitlen_ll[i] = 8;
		for (uint32_t i = 144; i <= 255; ++i) bitlen_ll[i] = 9;
		for (uint32_t i = 256; i <= 279; ++i) bitlen_ll[i] = 7;
		for (uint32_t i = 280; i <= 287; ++i) bitlen_ll[i] = 8;
		
		auto tree_ll = buildHuffmanTree(bitlen_ll.data(), NUM_DEFLATE_CODE_SYMBOLS, 15);
		if (!tree_ll.has_value())
		{
			return std::unexpected("构建固定 literal/length 树失败: " + tree_ll.error());
		}
		
		// 构建距离树
		std::vector<uint32_t> bitlen_d(NUM_DISTANCE_SYMBOLS, 5);
		auto tree_d = buildHuffmanTree(bitlen_d.data(), NUM_DISTANCE_SYMBOLS, 15);
		if (!tree_d.has_value())
		{
			return std::unexpected("构建固定距离树失败: " + tree_d.error());
		}
		
		return std::make_pair(std::move(tree_ll.value()), std::move(tree_d.value()));
	}
	
	// Huffman 解码符号
	static uint32_t huffmanDecodeSymbol(util::BitReader& reader, const HuffmanTree& tree)
	{
		uint32_t code = reader.peekBits(FIRSTBITS);
		if (code >= tree.table_len.size())
		{
			return INVALIDSYMBOL;
		}
		
		uint8_t l = tree.table_len[code];
		uint16_t value = tree.table_value[code];
		
		if (l <= FIRSTBITS)
		{
			reader.advanceBits(l);
			return value;
		}
		else
		{
			reader.advanceBits(FIRSTBITS);
			uint32_t code2 = reader.peekBits(l - FIRSTBITS);
			uint32_t index2 = value + code2;
			
			if (index2 >= tree.table_len.size())
			{
				return INVALIDSYMBOL;
			}
			
			uint8_t l2 = tree.table_len[index2];
			reader.advanceBits(l2 - FIRSTBITS);
			return tree.table_value[index2];
		}
	}
	
	// 解压缩 Huffman 块（BTYPE=1 或 2）
	static std::expected<std::vector<uint8_t>, std::string> inflateHuffmanBlock(
		util::BitReader& reader, uint32_t btype, size_t max_output_size = 0)
	{
		HuffmanTree tree_ll, tree_d;
		
		// 构建 Huffman 树
		if (btype == 1)
		{
			// 固定树
			auto trees = buildFixedHuffmanTrees();
			if (!trees.has_value())
			{
				return std::unexpected(trees.error());
			}
			tree_ll = std::move(trees->first);
			tree_d = std::move(trees->second);
		}
		else // btype == 2
		{
			// 动态树
			reader.ensureBits(17);
			
			// 读取 HLIT, HDIST, HCLEN
			uint32_t HLIT = reader.readBits(5) + 257;
			uint32_t HDIST = reader.readBits(5) + 1;
			uint32_t HCLEN = reader.readBits(4) + 4;
			
			if (HLIT > 286 || HDIST > 30)
			{
				return std::unexpected("无效的 HLIT 或 HDIST");
			}
			
			// 读取代码长度码的长度
			std::vector<uint32_t> bitlen_cl(NUM_CODE_LENGTH_CODES, 0);
			for (uint32_t i = 0; i < HCLEN; ++i)
			{
				reader.ensureBits(3);
				uint32_t order = CLCL_ORDER[i];
				if (order >= NUM_CODE_LENGTH_CODES)
				{
					return std::unexpected("无效的代码长度码顺序");
				}
				bitlen_cl[order] = reader.readBits(3);
			}
			
			// 构建代码长度码的 Huffman 树
			auto tree_cl_result = buildHuffmanTree(bitlen_cl.data(), NUM_CODE_LENGTH_CODES, 7);
			if (!tree_cl_result.has_value())
			{
				return std::unexpected("构建代码长度码树失败: " + tree_cl_result.error());
			}
			HuffmanTree tree_cl = std::move(tree_cl_result.value());
			
			// 解码 literal/length 和 distance 的码长度
			std::vector<uint32_t> bitlen_ll(HLIT, 0);
			std::vector<uint32_t> bitlen_d(HDIST, 0);
			
			uint32_t i = 0;
			while (i < HLIT + HDIST)
			{
				reader.ensureBits(25);
				uint32_t code = huffmanDecodeSymbol(reader, tree_cl);
				
				if (code == INVALIDSYMBOL)
				{
					return std::unexpected("解码代码长度码失败");
				}
				
				uint32_t repeat = 0;
				uint32_t value = 0;
				
				if (code <= 15)
				{
					// 直接使用码长度
					value = code;
					repeat = 1;
				}
				else if (code == 16)
				{
					// 重复前一个码长度 3-6 次
					reader.ensureBits(2);
					repeat = reader.readBits(2) + 3;
					if (i == 0)
					{
						return std::unexpected("代码长度码 16 不能是第一个");
					}
					value = (i <= HLIT) ? bitlen_ll[i - 1] : bitlen_d[i - HLIT - 1];
				}
				else if (code == 17)
				{
					// 重复 0 码长度 3-10 次
					reader.ensureBits(3);
					repeat = reader.readBits(3) + 3;
					value = 0;
				}
				else if (code == 18)
				{
					// 重复 0 码长度 11-138 次
					reader.ensureBits(7);
					repeat = reader.readBits(7) + 11;
					value = 0;
				}
				else
				{
					return std::unexpected("无效的代码长度码: " + std::to_string(code));
				}
				
				// 填充码长度
				for (uint32_t j = 0; j < repeat && i < HLIT + HDIST; ++j, ++i)
				{
					if (i < HLIT)
					{
						bitlen_ll[i] = value;
					}
					else
					{
						bitlen_d[i - HLIT] = value;
					}
				}
			}
			
			// 检查结束码必须存在
			if (bitlen_ll.size() <= 256 || bitlen_ll[256] == 0)
			{
				return std::unexpected("结束码 256 的长度必须大于 0");
			}
			
			// 构建 literal/length 和 distance 树
			auto tree_ll_result = buildHuffmanTree(bitlen_ll.data(), HLIT, 15);
			if (!tree_ll_result.has_value())
			{
				return std::unexpected("构建 literal/length 树失败: " + tree_ll_result.error());
			}
			tree_ll = std::move(tree_ll_result.value());
			
			auto tree_d_result = buildHuffmanTree(bitlen_d.data(), HDIST, 15);
			if (!tree_d_result.has_value())
			{
				return std::unexpected("构建距离树失败: " + tree_d_result.error());
			}
			tree_d = std::move(tree_d_result.value());
		}
		
		std::vector<uint8_t> output;
		output.reserve(1024);
		
		bool done = false;
		while (!done)
		{
			// 确保有足够的位
			reader.ensureBits(30);
			
			// 解码符号
			uint32_t code_ll = huffmanDecodeSymbol(reader, tree_ll);
			
			if (code_ll == INVALIDSYMBOL)
			{
				return std::unexpected("无效的 Huffman 符号");
			}
			
			if (code_ll <= 255)
			{
				// 字面量
				output.push_back(static_cast<uint8_t>(code_ll));
			}
			else if (code_ll >= FIRST_LENGTH_CODE_INDEX && code_ll <= LAST_LENGTH_CODE_INDEX)
			{
				// 长度/距离对
				uint32_t length = LENGTHBASE[code_ll - FIRST_LENGTH_CODE_INDEX];
				uint8_t numextrabits_l = LENGTHEXTRA[code_ll - FIRST_LENGTH_CODE_INDEX];
				
				if (numextrabits_l > 0)
				{
					reader.ensureBits(25);
					length += reader.readBits(numextrabits_l);
				}
				
				// 解码距离码
				reader.ensureBits(28);
				uint32_t code_d = huffmanDecodeSymbol(reader, tree_d);
				
				if (code_d == INVALIDSYMBOL || code_d > 29)
				{
					return std::unexpected("无效的距离码");
				}
				
				uint32_t distance = DISTANCEBASE[code_d];
				uint8_t numextrabits_d = DISTANCEEXTRA[code_d];
				
				if (numextrabits_d > 0)
				{
					distance += reader.readBits(numextrabits_d);
				}
				
				// LZ77 解码：从输出缓冲区复制
				if (distance > output.size())
				{
					return std::unexpected("距离超出输出缓冲区");
				}
				
				size_t start = output.size();
				size_t backward = start - distance;
				output.resize(start + length);
				
				if (distance < length)
				{
					// 需要重复复制
					std::memcpy(output.data() + start, output.data() + backward, distance);
					size_t forward = distance;
					while (forward < length)
					{
						size_t copyLen = std::min(distance, length - forward);
						std::memcpy(output.data() + start + forward, 
						           output.data() + backward, copyLen);
						forward += copyLen;
					}
				}
				else
				{
					std::memcpy(output.data() + start, output.data() + backward, length);
				}
			}
			else if (code_ll == 256)
			{
				// 结束码
				done = true;
			}
			else
			{
				return std::unexpected("无效的 Huffman 码");
			}
			
			if (max_output_size > 0 && output.size() > max_output_size)
			{
				return std::unexpected("输出大小超出限制");
			}
		}
		
		return output;
	}
	
	// Zlib 解压缩实现（完整版，支持 Deflate）
	std::expected<std::vector<uint8_t>, std::string> png::zlibDecompress(std::span<const uint8_t> compressed) const
	{
		if (compressed.size() < 2)
		{
			return std::unexpected("Zlib 数据太小");
		}
		
		// 使用 byte_convert 工具读取 zlib 头
		std::span<const std::byte> compressedSpan(reinterpret_cast<const std::byte*>(compressed.data()), compressed.size());
		uint16_t header = util::read_u16(compressedSpan, 0);
		
		if ((header % 31) != 0)
		{
			return std::unexpected("无效的 Zlib 头");
		}
		
		uint8_t cm = util::read_u8(compressedSpan, 0) & 0x0F;
		uint8_t cinfo = (util::read_u8(compressedSpan, 0) >> 4) & 0x0F;
		uint8_t fdict = (util::read_u8(compressedSpan, 1) >> 5) & 0x01;
		
		if (cm != 8 || cinfo > 7)
		{
			return std::unexpected("不支持的压缩方法");
		}
		
		if (fdict != 0)
		{
			return std::unexpected("不支持预设字典");
		}
		
		// 跳过 zlib 头，开始 deflate 数据
		std::span<const uint8_t> deflateData = compressed.subspan(2);
		if (deflateData.size() < 4) // 至少需要 Adler-32
		{
			return std::unexpected("Deflate 数据太小");
		}
		
		// 创建位读取器
		util::BitReader reader(deflateData.data(), deflateData.size() - 4); // 保留最后4字节用于 Adler-32
		
		std::vector<uint8_t> output;
		output.reserve(1024); // 预分配一些空间
		
		// 解压缩 deflate 块
		bool bfinal = false;
		while (!bfinal)
		{
			if (reader.getBitPos() + 3 > reader.getBytePos() * 8 + (deflateData.size() - 4) * 8)
			{
				return std::unexpected("位指针超出范围");
			}
			
			// 读取 BFINAL (1 bit) 和 BTYPE (2 bits)
			bfinal = reader.readBits(1) != 0;
			uint32_t btype = reader.readBits(2);
			
			if (btype == 3)
			{
				return std::unexpected("无效的 BTYPE");
			}
			
			// 处理不同类型的块
			if (btype == 0)
			{
				// 无压缩块
				reader.alignToByte();
				size_t bytePos = reader.getBytePos();
				
				if (bytePos + 4 > deflateData.size() - 4)
				{
					return std::unexpected("无压缩块数据不足");
				}
				
				// 使用 byte_convert 工具读取长度
				std::span<const std::byte> deflateSpan(reinterpret_cast<const std::byte*>(deflateData.data()), deflateData.size());
				uint16_t len = util::read_u16(deflateSpan, bytePos);
				uint16_t nlen = util::read_u16(deflateSpan, bytePos + 2);
				
				if ((len + nlen) != 0xFFFF)
				{
					return std::unexpected("无压缩块长度校验失败");
				}
				
				bytePos += 4;
				if (bytePos + len > deflateData.size() - 4)
				{
					return std::unexpected("无压缩块数据超出范围");
				}
				
				size_t oldSize = output.size();
				output.resize(oldSize + len);
				std::memcpy(output.data() + oldSize, deflateData.data() + bytePos, len);
				
				reader.advanceBits(len * 8);
			}
			else if (btype == 1 || btype == 2)
			{
				// BTYPE=1 (固定 Huffman) 或 BTYPE=2 (动态 Huffman)
				auto blockResult = inflateHuffmanBlock(reader, btype);
				if (!blockResult.has_value())
				{
					return std::unexpected("Huffman 块解码失败: " + blockResult.error());
				}
				
				// 追加到输出
				size_t oldSize = output.size();
				output.resize(oldSize + blockResult->size());
				std::memcpy(output.data() + oldSize, blockResult->data(), blockResult->size());
			}
			else
			{
				return std::unexpected("不支持的 BTYPE: " + std::to_string(btype));
			}
		}
		
		// 验证 Adler-32 校验和
		if (deflateData.size() >= 4)
		{
			uint32_t expectedAdler = util::read_u32(compressedSpan, compressed.size() - 4);
			uint32_t calculatedAdler = calculateAdler32(output.data(), output.size());
			
			if (expectedAdler != calculatedAdler)
			{
				return std::unexpected("Adler-32 校验失败: 期望=" + 
					std::to_string(expectedAdler) + ", 计算=" + std::to_string(calculatedAdler));
			}
		}
		
		if (output.empty())
		{
			return std::unexpected("解压缩后数据为空");
		}
		
		return output;
	}
	
	// PNG 滤波器处理
	void png::unfilterScanline(std::span<uint8_t> recon, std::span<const uint8_t> scanline,
	                            const uint8_t* prevline, size_t bytewidth, uint8_t filterType) const
	{
		const size_t length = recon.size();
		
		switch (filterType)
		{
		case 0: // None
			std::memcpy(recon.data(), scanline.data(), length);
			break;
			
		case 1: // Sub
		{
			for (size_t i = 0; i < bytewidth && i < length; ++i)
			{
				recon[i] = scanline[i];
			}
			for (size_t i = bytewidth; i < length; ++i)
			{
				recon[i] = scanline[i] + recon[i - bytewidth];
			}
			break;
		}
		
		case 2: // Up
		{
			if (prevline)
			{
				for (size_t i = 0; i < length; ++i)
				{
					recon[i] = scanline[i] + prevline[i];
				}
			}
			else
			{
				std::memcpy(recon.data(), scanline.data(), length);
			}
			break;
		}
		
		case 3: // Average
		{
			if (prevline)
			{
				for (size_t i = 0; i < bytewidth && i < length; ++i)
				{
					recon[i] = scanline[i] + (prevline[i] / 2);
				}
				for (size_t i = bytewidth; i < length; ++i)
				{
					recon[i] = scanline[i] + ((recon[i - bytewidth] + prevline[i]) / 2);
				}
			}
			else
			{
				for (size_t i = 0; i < bytewidth && i < length; ++i)
				{
					recon[i] = scanline[i];
				}
				for (size_t i = bytewidth; i < length; ++i)
				{
					recon[i] = scanline[i] + (recon[i - bytewidth] / 2);
				}
			}
			break;
		}
		
		case 4: // Paeth
		{
			if (prevline)
			{
				for (size_t i = 0; i < bytewidth && i < length; ++i)
				{
					recon[i] = scanline[i] + prevline[i];
				}
				for (size_t i = bytewidth; i < length; ++i)
				{
					recon[i] = scanline[i] + paethPredictor(recon[i - bytewidth], prevline[i], prevline[i - bytewidth]);
				}
			}
			else
			{
				for (size_t i = 0; i < bytewidth && i < length; ++i)
				{
					recon[i] = scanline[i];
				}
				for (size_t i = bytewidth; i < length; ++i)
				{
					recon[i] = scanline[i] + recon[i - bytewidth];
				}
			}
			break;
		}
		
		default:
			// 未知滤波器类型，复制原始数据
			std::memcpy(recon.data(), scanline.data(), length);
			break;
		}
	}
	
	// Adam7 交织解码
	std::expected<std::vector<uint8_t>, std::string> png::decodeAdam7(
		std::span<const uint8_t> decompressed, size_t bytesPerPixel, size_t scanlineWidth) const
	{
		// Adam7 常量定义
		constexpr uint32_t ADAM7_IX[7] = { 0, 4, 0, 2, 0, 1, 0 }; // x 起始位置
		constexpr uint32_t ADAM7_IY[7] = { 0, 0, 4, 0, 2, 0, 1 }; // y 起始位置
		constexpr uint32_t ADAM7_DX[7] = { 8, 8, 4, 4, 2, 2, 1 }; // x 步长
		constexpr uint32_t ADAM7_DY[7] = { 8, 8, 8, 4, 4, 2, 2 }; // y 步长
		
		// 计算每个通道的宽度和高度
		uint32_t passw[7], passh[7];
		size_t filter_passstart[8] = {0}; // 每个通道在解压缩数据中的起始位置
		
		for (int i = 0; i < 7; ++i)
		{
			passw[i] = (_width + ADAM7_DX[i] - ADAM7_IX[i] - 1) / ADAM7_DX[i];
			passh[i] = (_height + ADAM7_DY[i] - ADAM7_IY[i] - 1) / ADAM7_DY[i];
			
			// 计算每个通道的扫描线宽度
			size_t passScanlineWidth = 0;
			if (bitDepth >= 8)
			{
				passScanlineWidth = passw[i] * bytesPerPixel;
			}
			else
			{
				passScanlineWidth = ((passw[i] * bitDepth) + 7) / 8;
			}
			
			// 计算下一个通道的起始位置
			if (i < 6)
			{
				filter_passstart[i + 1] = filter_passstart[i];
				for (uint32_t y = 0; y < passh[i]; ++y)
				{
					filter_passstart[i + 1] += passScanlineWidth + 1; // +1 是滤波器字节
				}
			}
		}
		
		// 创建输出缓冲区（非交织数据）
		std::vector<uint8_t> output(_height * scanlineWidth, 0);
		
		// 处理每个通道
		for (int pass = 0; pass < 7; ++pass)
		{
			if (passw[pass] == 0 || passh[pass] == 0)
				continue; // 跳过空通道
			
			// 计算通道的扫描线宽度
			size_t passScanlineWidth = 0;
			if (bitDepth >= 8)
			{
				passScanlineWidth = passw[pass] * bytesPerPixel;
			}
			else
			{
				passScanlineWidth = ((passw[pass] * bitDepth) + 7) / 8;
			}
			
			// 解滤波器这个通道的数据
			std::vector<uint8_t> passData(passh[pass] * passScanlineWidth);
			const uint8_t* prevline = nullptr;
			size_t dataOffset = filter_passstart[pass];
			
			for (uint32_t y = 0; y < passh[pass]; ++y)
			{
				if (dataOffset >= decompressed.size())
				{
					return std::unexpected("Adam7 通道 " + std::to_string(pass) + " 数据不足");
				}
				
				uint8_t filterType = decompressed[dataOffset];
				if (dataOffset + 1 + passScanlineWidth > decompressed.size())
				{
					return std::unexpected("Adam7 通道 " + std::to_string(pass) + " 扫描线数据不足");
				}
				
				std::span<const uint8_t> scanline(decompressed.data() + dataOffset + 1, passScanlineWidth);
				
				size_t reconOffset = y * passScanlineWidth;
				std::span<uint8_t> recon(passData.data() + reconOffset, passScanlineWidth);
				
				unfilterScanline(recon, scanline, prevline, bytesPerPixel, filterType);
				prevline = recon.data();
				
				dataOffset += passScanlineWidth + 1;
			}
			
			// 将通道数据重组到最终图像中
			if (bitDepth >= 8)
			{
				// 字节对齐的数据
				for (uint32_t y = 0; y < passh[pass]; ++y)
				{
					for (uint32_t x = 0; x < passw[pass]; ++x)
					{
						uint32_t srcX = x;
						uint32_t srcY = y;
						uint32_t dstX = ADAM7_IX[pass] + x * ADAM7_DX[pass];
						uint32_t dstY = ADAM7_IY[pass] + y * ADAM7_DY[pass];
						
						if (dstX < _width && dstY < _height)
						{
							size_t srcOffset = (srcY * passScanlineWidth + srcX * bytesPerPixel);
							size_t dstOffset = (dstY * scanlineWidth + dstX * bytesPerPixel);
							
							if (srcOffset + bytesPerPixel <= passData.size() && 
							    dstOffset + bytesPerPixel <= output.size())
							{
								std::memcpy(output.data() + dstOffset, 
								           passData.data() + srcOffset, bytesPerPixel);
							}
						}
					}
				}
			}
			else
			{
				// 位级别的数据（低位深度）- 实现 Adam7 位级别重组
				size_t actualBitsPerLine = passw[pass] * bitDepth;
				size_t paddedBitsPerLine = passScanlineWidth * 8;
				
				// 移除填充位（如果存在）
				std::vector<uint8_t> passDataNoPadding;
				if (paddedBitsPerLine != actualBitsPerLine)
				{
					passDataNoPadding = removePaddingBits(passData, actualBitsPerLine, paddedBitsPerLine);
				}
				else
				{
					passDataNoPadding = passData;
				}
				
				// 将通道数据重组到最终图像中（位级别）
				size_t outputBitsPerLine = _width * bitDepth;
				size_t outputBytesPerLine = (outputBitsPerLine + 7) / 8;
				
				// 确保输出缓冲区足够大
				if (output.size() < _height * outputBytesPerLine)
				{
					output.resize(_height * outputBytesPerLine, 0);
				}
				
				for (uint32_t y = 0; y < passh[pass]; ++y)
				{
					for (uint32_t x = 0; x < passw[pass]; ++x)
					{
						uint32_t dstX = ADAM7_IX[pass] + x * ADAM7_DX[pass];
						uint32_t dstY = ADAM7_IY[pass] + y * ADAM7_DY[pass];
						
						if (dstX < _width && dstY < _height)
						{
							// 读取源像素位
							size_t srcBitPos = (y * actualBitsPerLine + x * bitDepth);
							
							// 写入目标像素位
							size_t dstBitPos = (dstY * outputBitsPerLine + dstX * bitDepth);
							
							// 复制位（需要临时变量，因为 readBitFromStream 会修改 bitPos）
							size_t tempSrcBitPos = srcBitPos;
							size_t tempDstBitPos = dstBitPos;
							for (uint32_t b = 0; b < bitDepth; ++b)
							{
								uint8_t bit = readBitFromStream(tempSrcBitPos, passDataNoPadding.data());
								writeBitToStream(tempDstBitPos, output.data(), bit);
							}
						}
					}
				}
			}
		}
		
		return output;
	}
	
	// 从位流读取位（MSB优先，用于低位深度）
	uint8_t png::readBitFromStream(size_t& bitPos, const uint8_t* data) noexcept
	{
		size_t bytePos = bitPos / 8;
		size_t bitInByte = 7 - (bitPos % 8); // MSB优先
		uint8_t bit = (data[bytePos] >> bitInByte) & 1;
		++bitPos;
		return bit;
	}
	
	// 写入位到位流（MSB优先，用于低位深度）
	void png::writeBitToStream(size_t& bitPos, uint8_t* data, uint8_t bit) noexcept
	{
		size_t bytePos = bitPos / 8;
		size_t bitInByte = 7 - (bitPos % 8); // MSB优先
		if (bit)
		{
			data[bytePos] |= (1 << bitInByte);
		}
		else
		{
			data[bytePos] &= ~(1 << bitInByte);
		}
		++bitPos;
	}
	
	// 移除填充位（用于低位深度）
	std::vector<uint8_t> png::removePaddingBits(
		const std::vector<uint8_t>& paddedData, size_t actualBitsPerLine, size_t paddedBitsPerLine) const
	{
		if (actualBitsPerLine == paddedBitsPerLine)
		{
			return paddedData; // 没有填充位
		}
		
		size_t diff = paddedBitsPerLine - actualBitsPerLine;
		size_t outputBits = _height * actualBitsPerLine;
		size_t outputBytes = (outputBits + 7) / 8;
		
		std::vector<uint8_t> output(outputBytes, 0);
		size_t inputBitPos = 0;
		size_t outputBitPos = 0;
		
		for (size_t y = 0; y < _height; ++y)
		{
			// 复制实际数据位
			for (size_t x = 0; x < actualBitsPerLine; ++x)
			{
				uint8_t bit = readBitFromStream(inputBitPos, paddedData.data());
				writeBitToStream(outputBitPos, output.data(), bit);
			}
			// 跳过填充位
			inputBitPos += diff;
		}
		
		return output;
	}
	
	// 颜色转换到 RGBA
	void png::convertToRGBA(std::span<const uint8_t> rawData, std::vector<uint8_t>& output) const
	{
		output.clear();
		output.reserve(_width * _height * 4);
		
		if (bitDepth == 8)
		{
			switch (colorType)
			{
			case PngColorType::GERY:
				for (size_t i = 0; i < rawData.size(); ++i)
				{
					uint8_t gray = rawData[i];
					output.push_back(gray);
					output.push_back(gray);
					output.push_back(gray);
					output.push_back(255);
				}
				break;
				
			case PngColorType::RGB:
				for (size_t i = 0; i < rawData.size(); i += 3)
				{
					output.push_back(rawData[i]);
					output.push_back(rawData[i + 1]);
					output.push_back(rawData[i + 2]);
					output.push_back(255);
				}
				break;
				
			case PngColorType::PALETTE:
				for (size_t i = 0; i < rawData.size(); ++i)
				{
					uint8_t index = rawData[i];
					if (index < palettColors.size())
					{
						output.push_back(palettColors[index][0]);
						output.push_back(palettColors[index][1]);
						output.push_back(palettColors[index][2]);
						output.push_back(palettColors[index][3]);
					}
					else
					{
						output.push_back(0);
						output.push_back(0);
						output.push_back(0);
						output.push_back(255);
					}
				}
				break;
				
			case PngColorType::GERY_ALPHA:
				for (size_t i = 0; i < rawData.size(); i += 2)
				{
					uint8_t gray = rawData[i];
					uint8_t alpha = rawData[i + 1];
					output.push_back(gray);
					output.push_back(gray);
					output.push_back(gray);
					output.push_back(alpha);
				}
				break;
				
			case PngColorType::RGBA:
				output.assign(rawData.begin(), rawData.end());
				break;
				
			default:
				break;
			}
		}
		else if (bitDepth == 16)
		{
			// 16 位深度需要降采样到 8 位，PNG 使用大端序存储
			std::span<const std::byte> rawDataSpan(reinterpret_cast<const std::byte*>(rawData.data()), rawData.size());
			
			switch (colorType)
			{
			case PngColorType::GERY:
				for (size_t i = 0; i < rawData.size(); i += 2)
				{
					uint16_t gray16 = util::read_u16(rawDataSpan, i);
					uint8_t gray = static_cast<uint8_t>(gray16 >> 8); // 取高8位
					output.push_back(gray);
					output.push_back(gray);
					output.push_back(gray);
					output.push_back(255);
				}
				break;
				
			case PngColorType::RGB:
				for (size_t i = 0; i < rawData.size(); i += 6)
				{
					uint16_t r16 = util::read_u16(rawDataSpan, i);
					uint16_t g16 = util::read_u16(rawDataSpan, i + 2);
					uint16_t b16 = util::read_u16(rawDataSpan, i + 4);
					output.push_back(static_cast<uint8_t>(r16 >> 8));
					output.push_back(static_cast<uint8_t>(g16 >> 8));
					output.push_back(static_cast<uint8_t>(b16 >> 8));
					output.push_back(255);
				}
				break;
				
			case PngColorType::GERY_ALPHA:
				for (size_t i = 0; i < rawData.size(); i += 4)
				{
					uint16_t gray16 = util::read_u16(rawDataSpan, i);
					uint16_t alpha16 = util::read_u16(rawDataSpan, i + 2);
					uint8_t gray = static_cast<uint8_t>(gray16 >> 8);
					uint8_t alpha = static_cast<uint8_t>(alpha16 >> 8);
					output.push_back(gray);
					output.push_back(gray);
					output.push_back(gray);
					output.push_back(alpha);
				}
				break;
				
			case PngColorType::RGBA:
				for (size_t i = 0; i < rawData.size(); i += 8)
				{
					uint16_t r16 = util::read_u16(rawDataSpan, i);
					uint16_t g16 = util::read_u16(rawDataSpan, i + 2);
					uint16_t b16 = util::read_u16(rawDataSpan, i + 4);
					uint16_t a16 = util::read_u16(rawDataSpan, i + 6);
					output.push_back(static_cast<uint8_t>(r16 >> 8));
					output.push_back(static_cast<uint8_t>(g16 >> 8));
					output.push_back(static_cast<uint8_t>(b16 >> 8));
					output.push_back(static_cast<uint8_t>(a16 >> 8));
				}
				break;
				
			default:
				break;
			}
		}
		else if (bitDepth == 1 || bitDepth == 2 || bitDepth == 4)
		{
			// 低位深度处理：需要按位读取
			size_t bitsPerPixel = 0;
			size_t pixelsPerByte = 8 / bitDepth;
			
			switch (colorType)
			{
			case PngColorType::GERY:
				bitsPerPixel = bitDepth;
				for (size_t byteIdx = 0; byteIdx < rawData.size(); ++byteIdx)
				{
					uint8_t byte = rawData[byteIdx];
					for (size_t bitIdx = 0; bitIdx < pixelsPerByte && (byteIdx * pixelsPerByte + bitIdx) < (_width * _height); ++bitIdx)
					{
						// 从高位到低位读取
						uint8_t shift = 8 - (bitIdx + 1) * bitDepth;
						uint8_t mask = (1 << bitDepth) - 1;
						uint8_t gray = (byte >> shift) & mask;
						
						// 扩展到 8 位
						if (bitDepth == 1)
							gray = gray ? 255 : 0;
						else if (bitDepth == 2)
							gray = (gray << 6) | (gray << 4) | (gray << 2) | gray;
						else if (bitDepth == 4)
							gray = (gray << 4) | gray;
						
						output.push_back(gray);
						output.push_back(gray);
						output.push_back(gray);
						output.push_back(255);
					}
				}
				break;
				
			case PngColorType::PALETTE:
				bitsPerPixel = bitDepth;
				{
					size_t bitPos = 0;
					size_t pixelCount = _width * _height;
					
					for (size_t pixelIdx = 0; pixelIdx < pixelCount; ++pixelIdx)
					{
						// 从位流读取调色板索引（MSB优先）
						uint8_t index = 0;
						for (size_t b = 0; b < bitDepth; ++b)
						{
							index = (index << 1) | readBitFromStream(bitPos, rawData.data());
						}
						
						if (index < palettColors.size())
						{
							output.push_back(palettColors[index][0]);
							output.push_back(palettColors[index][1]);
							output.push_back(palettColors[index][2]);
							output.push_back(palettColors[index][3]);
						}
						else
						{
							output.push_back(0);
							output.push_back(0);
							output.push_back(0);
							output.push_back(255);
						}
					}
				}
				break;
				
			default:
				// 其他颜色类型不支持低位深度
				output.clear();
				break;
			}
		}
		else
		{
			output.clear();
		}
	}
}



