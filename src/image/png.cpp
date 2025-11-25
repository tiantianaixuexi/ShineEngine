#include "png.h"

#include <array>
#include <algorithm>
#include <string_view>
#include <string>
#include <expected>
#include <bit>

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
#include "util/encoding/huffman_tree.h"
#include "util/encoding/huffman_decoder.h"


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
			setState(shine::loader::EAssetLoadState::FAILD);
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
			setState(shine::loader::EAssetLoadState::FAILD);
			return false;
		}
		
		std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(data), size);
		
		// 验证PNG文件格式并解析IHDR块
		// IsPngFile会验证文件头并解析IHDR块数据
		if (!IsPngFile(dataSpan))
		{
			setError(shine::loader::EAssetLoaderError::INVALID_FORMAT);
			setState(shine::loader::EAssetLoadState::FAILD);
			return false;
		}
		
		// 验证IHDR解析结果
		if (_width == 0 || _height == 0)
		{
			setError(shine::loader::EAssetLoaderError::PARSE_ERROR, "IHDR块解析失败：图像尺寸无效");
			setState(shine::loader::EAssetLoadState::FAILD);
			return false;
		}
		
		// 保存原始数据
		rawPngData.assign(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size);
		
		setState(shine::loader::EAssetLoadState::PROCESSING);
		
		// 解析 PNG 块（类似 parsePngFile 的逻辑，但不从文件读取）
		// PNG规范要求IHDR必须是第一个块，位于文件头（8字节）之后
		// IHDR块结构：4字节长度(13) + 4字节类型("IHDR") + 13字节数据 + 4字节CRC = 25字节
		// 所以第一个块从偏移33开始（8字节文件头 + 25字节IHDR块）
		if (dataSpan.size() < 33)
		{
			setError(shine::loader::EAssetLoaderError::PARSE_ERROR, "PNG数据不完整：缺少IHDR块");
			setState(shine::loader::EAssetLoadState::FAILD);
			return false;
		}
		
		bool isEnd = false;
		std::span<const std::byte> chunkData = dataSpan.subspan(33); // 跳过文件头(8) + IHDR块(25)
		
		while (!isEnd)
		{
			// 优化：提前检查是否有足够的数据读取块头（至少12字节）
			if (chunkData.size() < 12)
			{
				fmt::println("警告: PNG 块数据不足，停止解析");
				break;
			}
			
			uint32_t chunkLength = 0;
			read_be_ref(chunkData, chunkLength);
			
			std::span<const std::byte> type = chunkData.subspan(4);
			
			// 检查是否有足够的数据读取块内容
			if (chunkData.size() < 8 + chunkLength)
			{
				fmt::println("警告: PNG 块内容数据不足，停止解析");
				break;
			}
			
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
				// 优化：遇到IEND块后立即退出，不需要继续处理
				isEnd = true;
				break;
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
			auto errorState = shine::loader::EAssetLoadState::FAILD;
			setState(errorState);
			return std::unexpected(result.error());
		}

		setState(shine::loader::EAssetLoadState::PARSING_DATA);
		
		auto file_data_view = std::move(result->view);

		// 验证 PNG 文件格式（只验证文件头，不解析块）
		if (!IsPngFile(file_data_view.content))
		{
			setError(shine::loader::EAssetLoaderError::INVALID_FORMAT);
			auto errorState = shine::loader::EAssetLoadState::FAILD;
			setState(errorState);
			return std::unexpected("无效的 PNG 文件格式");
		}

		setState(shine::loader::EAssetLoadState::PROCESSING);

		// 保存完整的 PNG 文件数据用于后续解码
		rawPngData.assign(
			reinterpret_cast<const uint8_t*>(file_data_view.content.data()),
			reinterpret_cast<const uint8_t*>(file_data_view.content.data()) + file_data_view.content.size()
		);

		// 解析后续的 PNG 块（从偏移 33 开始，跳过文件头 8 字节 + IHDR 块 25 字节）
		// IsPngFile 已经解析了 IHDR 块，现在解析其他块
		bool isEnd = false;
		std::span<const std::byte> Data = file_data_view.content.subspan(33);
		while (!isEnd)
		{
			// 优化：提前检查是否有足够的数据读取块头（至少12字节：4字节长度+4字节类型+4字节CRC）
			if (Data.size() < 12)
			{
				fmt::println("警告: PNG 块数据不足，停止解析");
				break;
			}
			
			unsigned chunkLength = 0;
			read_be_ref(Data, chunkLength);

			std::span<const std::byte> type = Data.subspan(4);
			
			// 检查是否有足够的数据读取块内容
			if (Data.size() < 8 + chunkLength)
			{
				fmt::println("警告: PNG 块内容数据不足，停止解析");
				break;
			}
			
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
				// 优化：遇到IEND块后立即退出，不需要继续处理
				isEnd = true;
				break;
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

		return {};
	}

	bool png::IsPngFile(std::span<const std::byte> content) noexcept
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

		// IsPngFile 只验证文件格式，不解析后续块
		// 块解析应该在 parsePngFile 中进行
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
		palettColors.resize(palettesize);

		// 优化：批量处理调色板颜色
		// 先批量复制RGB数据，然后批量设置Alpha通道
		const uint8_t* srcData = reinterpret_cast<const uint8_t*>(data.data());
		for (size_t i = 0; i < palettesize; ++i)
		{
			size_t srcOffset = i * 3;
			palettColors[i][0] = srcData[srcOffset];     // R
			palettColors[i][1] = srcData[srcOffset + 1]; // G
			palettColors[i][2] = srcData[srcOffset + 2]; // B
			palettColors[i][3] = 255;                     // A (默认不透明，后续可能由 tRNS 块更新)
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
				for (size_t i = 0; i < std::min(length, palettColors.size()); ++i)
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
			if (length != 1) return false;

			const uint8_t index = read_u8(data, 0);

			if (index >= palettesize) return false;

			// 使用调色板中的实际颜色值，而不是索引值
			if (index < palettColors.size())
			{
				background = PngBackground(
					palettColors[index][0],
					palettColors[index][1],
					palettColors[index][2]
				);
			}
			else
			{
				return false;
			}
		}
		else if (colorType == PngColorType::GERY || colorType == PngColorType::GERY_ALPHA)
		{
			if (length != 2) return false;

			const uint16_t gray = read_u16(data, 0);

			background = PngBackground(
				gray, gray, gray
			);
		}
		else if (colorType == PngColorType::RGB || colorType == PngColorType::RGBA)
		{
			if (length != 6) return false;

			background = PngBackground(
				read_u16(data, 0),
				read_u16(data, 2),
				read_u16(data, 4)
			);
		}
		else
		{
			// 不支持的颜色类型
			return false;
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
		std::expected<std::vector<uint8_t>, std::string> decompressedResult;
		{
			util::FunctionTimer timer1("PNG解码: Zlib解压缩", util::TimerPrecision::Milliseconds);
			decompressedResult = zlibDecompress(idatData);
		}
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
		// 对于非交织图像，可以精确计算大小；对于Adam7，decodeAdam7会处理
		size_t estimatedSize = _height * scanlineWidth;
		if (bitDepth < 8)
		{
			// 低位深度可能需要额外的填充位空间
			size_t actualBitsPerLine = _width * bitDepth;
			size_t paddedBitsPerLine = ((actualBitsPerLine + 7) / 8) * 8;
			estimatedSize = _height * ((paddedBitsPerLine + 7) / 8);
		}
		
		// 对于非交织图像，直接resize而不是reserve，避免多次分配
		if (interlaceMethod == 0)
		{
			unfilteredData.resize(estimatedSize);
		}
		else
		{
			unfilteredData.reserve(estimatedSize);
		}
		
		// 4. 解滤波器
		{
			util::FunctionTimer timer2("PNG解码: 解滤波器", util::TimerPrecision::Milliseconds);
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
					
					// 优化：直接使用预分配的缓冲区，避免resize
					size_t reconOffset = y * scanlineWidth;
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
						unfilteredData = removePaddingBits(unfilteredData, actualBitsPerLine, paddedBitsPerLine, _height);
					}
				}
			}
		}
		
		// 5. 转换为 RGBA
		{
			util::FunctionTimer timer3("PNG解码: 转换为RGBA", util::TimerPrecision::Milliseconds);
			convertToRGBA(unfilteredData, imageData);
		}
		
		return {};
	}

	std::expected<std::vector<uint8_t>, std::string> png::decodeRGB()
	{
		// 先解码为 RGBA，然后转换为 RGB
		auto result = decodeInternal();
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
	
	// 使用通用的 HuffmanTree（来自 util/encoding/huffman_tree.h）
	using HuffmanTree = util::HuffmanTree;
	
	// 从码长度构建 Huffman 树（使用通用实现）
	static std::expected<HuffmanTree, std::string> buildHuffmanTree(
		const uint32_t* bitlen, size_t numcodes, uint32_t maxbitlen)
	{
		// 使用通用实现
		return util::buildHuffmanTree(bitlen, numcodes, maxbitlen);
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
	
	// Huffman 解码符号（使用通用实现）
	// 注意：直接使用 util::huffmanDecodeSymbol 避免函数调用不明确
	
	// 解压缩 Huffman 块（BTYPE=1 或 2）
	// 注意：output 参数用于累积所有块的数据，使得距离码可以引用之前块的数据
	static std::expected<void, std::string> inflateHuffmanBlock(
		util::BitReader& reader, uint32_t btype, std::vector<uint8_t>& output, size_t max_output_size = 0)
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
			// 检查是否有足够的数据读取头部（至少需要14位：5+5+4）
			if (!reader.hasMoreData() || reader.remainingBytes() < 2)
			{
				return std::unexpected("数据不足：无法读取动态Huffman树头部");
			}
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
				// 检查是否有足够的数据
				if (!reader.hasMoreData())
				{
					return std::unexpected("数据不足：无法读取代码长度码");
				}
				reader.ensureBits(25);
				uint32_t code = util::huffmanDecodeSymbol(reader, tree_cl);
				
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
		
		bool done = false;
		while (!done)
		{
			// 检查是否有足够的数据
			if (!reader.hasMoreData())
			{
				return std::unexpected("数据不足：无法读取Huffman符号");
			}
			
			// 解码符号（huffmanDecodeSymbol内部会调用ensureBits，这里不需要重复调用）
			uint32_t code_ll = util::huffmanDecodeSymbol(reader, tree_ll);
			
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
				
				// 解码距离码（huffmanDecodeSymbol内部会调用ensureBits）
				uint32_t code_d = util::huffmanDecodeSymbol(reader, tree_d);
				
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
				
				// LZ77 解码：从输出缓冲区复制（可以引用之前所有块的数据）
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
					uint32_t forward = distance;
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
		
		return {};
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
		// 优化：根据压缩数据大小估算解压后大小，减少重新分配
		// PNG图像解压后大小约为：height * (width * bytesPerPixel + 1)
		// 对于844x167 RGBA，约为 167 * (844*4 + 1) ≈ 563KB
		// 保守估计：压缩比通常为2-10倍，预分配压缩大小的5倍
		size_t estimatedSize = compressed.size() * 5;
		if (estimatedSize < 1024) estimatedSize = 1024; // 至少1KB
		output.reserve(estimatedSize);
		
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
				// 注意：传递累积的输出缓冲区，使得距离码可以引用之前块的数据
				auto blockResult = inflateHuffmanBlock(reader, btype, output);
				if (!blockResult.has_value())
				{
					return std::unexpected("Huffman 块解码失败: " + blockResult.error());
				}
			}
			else
			{
				return std::unexpected("不支持的 BTYPE: " + std::to_string(btype));
			}
		}
		
		// 验证 Adler-32 校验和
		// Adler-32校验和在zlib中是大端序存储的，read_u32会将其转换为本机字节序
		if (deflateData.size() >= 4)
		{
			// 读取期望的Adler-32校验和（大端序，read_u32会转换为本机字节序）
			uint32_t expectedAdler = util::read_u32(compressedSpan, compressed.size() - 4);
			
			// 计算Adler-32校验和
			// Adler-32算法：s1和s2初始值分别为1和0
			// 对于每个字节：s1 = (s1 + byte) % 65521, s2 = (s2 + s1) % 65521
			// 最终校验和 = (s2 << 16) | s1
			uint32_t s1 = 1, s2 = 0;
			const uint32_t ADLER_MOD = 65521;
			
			// 优化：批量处理减少模运算次数
			// 由于s1每次最多加255，可以累积约257个字节后再做模运算（65521/255 ≈ 257）
			// 使用延迟模运算优化性能
			const size_t BATCH_SIZE = 256; // 批量处理大小
			size_t i = 0;
			const size_t size = output.size();
			
			// 批量处理
			for (; i + BATCH_SIZE <= size; i += BATCH_SIZE)
			{
				for (size_t j = 0; j < BATCH_SIZE; ++j)
				{
					s1 += output[i + j];
					s2 += s1;
				}
				// 延迟模运算：批量处理后再做模运算
				s1 %= ADLER_MOD;
				s2 %= ADLER_MOD;
			}
			
			// 处理剩余字节
			for (; i < size; ++i)
			{
				s1 = (s1 + output[i]) % ADLER_MOD;
				s2 = (s2 + s1) % ADLER_MOD;
			}
			
			// 组合校验和：高16位是s2，低16位是s1
			uint32_t calculatedAdler = (s2 << 16) | s1;
			
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
			// 复制前bytewidth个字节
			const size_t copyLen = (bytewidth < length) ? bytewidth : length;
			std::memcpy(recon.data(), scanline.data(), copyLen);
			
			// 优化：使用SIMD加速Sub滤波器（如果可用）
#ifndef __EMSCRIPTEN__
#if defined(__AVX2__) && (defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__))
			// AVX2优化：批量处理Sub滤波器
			if (length > bytewidth + 32)
			{
				size_t i = bytewidth;
				const uint8_t* src = scanline.data();
				uint8_t* dst = recon.data();
				
				// 处理对齐部分
				for (; i + 32 <= length; i += 32)
				{
					__m256i scan = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
					__m256i prev = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(dst + i - bytewidth));
					__m256i result = _mm256_add_epi8(scan, prev);
					_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), result);
				}
				
				// 处理剩余字节
				for (; i < length; ++i)
				{
					dst[i] = src[i] + dst[i - bytewidth];
				}
			}
			else
			{
				// 标准实现（小数据）
				for (size_t i = bytewidth; i < length; ++i)
				{
					recon[i] = scanline[i] + recon[i - bytewidth];
				}
			}
#else
			// 标准实现（无AVX2）
			for (size_t i = bytewidth; i < length; ++i)
			{
				recon[i] = scanline[i] + recon[i - bytewidth];
			}
#endif
#else
			// 标准实现（WASM平台）
			for (size_t i = bytewidth; i < length; ++i)
			{
				recon[i] = scanline[i] + recon[i - bytewidth];
			}
#endif
			break;
		}
		
		case 2: // Up
		{
			if (prevline)
			{
				// 优化：使用SIMD加速Up滤波器（如果可用）
#ifndef __EMSCRIPTEN__
#if defined(__AVX2__) && (defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__))
				if (length >= 32)
				{
					size_t i = 0;
					const uint8_t* src = scanline.data();
					const uint8_t* prev = prevline;
					uint8_t* dst = recon.data();
					
					// AVX2批量处理
					for (; i + 32 <= length; i += 32)
					{
						__m256i scan = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
						__m256i prev_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(prev + i));
						__m256i result = _mm256_add_epi8(scan, prev_vec);
						_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), result);
					}
					
					// 处理剩余字节
					for (; i < length; ++i)
					{
						dst[i] = src[i] + prev[i];
					}
					break;
				}
#endif
#endif
				// 标准实现
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
				// 优化：如果步长为1（连续像素），可以批量复制整行；否则逐个像素复制
				if (ADAM7_DX[pass] == 1 && ADAM7_IX[pass] == 0)
				{
					// 优化：整行连续，可以批量复制
					for (uint32_t y = 0; y < passh[pass]; ++y)
					{
						uint32_t dstY = ADAM7_IY[pass] + y * ADAM7_DY[pass];
						if (dstY < _height)
						{
							size_t srcOffset = y * passScanlineWidth;
							size_t dstOffset = dstY * scanlineWidth;
							size_t copySize = passScanlineWidth;
							
							if (srcOffset + copySize <= passData.size() && 
							    dstOffset + copySize <= output.size())
							{
								std::memcpy(output.data() + dstOffset, 
								           passData.data() + srcOffset, copySize);
							}
						}
					}
				}
				else
				{
					// 非连续像素，逐个复制
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
					passDataNoPadding = removePaddingBits(passData, actualBitsPerLine, paddedBitsPerLine, passh[pass]);
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
				
				// 计算源数据的最大位数
				size_t maxSrcBits = passh[pass] * actualBitsPerLine;
				size_t maxDstBits = _height * outputBitsPerLine;
				
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
							
							// 边界检查：确保不会超出数据范围
							if (srcBitPos + bitDepth > maxSrcBits || dstBitPos + bitDepth > maxDstBits)
							{
								return std::unexpected("Adam7 通道 " + std::to_string(pass) + 
									" 位位置超出范围: src=" + std::to_string(srcBitPos) + 
									", dst=" + std::to_string(dstBitPos));
							}
							
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
	// 优化：内联函数，减少函数调用开销；优化位操作计算
	inline uint8_t png::readBitFromStream(size_t& bitPos, const uint8_t* data) noexcept
	{
		// 优化：使用位操作替代除法和取模
		size_t bytePos = bitPos >> 3;  // bitPos / 8
		size_t bitInByte = 7 - (bitPos & 7); // bitPos % 8, MSB优先
		uint8_t bit = (data[bytePos] >> bitInByte) & 1;
		++bitPos;
		return bit;
	}
	
	// 写入位到位流（MSB优先，用于低位深度）
	// 优化：内联函数，减少函数调用开销；优化位操作计算
	inline void png::writeBitToStream(size_t& bitPos, uint8_t* data, uint8_t bit) noexcept
	{
		// 优化：使用位操作替代除法和取模
		size_t bytePos = bitPos >> 3;  // bitPos / 8
		size_t bitInByte = 7 - (bitPos & 7); // bitPos % 8, MSB优先
		uint8_t mask = 1 << bitInByte;
		if (bit)
		{
			data[bytePos] |= mask;
		}
		else
		{
			data[bytePos] &= ~mask;
		}
		++bitPos;
	}
	
	// 移除填充位（用于低位深度）
	std::vector<uint8_t> png::removePaddingBits(
		const std::vector<uint8_t>& paddedData, size_t actualBitsPerLine, size_t paddedBitsPerLine, size_t height) const
	{
		if (actualBitsPerLine == paddedBitsPerLine)
		{
			return paddedData; // 没有填充位
		}
		
		size_t diff = paddedBitsPerLine - actualBitsPerLine;
		size_t outputBits = height * actualBitsPerLine;
		size_t outputBytes = (outputBits + 7) / 8;
		
		std::vector<uint8_t> output(outputBytes, 0);
		size_t inputBitPos = 0;
		size_t outputBitPos = 0;
		
		for (size_t y = 0; y < height; ++y)
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
		// 优化：预分配精确大小的空间，避免多次重新分配
		const size_t pixelCount = _width * _height;
		const size_t outputSize = pixelCount * 4;
		output.clear();
		output.resize(outputSize);
		
		if (bitDepth == 8)
		{
			switch (colorType)
			{
			case PngColorType::GERY:
				{
					size_t outIdx = 0;
					for (size_t i = 0; i < rawData.size(); ++i)
					{
						uint8_t gray = rawData[i];
						output[outIdx++] = gray;
						output[outIdx++] = gray;
						output[outIdx++] = gray;
						output[outIdx++] = 255;
					}
				}
				break;
				
			case PngColorType::RGB:
				{
					// 优化：RGB转RGBA，使用SIMD加速
					const size_t pixelCount = rawData.size() / 3;
					const uint8_t* src = rawData.data();
					uint8_t* dst = output.data();
					
#ifndef __EMSCRIPTEN__
#if defined(__SSE4_1__) && (defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__))
					// SSE4.1优化：每次处理4个像素（12字节输入，16字节输出）
					// 使用shuffle指令重新排列RGB数据并插入Alpha通道
					size_t i = 0;
					const __m128i alpha = _mm_set1_epi8(0xFF);
					
					// 确保至少有5个像素（15字节）才使用SIMD，避免越界读取
					// _mm_loadu_si128 会加载16字节，所以需要确保 i*3 + 15 不超出缓冲区
					// 当 i + 4 < pixelCount 时，i*3 + 15 < (pixelCount-1)*3 + 15 = pixelCount*3 + 12
					// 但我们需要 i*3 + 15 < pixelCount*3，即 i + 5 <= pixelCount
					for (; i + 5 <= pixelCount; i += 4)
					{
						// 加载12字节RGB数据（安全：i*3 + 15 < (i+5)*3 = i*3 + 15 <= pixelCount*3）
						__m128i rgb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i * 3));
						
						// 使用shuffle重新排列：RGBRGBRGBRGB -> RGBARGBA...
						// Shuffle mask: 提取位置0,1,2,插入255, 位置3,4,5,插入255, ...
						__m128i rgba = _mm_shuffle_epi8(rgb, _mm_setr_epi8(
							0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1
						));
						
						// 插入Alpha通道（使用blend）
						__m128i alpha_mask = _mm_setr_epi8(0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1);
						rgba = _mm_blendv_epi8(rgba, alpha, alpha_mask);
						
						_mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i * 4), rgba);
					}
					
					// 处理剩余像素（使用标量代码）
					for (; i < pixelCount; ++i)
					{
						dst[i * 4 + 0] = src[i * 3 + 0];
						dst[i * 4 + 1] = src[i * 3 + 1];
						dst[i * 4 + 2] = src[i * 3 + 2];
						dst[i * 4 + 3] = 255;
					}
#else
					// 标量优化：展开循环，每次处理4个像素
					size_t i = 0;
					for (; i + 4 <= pixelCount; i += 4)
					{
						// 像素0
						dst[i * 4 + 0] = src[i * 3 + 0];
						dst[i * 4 + 1] = src[i * 3 + 1];
						dst[i * 4 + 2] = src[i * 3 + 2];
						dst[i * 4 + 3] = 255;
						// 像素1
						dst[i * 4 + 4] = src[i * 3 + 3];
						dst[i * 4 + 5] = src[i * 3 + 4];
						dst[i * 4 + 6] = src[i * 3 + 5];
						dst[i * 4 + 7] = 255;
						// 像素2
						dst[i * 4 + 8] = src[i * 3 + 6];
						dst[i * 4 + 9] = src[i * 3 + 7];
						dst[i * 4 + 10] = src[i * 3 + 8];
						dst[i * 4 + 11] = 255;
						// 像素3
						dst[i * 4 + 12] = src[i * 3 + 9];
						dst[i * 4 + 13] = src[i * 3 + 10];
						dst[i * 4 + 14] = src[i * 3 + 11];
						dst[i * 4 + 15] = 255;
					}
					
					// 处理剩余像素
					for (; i < pixelCount; ++i)
					{
						dst[i * 4 + 0] = src[i * 3 + 0];
						dst[i * 4 + 1] = src[i * 3 + 1];
						dst[i * 4 + 2] = src[i * 3 + 2];
						dst[i * 4 + 3] = 255;
					}
#endif
#else
					// WASM平台：标量实现
					size_t i = 0;
					for (; i + 4 <= pixelCount; i += 4)
					{
						dst[i * 4 + 0] = src[i * 3 + 0];
						dst[i * 4 + 1] = src[i * 3 + 1];
						dst[i * 4 + 2] = src[i * 3 + 2];
						dst[i * 4 + 3] = 255;
						dst[i * 4 + 4] = src[i * 3 + 3];
						dst[i * 4 + 5] = src[i * 3 + 4];
						dst[i * 4 + 6] = src[i * 3 + 5];
						dst[i * 4 + 7] = 255;
						dst[i * 4 + 8] = src[i * 3 + 6];
						dst[i * 4 + 9] = src[i * 3 + 7];
						dst[i * 4 + 10] = src[i * 3 + 8];
						dst[i * 4 + 11] = 255;
						dst[i * 4 + 12] = src[i * 3 + 9];
						dst[i * 4 + 13] = src[i * 3 + 10];
						dst[i * 4 + 14] = src[i * 3 + 11];
						dst[i * 4 + 15] = 255;
					}
					for (; i < pixelCount; ++i)
					{
						dst[i * 4 + 0] = src[i * 3 + 0];
						dst[i * 4 + 1] = src[i * 3 + 1];
						dst[i * 4 + 2] = src[i * 3 + 2];
						dst[i * 4 + 3] = 255;
					}
#endif
				}
				break;
				
			case PngColorType::PALETTE:
				{
					size_t outIdx = 0;
					for (size_t i = 0; i < rawData.size(); ++i)
					{
						uint8_t index = rawData[i];
						if (index < palettColors.size())
						{
							output[outIdx++] = palettColors[index][0];
							output[outIdx++] = palettColors[index][1];
							output[outIdx++] = palettColors[index][2];
							output[outIdx++] = palettColors[index][3];
						}
						else
						{
							output[outIdx++] = 0;
							output[outIdx++] = 0;
							output[outIdx++] = 0;
							output[outIdx++] = 255;
						}
					}
				}
				break;
				
			case PngColorType::GERY_ALPHA:
				{
					size_t outIdx = 0;
					for (size_t i = 0; i < rawData.size(); i += 2)
					{
						uint8_t gray = rawData[i];
						uint8_t alpha = rawData[i + 1];
						output[outIdx++] = gray;
						output[outIdx++] = gray;
						output[outIdx++] = gray;
						output[outIdx++] = alpha;
					}
				}
				break;
				
			case PngColorType::RGBA:
				// 优化：RGBA类型不需要转换，直接memcpy避免vector的assign开销
				std::memcpy(output.data(), rawData.data(), rawData.size());
				break;
				
			default:
				break;
			}
		}
		else if (bitDepth == 16)
		{
			// 16 位深度需要降采样到 8 位
			// PNG 规范：16位值以大端序存储，read_u16会将其转换为本机字节序
			// 降采样时取高8位（MSB），这是PNG规范推荐的做法
			std::span<const std::byte> rawDataSpan(reinterpret_cast<const std::byte*>(rawData.data()), rawData.size());
			
			switch (colorType)
			{
			case PngColorType::GERY:
				{
					// 优化：使用byte_convert工具处理16位灰度值
					// read_u16会自动处理大端序到小端序的转换
					size_t outIdx = 0;
					for (size_t i = 0; i < rawData.size(); i += 2)
					{
						uint16_t gray16 = util::read_u16(rawDataSpan, i);
						// read_u16已处理字节序转换，取高8位进行降采样
						uint8_t gray = static_cast<uint8_t>(gray16 >> 8);
						output[outIdx++] = gray;
						output[outIdx++] = gray;
						output[outIdx++] = gray;
						output[outIdx++] = 255;
					}
				}
				break;
				
			case PngColorType::RGB:
				{
					// 优化：使用byte_convert工具处理16位RGB值
					// read_u16会自动处理大端序到小端序的转换
					size_t outIdx = 0;
					for (size_t i = 0; i < rawData.size(); i += 6)
					{
						uint16_t r16 = util::read_u16(rawDataSpan, i);
						uint16_t g16 = util::read_u16(rawDataSpan, i + 2);
						uint16_t b16 = util::read_u16(rawDataSpan, i + 4);
						output[outIdx++] = static_cast<uint8_t>(r16 >> 8);
						output[outIdx++] = static_cast<uint8_t>(g16 >> 8);
						output[outIdx++] = static_cast<uint8_t>(b16 >> 8);
						output[outIdx++] = 255;
					}
				}
				break;
				
			case PngColorType::GERY_ALPHA:
				{
					// 优化：使用byte_convert工具处理16位灰度+Alpha值
					// read_u16会自动处理大端序到小端序的转换
					size_t outIdx = 0;
					for (size_t i = 0; i < rawData.size(); i += 4)
					{
						uint16_t gray16 = util::read_u16(rawDataSpan, i);
						uint16_t alpha16 = util::read_u16(rawDataSpan, i + 2);
						uint8_t gray = static_cast<uint8_t>(gray16 >> 8);
						uint8_t alpha = static_cast<uint8_t>(alpha16 >> 8);
						output[outIdx++] = gray;
						output[outIdx++] = gray;
						output[outIdx++] = gray;
						output[outIdx++] = alpha;
					}
				}
				break;
				
			case PngColorType::RGBA:
				{
					// 优化：使用byte_convert工具处理16位RGBA值
					// read_u16会自动处理大端序到小端序的转换
					size_t outIdx = 0;
					for (size_t i = 0; i < rawData.size(); i += 8)
					{
						uint16_t r16 = util::read_u16(rawDataSpan, i);
						uint16_t g16 = util::read_u16(rawDataSpan, i + 2);
						uint16_t b16 = util::read_u16(rawDataSpan, i + 4);
						uint16_t a16 = util::read_u16(rawDataSpan, i + 6);
						output[outIdx++] = static_cast<uint8_t>(r16 >> 8);
						output[outIdx++] = static_cast<uint8_t>(g16 >> 8);
						output[outIdx++] = static_cast<uint8_t>(b16 >> 8);
						output[outIdx++] = static_cast<uint8_t>(a16 >> 8);
					}
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
				{
					size_t outIdx = 0;
					for (size_t byteIdx = 0; byteIdx < rawData.size(); ++byteIdx)
					{
						uint8_t byte = rawData[byteIdx];
						for (size_t bitIdx = 0; bitIdx < pixelsPerByte && (byteIdx * pixelsPerByte + bitIdx) < pixelCount; ++bitIdx)
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
							
							output[outIdx++] = gray;
							output[outIdx++] = gray;
							output[outIdx++] = gray;
							output[outIdx++] = 255;
						}
					}
				}
				break;
				
			case PngColorType::PALETTE:
				bitsPerPixel = bitDepth;
				{
					size_t bitPos = 0;
					size_t totalBits = rawData.size() * 8;
					size_t requiredBits = pixelCount * bitDepth;
					
					// 验证数据是否足够
					if (requiredBits > totalBits)
					{
						output.clear();
						break;
					}
					
					size_t outIdx = 0;
					for (size_t pixelIdx = 0; pixelIdx < pixelCount; ++pixelIdx)
					{
						// 从位流读取调色板索引（MSB优先）
						// readBitFromStream会修改bitPos，这是预期的行为，用于顺序读取位
						uint8_t index = 0;
						for (size_t b = 0; b < bitDepth; ++b)
						{
							// 边界检查：确保不会超出数据范围
							if (bitPos >= totalBits)
							{
								output.clear();
								return;
							}
							index = (index << 1) | readBitFromStream(bitPos, rawData.data());
						}
						
						if (index < palettColors.size())
						{
							output[outIdx++] = palettColors[index][0];
							output[outIdx++] = palettColors[index][1];
							output[outIdx++] = palettColors[index][2];
							output[outIdx++] = palettColors[index][3];
						}
						else
						{
							// 索引超出范围，使用黑色
							output[outIdx++] = 0;
							output[outIdx++] = 0;
							output[outIdx++] = 0;
							output[outIdx++] = 255;
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



