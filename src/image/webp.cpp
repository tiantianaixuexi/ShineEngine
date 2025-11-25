#include "webp.h"

#include <string_view>
#include <string>
#include <expected>
#include <span>
#include <algorithm>
#include <cstring>
#include <bit>
#include <array>

#include "util/memory_util.h"
#include "util/file_util.h"
#include "util/encoding/byte_convert.h"
#include "util/encoding/bit_reader.h"
#include "util/encoding/huffman_tree.h"
#include "util/encoding/huffman_decoder.h"
#include "fmt/format.h"

namespace shine::image
{
	using namespace util;
	using MemoryPoolManager = util::MemoryPoolManager;

	// ============================================================================
	// WebP 格式常量定义
	// ============================================================================

	/// WebP 文件头签名（RIFF + 文件大小 + WEBP）
	static constexpr std::array<uint8_t, 4> RIFF_SIGNATURE = { 'R', 'I', 'F', 'F' };
	static constexpr std::array<uint8_t, 4> WEBP_SIGNATURE = { 'W', 'E', 'B', 'P' };
	static constexpr std::array<uint8_t, 4> VP8_SIGNATURE = { 'V', 'P', '8', ' ' };
	static constexpr std::array<uint8_t, 4> VP8L_SIGNATURE = { 'V', 'P', '8', 'L' };
	static constexpr std::array<uint8_t, 4> VP8X_SIGNATURE = { 'V', 'P', '8', 'X' };

	// ============================================================================
	// 辅助函数
	// ============================================================================

	/**
	 * @brief 检查字节数组是否匹配签名
	 */
	template<size_t N>
	constexpr bool matchSignature(std::span<const std::byte> data, const std::array<uint8_t, N>& signature) noexcept
	{
		if (data.size() < N)
		{
			return false;
		}
		return std::equal(signature.begin(), signature.end(), 
			reinterpret_cast<const uint8_t*>(data.data()));
	}

	// ============================================================================
	// IAssetLoader 接口实现
	// ============================================================================

	std::string_view webp::getFileName() const noexcept
	{
		return _name;
	}

	bool webp::loadFromFile(const char* filePath)
	{
		setState(shine::loader::EAssetLoadState::READING_FILE);

		auto result = parseWebPFile(filePath);
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

	bool webp::loadFromMemory(const void* data, size_t size)
	{
		setState(shine::loader::EAssetLoadState::PARSING_DATA);

		if (!data || size == 0)
		{
			setError(shine::loader::EAssetLoaderError::INVALID_PARAMETER);
			setState(shine::loader::EAssetLoadState::FAILD);
			return false;
		}

		std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(data), size);

		// 验证 WebP 文件格式
		if (!IsWebPFile(dataSpan))
		{
			setError(shine::loader::EAssetLoaderError::INVALID_FORMAT);
			setState(shine::loader::EAssetLoadState::FAILD);
			return false;
		}

		// 保存原始数据
		rawWebPData.assign(reinterpret_cast<const uint8_t*>(data), 
			reinterpret_cast<const uint8_t*>(data) + size);

		// 解析 WebP 数据
		auto parseResult = parseWebPData(dataSpan);
		if (!parseResult.has_value())
		{
			setError(shine::loader::EAssetLoaderError::PARSE_ERROR, parseResult.error());
			setState(shine::loader::EAssetLoadState::FAILD);
			return false;
		}

		_loaded = true;
		setState(shine::loader::EAssetLoadState::COMPLETE);
		return true;
	}

	void webp::unload()
	{
		rawWebPData.clear();
		imageData.clear();
		iccProfile.clear();
		exifData.clear();
		animationInfo = WebPAnimationInfo{};
		_name.clear();
		_width = 0;
		_height = 0;
		format = WebPFormat::LOSSY;
		_hasAlpha = false;
		_hasAnimation = false;
		_loaded = false;
		setState(shine::loader::EAssetLoadState::NONE);
	}

	// ============================================================================
	// WebP 文件解析
	// ============================================================================

	constexpr bool webp::IsWebPFile(std::span<const std::byte> content) noexcept
	{
		// WebP 文件格式：
		// - RIFF (4 bytes)
		// - 文件大小 - 8 (4 bytes, little-endian)
		// - WEBP (4 bytes)
		// - 后续是 VP8/VP8L/VP8X 块

		if (content.size() < 12)
		{
			return false;
		}

		// 检查 RIFF 签名
		if (!matchSignature(content.subspan(0, 4), RIFF_SIGNATURE))
		{
			return false;
		}

		// 检查 WEBP 签名（偏移 8）
		if (!matchSignature(content.subspan(8, 4), WEBP_SIGNATURE))
		{
			return false;
		}

		return true;
	}

	std::expected<void, std::string> webp::parseWebPFile(std::string_view filePath)
	{
		// 使用 file_util 读取文件
#ifndef SHINE_PLATFORM_WASM
		auto fileResult = util::read_file_bytes(filePath.data());
		if (!fileResult.has_value())
		{
			return std::unexpected(fileResult.error());
		}

		std::vector<std::byte> fileData = std::move(fileResult.value());
#else
		bool success = false;
		std::vector<std::byte> fileData = file_util::read_file_bytes(filePath.data(), &success);
		if (!success)
		{
			return std::unexpected("Failed to read WebP file");
		}
#endif

		// 验证文件格式
		std::span<const std::byte> dataSpan(fileData.data(), fileData.size());
		if (!IsWebPFile(dataSpan))
		{
			return std::unexpected("Invalid WebP file format");
		}

		// 保存原始数据
		rawWebPData.assign(reinterpret_cast<const uint8_t*>(fileData.data()),
			reinterpret_cast<const uint8_t*>(fileData.data()) + fileData.size());

		// 解析 WebP 数据
		return parseWebPData(dataSpan);
	}

	std::expected<void, std::string> webp::parseWebPData(std::span<const std::byte> data)
	{
		if (data.size() < 12)
		{
			return std::unexpected("WebP data too small");
		}

		// 提取格式特征
		if (!extractFeatures())
		{
			return std::unexpected("Failed to extract WebP features");
		}

		// 提取元数据块（ICC、EXIF 等）
		extractChunks();

		return {};
	}


	/**
	 * @brief 解析 RIFF 块
	 * @param data 数据 span
	 * @param offset 偏移量
	 * @param chunkType 输出的块类型
	 * @return 块大小（不包括块头），0 表示失败
	 */
	inline uint32_t parseRIFFChunk(std::span<const std::byte> data, size_t offset, std::array<char, 4>& chunkType)
	{
		if (offset + 8 > data.size())
		{
			return 0;
		}
		
		// RIFF 块格式：[ChunkID (4 bytes)][ChunkSize (4 bytes)]
		// 先读取块类型（4 字节 ASCII）
		std::memcpy(chunkType.data(), data.data() + offset, 4);
		
		// 然后读取块大小（小端序，不包括块头）
		uint32_t chunkSize = util::read_le32(data, offset + 4);
		
		return chunkSize;
	}

	bool webp::extractFeatures()
	{
		if (rawWebPData.empty() || rawWebPData.size() < 12)
		{
			return false;
		}

		std::span<const std::byte> data(reinterpret_cast<const std::byte*>(rawWebPData.data()), rawWebPData.size());
		size_t offset = 0;

		// 检查 RIFF 签名
		if (!matchSignature(data.subspan(0, 4), RIFF_SIGNATURE))
		{
			return false;
		}

		// 读取文件大小（小端序）
		uint32_t fileSize = util::read_le32(data, 4);
		offset += 8;

		// 检查 WEBP 签名
		if (!matchSignature(data.subspan(offset, 4), WEBP_SIGNATURE))
		{
			return false;
		}
		offset += 4;

		// 解析第一个块（VP8/VP8L/VP8X）
		std::array<char, 4> chunkType;
		uint32_t chunkSize = parseRIFFChunk(data, offset, chunkType);
		
		if (chunkSize == 0 || offset + 8 + chunkSize > data.size())
		{
			return false;
		}

		// 检查块类型
		if (std::memcmp(chunkType.data(), "VP8X", 4) == 0)
		{
			// VP8X 块（扩展格式）
			format = WebPFormat::EXTENDED;
			
			if (chunkSize < 10)
			{
				return false;
			}

			size_t vp8xOffset = offset + 8;
			
			// 读取标志位
			uint8_t flags = util::read_u8(data, vp8xOffset);
			_hasAnimation = (flags & 0x02) != 0;
			_hasAlpha = (flags & 0x10) != 0;
			
			// 读取画布尺寸（24 位，小端序）
			_width = util::read_le24(data, vp8xOffset + 4) + 1;
			_height = util::read_le24(data, vp8xOffset + 7) + 1;
			
			// 提取动画信息
			if (_hasAnimation)
			{
				animationInfo.canvasWidth = _width;
				animationInfo.canvasHeight = _height;
			}
		}
		else if (std::memcmp(chunkType.data(), "VP8 ", 4) == 0)
		{
			// VP8 块（有损格式）
			format = WebPFormat::LOSSY;
			_hasAlpha = false;
			_hasAnimation = false;
			
			// VP8 格式：帧头（3字节） + 宽度/高度（2字节 + 2字节）
			if (chunkSize < 10)
			{
				return false;
			}
			
			size_t vp8Offset = offset + 8;
			
			// 跳过帧头（3字节）
			// 读取尺寸（14位宽度 + 14位高度，小端序）
			uint16_t dimension = util::read_le16(data, vp8Offset + 3);
			_width = (dimension & 0x3FFF) + 1;
			_height = ((dimension >> 14) | (util::read_le16(data, vp8Offset + 5) << 2)) + 1;
		}
		else if (std::memcmp(chunkType.data(), "VP8L", 4) == 0)
		{
			// VP8L 块（无损格式）
			format = WebPFormat::LOSSLESS;
			_hasAnimation = false;
			
			if (chunkSize < 5)
			{
				return false;
			}
			
			size_t vp8lOffset = offset + 8;
			
			// VP8L 格式：签名（1字节） + 尺寸（14位宽度 + 14位高度，小端序）
			uint8_t signature = util::read_u8(data, vp8lOffset);
			if ((signature & 0xC0) != 0x2E)
			{
				return false; // 无效的 VP8L 签名
			}
			
			// 读取尺寸（14位宽度 + 14位高度）
			uint32_t dimension = util::read_le24(data, vp8lOffset + 1);
			_width = (dimension & 0x3FFF) + 1;
			_height = ((dimension >> 14) & 0x3FFF) + 1;
			
			// 检查 Alpha 通道（从标志位）
			_hasAlpha = (signature & 0x10) != 0;
		}
		else
		{
			return false; // 未知的块类型
		}

		return true;
	}

	bool webp::extractChunks()
	{
		if (rawWebPData.empty() || rawWebPData.size() < 12)
		{
			return false;
		}

		std::span<const std::byte> data(reinterpret_cast<const std::byte*>(rawWebPData.data()), rawWebPData.size());
		size_t offset = 12; // 跳过 RIFF + 文件大小 + WEBP

		// 遍历所有块
		while (offset + 8 <= data.size())
		{
			std::array<char, 4> chunkType;
			uint32_t chunkSize = parseRIFFChunk(data, offset, chunkType);
			
			if (chunkSize == 0 || offset + 8 + chunkSize > data.size())
			{
				break;
			}

			size_t chunkDataOffset = offset + 8;

			// 解析不同类型的块
			if (std::memcmp(chunkType.data(), "ICCP", 4) == 0)
			{
				// ICC 配置文件
				iccProfile.assign(
					reinterpret_cast<const uint8_t*>(data.data() + chunkDataOffset),
					reinterpret_cast<const uint8_t*>(data.data() + chunkDataOffset + chunkSize)
				);
			}
			else if (std::memcmp(chunkType.data(), "EXIF", 4) == 0)
			{
				// EXIF 数据
				exifData.assign(
					reinterpret_cast<const uint8_t*>(data.data() + chunkDataOffset),
					reinterpret_cast<const uint8_t*>(data.data() + chunkDataOffset + chunkSize)
				);
			}
			else if (std::memcmp(chunkType.data(), "ANIM", 4) == 0 && chunkSize >= 6)
			{
				// 动画信息（在 VP8X 块之后）
				animationInfo.canvasWidth = util::read_le24(data, chunkDataOffset) + 1;
				animationInfo.canvasHeight = util::read_le24(data, chunkDataOffset + 3) + 1;
				animationInfo.loopCount = util::read_le16(data, chunkDataOffset + 6);
			}
			else if (std::memcmp(chunkType.data(), "ANMF", 4) == 0)
			{
				// 动画帧（暂时跳过，后续可以扩展）
			}

			// 移动到下一个块（块大小必须是偶数，如果是奇数则加1）
			offset += 8 + chunkSize;
			if (chunkSize & 1)
			{
				offset += 1; // 填充字节
			}
		}

		return true;
	}

	// ============================================================================
	// WebP 图像解码辅助函数
	// ============================================================================

	/**
	 * @brief 查找并提取 VP8/VP8L/ALPH 块数据
	 * @param data WebP 数据
	 * @param targetChunkType 要查找的块类型（"VP8 ", "VP8L", "ALPH"）
	 * @return 块数据指针和大小，如果未找到返回 nullptr
	 */
	static std::pair<const uint8_t*, size_t> findChunkData(std::span<const std::byte> data, const char* targetChunkType)
	{
		if (data.size() < 12)
		{
			return { nullptr, 0 };
		}

		size_t offset = 12; // 跳过 RIFF + 文件大小 + WEBP

		// 遍历所有块
		while (offset + 8 <= data.size())
		{
			std::array<char, 4> chunkType;
			uint32_t chunkSize = parseRIFFChunk(data, offset, chunkType);
			
			if (chunkSize == 0 || offset + 8 + chunkSize > data.size())
			{
				break;
			}

			// 检查是否匹配目标块类型
			if (std::memcmp(chunkType.data(), targetChunkType, 4) == 0)
			{
				return {
					reinterpret_cast<const uint8_t*>(data.data() + offset + 8),
					chunkSize
				};
			}

			// 移动到下一个块
			offset += 8 + chunkSize;
			if (chunkSize & 1)
			{
				offset += 1; // 填充字节
			}
		}

		return { nullptr, 0 };
	}

	// ============================================================================
	// WebP 图像解码
	// ============================================================================

	std::expected<void, std::string> webp::decode()
	{
		if (!_loaded)
		{
			return std::unexpected("WebP image not loaded");
		}

		return decodeInternal();
	}

	std::expected<std::vector<uint8_t>, std::string> webp::decodeRGB()
	{
		if (!_loaded)
		{
			return std::unexpected("WebP image not loaded");
		}

		if (rawWebPData.empty())
		{
			return std::unexpected("WebP data is empty");
		}

		if (_width == 0 || _height == 0)
		{
			return std::unexpected("Invalid image dimensions");
		}

		// 检查 WASM 内存（如果支持）
#ifdef __EMSCRIPTEN__
		size_t requiredMemory = _width * _height * 3;
		if (!wasm::hasEnoughMemory(requiredMemory))
		{
			return std::unexpected("Insufficient memory in WASM environment");
		}
#endif

		// 先解码为 RGBA，然后提取 RGB
		auto decodeResult = decodeInternal();
		if (!decodeResult.has_value())
		{
			return std::unexpected(decodeResult.error());
		}

		// 提取 RGB 数据（跳过 Alpha 通道）
		// 优化：使用循环展开和直接内存访问
		std::vector<uint8_t> rgbData(_width * _height * 3);
		const uint8_t* src = imageData.data();
		uint8_t* dst = rgbData.data();
		const size_t pixelCount = _width * _height;
		
		// 优化：循环展开，每次处理 4 个像素
		size_t i = 0;
		for (; i + 4 <= pixelCount; i += 4)
		{
			// 像素 0
			dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
			// 像素 1
			dst[3] = src[4]; dst[4] = src[5]; dst[5] = src[6];
			// 像素 2
			dst[6] = src[8]; dst[7] = src[9]; dst[8] = src[10];
			// 像素 3
			dst[9] = src[12]; dst[10] = src[13]; dst[11] = src[14];
			src += 16; // 跳过 4 个像素（4 * 4 字节）
			dst += 12; // 写入 4 个 RGB 像素（4 * 3 字节）
		}
		
		// 处理剩余像素
		for (; i < pixelCount; ++i)
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			src += 4;
			dst += 3;
		}

		return rgbData;
	}

	std::expected<void, std::string> webp::decodeInternal()
	{
		if (rawWebPData.empty())
		{
			return std::unexpected("WebP data is empty");
		}

		if (_width == 0 || _height == 0)
		{
			return std::unexpected("Invalid image dimensions");
		}

		// 检查 WASM 内存（如果支持）
#ifdef __EMSCRIPTEN__
		size_t requiredMemory = _width * _height * 4;
		if (!wasm::hasEnoughMemory(requiredMemory))
		{
			return std::unexpected("Insufficient memory in WASM environment");
		}
#endif

		// 预分配输出缓冲区
		size_t dataSize = _width * _height * 4;
		imageData.resize(dataSize);

		// 准备数据 span
		std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(rawWebPData.data()), rawWebPData.size());

		// 根据格式类型解码
		switch (format)
		{
		case WebPFormat::LOSSY:
		{
			// 查找 VP8 块
			auto [vp8Data, vp8Size] = findChunkData(dataSpan, "VP8 ");
			if (!vp8Data)
			{
				return std::unexpected("VP8 chunk not found");
			}
			
			// 解码 VP8 数据
			return decodeVP8(vp8Data, vp8Size, imageData);
		}
		case WebPFormat::LOSSLESS:
		{
			// 查找 VP8L 块
			auto [vp8lData, vp8lSize] = findChunkData(dataSpan, "VP8L");
			if (!vp8lData)
			{
				return std::unexpected("VP8L chunk not found");
			}
			
			// 解码 VP8L 数据
			return decodeVP8L(vp8lData, vp8lSize, imageData);
		}
		case WebPFormat::EXTENDED:
		{
			// 扩展格式：先查找 VP8 或 VP8L 块
			auto [vp8Data, vp8Size] = findChunkData(dataSpan, "VP8 ");
			if (vp8Data)
			{
				// 解码 VP8 数据
				auto result = decodeVP8(vp8Data, vp8Size, imageData);
				if (!result.has_value())
				{
					return result;
				}
				
				// 如果有 Alpha，还需要查找并解码 ALPH 块
				if (_hasAlpha)
				{
					auto [alphData, alphSize] = findChunkData(dataSpan, "ALPH");
					if (alphData)
					{
						// 修复变量遮蔽：使用不同的变量名避免遮蔽 alphData 指针
						std::vector<uint8_t> alphaOutput(_width * _height);
						auto alphaResult = decodeALPH(alphData, alphSize, alphaOutput);
						if (alphaResult.has_value())
						{
							// 合并 Alpha 通道到 RGBA 数据（优化：直接内存访问）
							const uint8_t* alphaSrc = alphaOutput.data();
							uint8_t* rgbaDst = imageData.data() + 3; // Alpha 通道偏移
							const size_t pixelCount = _width * _height;
							
							for (size_t i = 0; i < pixelCount; ++i)
							{
								rgbaDst[i * 4] = alphaSrc[i];
							}
						}
					}
				}
				return {};
			}
			
			auto [vp8lData, vp8lSize] = findChunkData(dataSpan, "VP8L");
			if (vp8lData)
			{
				// VP8L 本身支持 Alpha，直接解码
				return decodeVP8L(vp8lData, vp8lSize, imageData);
			}
			
			return std::unexpected("No VP8 or VP8L chunk found in extended WebP");
		}
		default:
			return std::unexpected("Unknown WebP format");
		}
	}

	// ============================================================================
	// VP8L 无损解码器实现
	// ============================================================================

	// 使用通用的 HuffmanTree（来自 util/encoding/huffman_tree.h）
	using HuffmanTree = util::HuffmanTree;

	std::expected<void, std::string> webp::decodeVP8L(const uint8_t* data, size_t size, std::vector<uint8_t>& output)
	{
		if (!data || size < 5)
		{
			return std::unexpected("Invalid VP8L data");
		}

		// VP8L 格式：
		// - 签名（1字节）：0x2E（低2位必须是 10）
		// - 尺寸（3字节）：14位宽度 + 14位高度（小端序）
		// - 后续是压缩的图像数据

		uint8_t signature = data[0];
		if ((signature & 0xC0) != 0x2E)
		{
			return std::unexpected("Invalid VP8L signature");
		}

		// 读取标志位
		bool hasAlpha = (signature & 0x10) != 0;
		bool hasColorCache = (signature & 0x08) != 0;

		// 创建位读取器（跳过签名和尺寸，从第4字节开始）
		util::BitReader reader(data + 4, size - 4);

		// 读取颜色缓存大小（如果有）
		uint32_t colorCacheSize = 0;
		std::vector<uint32_t> colorCache;
		if (hasColorCache)
		{
			uint32_t colorCacheBits = reader.readBits(4);
			if (colorCacheBits > 11)
			{
				return std::unexpected("Invalid color cache size");
			}
			colorCacheSize = 1u << colorCacheBits;
			colorCache.resize(colorCacheSize, 0);
		}

		// 读取 Huffman 编码表
		// VP8L 使用 5 个 Huffman 树：
		// - 绿色（256个符号）
		// - 红色（256个符号）
		// - 蓝色（256个符号）
		// - Alpha（256个符号，如果有）
		// - 距离码（40个符号，用于 LZ77）

		constexpr size_t NUM_LITERAL_CODES = 256;
		constexpr size_t NUM_DISTANCE_CODES = 40;
		constexpr uint32_t MAX_HUFFMAN_BITS = 15;

		// VP8L 使用简单编码读取 Huffman 码长度
		// 简单编码格式：
		// - 1 bit: 是否使用简单编码（0 = 简单，1 = 复杂）
		// - 如果简单编码：
		//   - 1 bit: 符号数量（0 = 1个，1 = 2个）
		//   - 如果1个符号：4 bits 码长度 + 8 bits 符号值
		//   - 如果2个符号：4 bits 码长度 + 8 bits 符号值1 + 8 bits 符号值2
		// - 如果复杂编码：使用代码长度码（类似 Deflate）

		// 读取绿色 Huffman 树
		auto readVP8LHuffmanTree = [&reader](size_t numCodes) -> std::expected<std::vector<uint32_t>, std::string> {
			std::vector<uint32_t> bitlen(numCodes, 0);
			
			bool useSimpleCode = reader.readBits(1) == 0;
			
			if (useSimpleCode)
			{
				bool numSymbols = reader.readBits(1) == 1; // 0 = 1个符号，1 = 2个符号
				
				uint32_t codeLength = reader.readBits(4);
				if (codeLength == 0 || codeLength > MAX_HUFFMAN_BITS)
				{
					return std::unexpected("Invalid simple code length");
				}
				
				uint32_t symbol1 = reader.readBits(8);
				if (symbol1 >= numCodes)
				{
					return std::unexpected("Invalid symbol value");
				}
				bitlen[symbol1] = codeLength;
				
				if (numSymbols)
				{
					uint32_t symbol2 = reader.readBits(8);
					if (symbol2 >= numCodes)
					{
						return std::unexpected("Invalid symbol value");
					}
					bitlen[symbol2] = codeLength;
				}
			}
			else
			{
				// 复杂编码：使用代码长度码（类似 Deflate，但 VP8L 使用不同的格式）
				// VP8L 复杂编码格式：
				// - 5 bits: 代码长度码的数量 (HCLEN)
				// - HCLEN * 3 bits: 代码长度码的码长度
				// - 使用代码长度码解码实际的码长度
				
				uint32_t hclen = reader.readBits(5);
				if (hclen > 19)
				{
					return std::unexpected("Invalid HCLEN value");
				}
				
				// VP8L 代码长度码的顺序（与 Deflate 不同）
				constexpr uint8_t VP8L_CLCL_ORDER[19] = {
					17, 18, 0, 1, 2, 3, 4, 5, 16, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
				};
				
				// 读取代码长度码的码长度
				std::vector<uint32_t> bitlen_cl(19, 0);
				for (uint32_t i = 0; i < hclen; ++i)
				{
					uint32_t order = VP8L_CLCL_ORDER[i];
					if (order >= 19)
					{
						return std::unexpected("Invalid code length code order");
					}
					bitlen_cl[order] = reader.readBits(3);
				}
				
				// 构建代码长度码的 Huffman 树
				auto tree_cl_result = util::buildHuffmanTree(bitlen_cl.data(), 19, 7);
				if (!tree_cl_result.has_value())
				{
					return std::unexpected("Failed to build code length code tree: " + tree_cl_result.error());
				}
				auto tree_cl = tree_cl_result.value();
				
				// 解码实际的码长度
				uint32_t i = 0;
				constexpr uint32_t INVALIDSYMBOL = 0xFFFF;
				
				while (i < numCodes)
				{
					if (!reader.hasMoreData())
					{
						return std::unexpected("Insufficient data for code length decoding");
					}
					
					uint32_t code = util::huffmanDecodeSymbol(reader, tree_cl);
					
					if (code == INVALIDSYMBOL)
					{
						return std::unexpected("Failed to decode code length code");
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
						repeat = reader.readBits(2) + 3;
						if (i == 0)
						{
							return std::unexpected("Code length code 16 cannot be first");
						}
						value = bitlen[i - 1];
					}
					else if (code == 17)
					{
						// 重复 0 码长度 3-10 次
						repeat = reader.readBits(3) + 3;
						value = 0;
					}
					else if (code == 18)
					{
						// 重复 0 码长度 11-138 次
						repeat = reader.readBits(7) + 11;
						value = 0;
					}
					else
					{
						return std::unexpected("Invalid code length code: " + std::to_string(code));
					}
					
					// 填充码长度
					for (uint32_t j = 0; j < repeat && i < numCodes; ++j, ++i)
					{
						bitlen[i] = value;
					}
				}
			}
			
			return bitlen;
		};

		// 读取各个 Huffman 树
		auto greenBitlenResult = readVP8LHuffmanTree(NUM_LITERAL_CODES);
		if (!greenBitlenResult.has_value())
		{
			return std::unexpected("Failed to read green Huffman tree: " + greenBitlenResult.error());
		}
		auto greenBitlen = greenBitlenResult.value();

		auto redBitlenResult = readVP8LHuffmanTree(NUM_LITERAL_CODES);
		if (!redBitlenResult.has_value())
		{
			return std::unexpected("Failed to read red Huffman tree: " + redBitlenResult.error());
		}
		auto redBitlen = redBitlenResult.value();

		auto blueBitlenResult = readVP8LHuffmanTree(NUM_LITERAL_CODES);
		if (!blueBitlenResult.has_value())
		{
			return std::unexpected("Failed to read blue Huffman tree: " + blueBitlenResult.error());
		}
		auto blueBitlen = blueBitlenResult.value();

		std::vector<uint32_t> alphaBitlen;
		if (hasAlpha)
		{
			auto alphaBitlenResult = readVP8LHuffmanTree(NUM_LITERAL_CODES);
			if (!alphaBitlenResult.has_value())
			{
				return std::unexpected("Failed to read alpha Huffman tree: " + alphaBitlenResult.error());
			}
			alphaBitlen = alphaBitlenResult.value();
		}

		auto distanceBitlenResult = readVP8LHuffmanTree(NUM_DISTANCE_CODES);
		if (!distanceBitlenResult.has_value())
		{
			return std::unexpected("Failed to read distance Huffman tree: " + distanceBitlenResult.error());
		}
		auto distanceBitlen = distanceBitlenResult.value();

		// 构建 Huffman 树（使用通用实现）
		auto greenTreeResult = util::buildHuffmanTree(greenBitlen.data(), NUM_LITERAL_CODES, MAX_HUFFMAN_BITS);
		if (!greenTreeResult.has_value())
		{
			return std::unexpected("Failed to build green Huffman tree: " + greenTreeResult.error());
		}
		auto greenTree = greenTreeResult.value();

		auto redTreeResult = util::buildHuffmanTree(redBitlen.data(), NUM_LITERAL_CODES, MAX_HUFFMAN_BITS);
		if (!redTreeResult.has_value())
		{
			return std::unexpected("Failed to build red Huffman tree: " + redTreeResult.error());
		}
		auto redTree = redTreeResult.value();

		auto blueTreeResult = util::buildHuffmanTree(blueBitlen.data(), NUM_LITERAL_CODES, MAX_HUFFMAN_BITS);
		if (!blueTreeResult.has_value())
		{
			return std::unexpected("Failed to build blue Huffman tree: " + blueTreeResult.error());
		}
		auto blueTree = blueTreeResult.value();

		HuffmanTree alphaTree;
		if (hasAlpha)
		{
			auto alphaTreeResult = util::buildHuffmanTree(alphaBitlen.data(), NUM_LITERAL_CODES, MAX_HUFFMAN_BITS);
			if (!alphaTreeResult.has_value())
			{
				return std::unexpected("Failed to build alpha Huffman tree: " + alphaTreeResult.error());
			}
			alphaTree = alphaTreeResult.value();
		}

		auto distanceTreeResult = util::buildHuffmanTree(distanceBitlen.data(), NUM_DISTANCE_CODES, MAX_HUFFMAN_BITS);
		if (!distanceTreeResult.has_value())
		{
			return std::unexpected("Failed to build distance Huffman tree: " + distanceTreeResult.error());
		}
		auto distanceTree = distanceTreeResult.value();

		// 解码图像数据
		// VP8L 解码顺序：从左到右，从上到下
		// 每个像素使用预测模式 + Huffman 解码
		
		// 预分配输出缓冲区
		size_t pixelCount = _width * _height;
		output.resize(pixelCount * 4);
		
		// 颜色缓存索引
		uint32_t colorCacheIndex = 0;
		
		// 解码每个像素
		for (size_t pixelIdx = 0; pixelIdx < pixelCount; ++pixelIdx)
		{
			uint32_t x = pixelIdx % _width;
			uint32_t y = pixelIdx / _width;
			size_t outputIdx = pixelIdx * 4;
			
					// 读取预测模式（前3个像素使用特殊模式）
					uint32_t transformMode = 0;
					if (pixelIdx >= 3)
					{
						transformMode = reader.readBits(2);
					}
			
			// 解码像素值
			uint32_t argb = 0;
			
			// 检查是否使用颜色缓存
			bool useColorCache = false;
			if (hasColorCache && colorCacheSize > 0)
			{
				useColorCache = reader.readBits(1) != 0;
			}
			
			if (useColorCache)
			{
				// 从颜色缓存读取
				// 计算需要的位数（log2(colorCacheSize)）
				uint32_t cacheBits = 0;
				uint32_t tempSize = colorCacheSize;
				while (tempSize > 1)
				{
					++cacheBits;
					tempSize >>= 1;
				}
				
				uint32_t cacheIdx = reader.readBits(cacheBits);
				if (cacheIdx >= colorCacheSize)
				{
					return std::unexpected("Invalid color cache index");
				}
				argb = colorCache[cacheIdx];
			}
			else
			{
				// 解码绿色通道
				// VP8L 使用绿色通道的特殊值来表示 LZ77（绿色值 >= 256）
				uint32_t green = util::huffmanDecodeSymbol(reader, greenTree);
				if (green == 0xFFFF)
				{
					return std::unexpected("Failed to decode green channel");
				}
				
				// 检查是否是 LZ77 距离码（绿色值 >= 256 表示 LZ77）
				if (green >= 256)
				{
					// LZ77 距离码：复制之前的像素块
					uint32_t lengthCode = green - 256;
					
					// VP8L 长度码表（参考 WebP 规范）
					// 长度码 0-23: 直接映射到长度 1-24
					// 长度码 24-39: 需要额外位数
					uint32_t length = 0;
					
					if (lengthCode < 24)
					{
						length = lengthCode + 1;
					}
					else if (lengthCode < 40)
					{
						uint32_t extraBits = ((lengthCode - 24) >> 1) + 1;
						uint32_t extra = reader.readBits(extraBits);
						length = 25 + ((lengthCode - 24) << (extraBits - 1)) + extra;
					}
					else
					{
						return std::unexpected("Invalid length code");
					}
					
					// 解码距离
					uint32_t distanceCode = util::huffmanDecodeSymbol(reader, distanceTree);
					if (distanceCode == 0xFFFF)
					{
						return std::unexpected("Failed to decode distance code");
					}
					
					// VP8L 距离码表（参考 WebP 规范）
					// 距离码 0-3: 直接映射到距离 1-4
					// 距离码 4-39: 需要额外位数
					uint32_t distance = 0;
					
					if (distanceCode < 4)
					{
						distance = distanceCode + 1;
					}
					else if (distanceCode < 40)
					{
						uint32_t extraBits = ((distanceCode - 4) >> 1) + 1;
						uint32_t extra = reader.readBits(extraBits);
						distance = 5 + ((distanceCode - 4) << (extraBits - 1)) + extra;
					}
					else
					{
						return std::unexpected("Invalid distance code");
					}
					
					// 检查距离是否有效
					if (distance > pixelIdx || pixelIdx + length > pixelCount)
					{
						return std::unexpected("Invalid LZ77 distance or length");
					}
					
					size_t sourceIdx = pixelIdx - distance;
					
					// 复制像素块（使用临时变量避免修改外层循环的 pixelIdx）
					// 使用 (std::min) 避免 Windows 宏冲突
					uint32_t copyCount = static_cast<uint32_t>((std::min)(static_cast<size_t>(length), pixelCount - pixelIdx));
					for (uint32_t i = 0; i < copyCount; ++i)
					{
						size_t srcIdx = (sourceIdx + i) * 4;
						size_t dstIdx = (pixelIdx + i) * 4;
						
						if (srcIdx < output.size() && dstIdx < output.size())
						{
							// 使用 memcpy 复制 4 字节更高效
							std::memcpy(&output[dstIdx], &output[srcIdx], 4);
							
							// 更新颜色缓存
							if (hasColorCache && colorCacheSize > 0)
							{
								uint32_t copiedArgb = (output[dstIdx + 3] << 24) |
								                      (output[dstIdx + 0] << 16) |
								                      (output[dstIdx + 1] << 8) |
								                      output[dstIdx + 2];
								colorCache[colorCacheIndex % colorCacheSize] = copiedArgb;
								++colorCacheIndex;
							}
						}
					}
					
					// 更新 pixelIdx，跳过已复制的像素
					// 注意：需要减1，因为 for 循环会自动递增 pixelIdx
					pixelIdx += copyCount - 1;
					
					// 跳过当前循环的剩余部分，因为已经处理了多个像素
					continue;
				}
				
				// 正常解码：红色 -> 蓝色 -> Alpha（如果有）
				// 注意：绿色已经在前面解码了
				uint32_t red = util::huffmanDecodeSymbol(reader, redTree);
				if (red == 0xFFFF)
				{
					return std::unexpected("Failed to decode red channel");
				}
				
				uint32_t blue = util::huffmanDecodeSymbol(reader, blueTree);
				if (blue == 0xFFFF)
				{
					return std::unexpected("Failed to decode blue channel");
				}
				
				uint32_t alpha = 255;
				if (hasAlpha)
				{
					alpha = util::huffmanDecodeSymbol(reader, alphaTree);
					if (alpha == 0xFFFF)
					{
						return std::unexpected("Failed to decode alpha channel");
					}
				}
				
				argb = (alpha << 24) | (red << 16) | (green << 8) | blue;
				
				// 应用预测模式
				if (pixelIdx >= 3)
				{
					uint32_t prevPixel = 0;
					if (transformMode == 0)
					{
						// 无预测
					}
					else if (transformMode == 1)
					{
						// 使用左侧像素
						if (x > 0)
						{
							prevPixel = *reinterpret_cast<uint32_t*>(&output[(pixelIdx - 1) * 4]);
							argb = argb ^ prevPixel;
						}
					}
					else if (transformMode == 2)
					{
						// 使用上方像素
						if (y > 0)
						{
							prevPixel = *reinterpret_cast<uint32_t*>(&output[(pixelIdx - _width) * 4]);
							argb = argb ^ prevPixel;
						}
					}
					else if (transformMode == 3)
					{
						// 使用左上角像素
						if (x > 0 && y > 0)
						{
							prevPixel = *reinterpret_cast<uint32_t*>(&output[(pixelIdx - _width - 1) * 4]);
							argb = argb ^ prevPixel;
						}
					}
				}
				
				// 更新颜色缓存
				if (hasColorCache && colorCacheSize > 0)
				{
					colorCache[colorCacheIndex % colorCacheSize] = argb;
					++colorCacheIndex;
				}
			}
			
			// 写入输出缓冲区（优化：直接写入，避免多次位操作）
			*reinterpret_cast<uint32_t*>(&output[outputIdx]) = argb;
		}
		
		return {};
	}

	// ============================================================================
	// VP8 有损解码器实现
	// ============================================================================

	std::expected<void, std::string> webp::decodeVP8(const uint8_t* data, size_t size, std::vector<uint8_t>& output)
	{
		if (!data || size < 10)
		{
			return std::unexpected("Invalid VP8 data");
		}

		// VP8 格式：
		// - 帧头（3字节）：0x9D 0x01 0x2A（签名）
		// - 尺寸（2字节 + 2字节）：14位宽度 + 14位高度（小端序）
		// - 后续是 VP8 视频帧数据

		// 验证 VP8 签名
		if (data[0] != 0x9D || data[1] != 0x01 || data[2] != 0x2A)
		{
			return std::unexpected("Invalid VP8 signature");
		}

		// 读取尺寸（已在 extractFeatures 中读取，这里验证）
		// VP8 尺寸格式：2字节宽度 + 2字节高度（14位有效）
		std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(data), size);
		uint16_t widthBytes = util::read_le16(dataSpan, 3);
		uint16_t heightBytes = util::read_le16(dataSpan, 5);
		
		uint32_t vp8Width = (widthBytes & 0x3FFF);
		uint32_t vp8Height = (heightBytes & 0x3FFF);
		
		if (vp8Width != _width || vp8Height != _height)
		{
			return std::unexpected("VP8 dimensions mismatch");
		}

		// 创建位读取器（跳过帧头和尺寸，从第7字节开始）
		util::BitReader reader(data + 7, size - 7);

		// VP8 解码流程：
		// 1. 读取帧头信息（关键帧标志、版本等）
		// 2. 读取量化参数和滤波器信息
		// 3. 解码宏块（16x16 或 4x4）
		// 4. 应用预测和 DCT 逆变换
		// 5. YUV 到 RGB 转换

		// 读取关键帧标志
		bool keyFrame = reader.readBits(1) == 0;
		
		if (!keyFrame)
		{
			return std::unexpected("VP8 inter-frame decoding not yet supported");
		}

		// 读取版本号（3 bits）
		uint32_t version = reader.readBits(3);
		(void)version; // 暂时不使用

		// 读取显示标志（1 bit）
		bool showFrame = reader.readBits(1) != 0;
		(void)showFrame; // 暂时不使用

		// 读取分区大小（19 bits）
		uint32_t partitionSize = reader.readBits(19);
		(void)partitionSize; // 暂时不使用

		// 读取量化参数（一次性确保足够的位）
		reader.ensureBits(40); // 7+4+7+4+6+6 = 34 bits，确保40位足够
		uint32_t yAcQ = reader.readBits(7);
		uint32_t yDcQ = reader.readBits(4);
		uint32_t y2AcQ = reader.readBits(7);
		uint32_t y2DcQ = reader.readBits(4);
		uint32_t uvAcQ = reader.readBits(6);
		uint32_t uvDcQ = reader.readBits(6);

		// 读取滤波器信息
		bool filterType = reader.readBits(1) != 0;
		uint32_t loopFilterLevel = reader.readBits(6);
		uint32_t sharpnessLevel = reader.readBits(3);
		(void)filterType; (void)loopFilterLevel; (void)sharpnessLevel; // 暂时不使用
		
		// ============================================================================
		// VP8 Huffman 表读取
		// ============================================================================
		// VP8 使用固定的 Huffman 表，但可以从位流读取更新
		// VP8 有多个 Huffman 表：
		// - DC 表：Y_DC、Y2_DC、UV_DC
		// - AC 表：Y_AC、Y2_AC、UV_AC
		
		// VP8 Huffman 表结构
		struct VP8HuffmanTable
		{
			std::vector<uint32_t> codeLengths;  // 每个符号的码长度（最多16位）
			std::vector<uint32_t> symbols;      // 符号值
			util::HuffmanTree tree;             // 构建的 Huffman 树
			bool isValid = false;
		};
		
		// VP8 Huffman 表数组
		VP8HuffmanTable dcTables[3];  // Y_DC, Y2_DC, UV_DC
		VP8HuffmanTable acTables[3];  // Y_AC, Y2_AC, UV_AC
		
		// 读取 Huffman 表的函数（根据 VP8 规范 RFC 6386）
		// VP8 实际上使用概率模型，但在 WebP 中可能使用简化的 Huffman 编码
		auto readVP8HuffmanTable = [&reader](VP8HuffmanTable& table, uint32_t numSymbols) -> std::expected<void, std::string> {
			// VP8 Huffman 表格式（根据 VP8 规范）：
			// - 1 bit: 是否使用默认表（0 = 使用默认，1 = 读取新表）
			bool useDefault = reader.readBits(1) == 0;
			
			if (useDefault)
			{
				// 使用默认表：VP8 有预定义的默认概率模型
				// 对于 DC 表：默认使用均匀分布
				// 对于 AC 表：使用预定义的分布
				table.codeLengths.resize(numSymbols, 0);
				table.symbols.resize(numSymbols, 0);
				
				// 构建默认的码长度（简化：使用均匀分布）
				// 实际 VP8 使用概率模型，这里使用简化的 Huffman 编码
				uint32_t defaultLength = 8; // 默认码长度
				for (uint32_t i = 0; i < numSymbols; ++i)
				{
					table.codeLengths[i] = defaultLength;
					table.symbols.push_back(i);
				}
				
				// 构建 Huffman 树
				auto treeResult = util::buildHuffmanTree(table.codeLengths.data(), numSymbols, 15);
				if (!treeResult.has_value())
				{
					return std::unexpected("Failed to build default VP8 Huffman tree: " + treeResult.error());
				}
				table.tree = treeResult.value();
				table.isValid = true;
				return {};
			}
			
			// 读取新表：VP8 使用概率模型，但这里使用简化的 Huffman 编码
			// VP8 规范中，概率模型使用 8-bit 概率值
			// 这里简化为读取码长度
			table.codeLengths.clear();
			table.codeLengths.reserve(numSymbols);
			
			// VP8 概率模型格式（简化实现）：
			// - 对于每个符号，读取概率值（8 bits）
			// - 将概率值转换为码长度
			// 实际 VP8 使用布尔解码器，这里使用简化的 Huffman 编码
			// 优化：批量读取概率值以减少 ensureBits 调用
			// 使用 (std::min) 避免 Windows 宏冲突
			reader.ensureBits((std::min)(static_cast<size_t>(numSymbols * 8), static_cast<size_t>(32)));
			for (uint32_t i = 0; i < numSymbols; ++i)
			{
				uint32_t probability = reader.readBits(8);
				
				// 将概率值转换为码长度（简化映射）
				// 概率值越大，码长度越短
				uint32_t codeLength = 0;
				if (probability > 200)
				{
					codeLength = 1; // 高概率：短码
				}
				else if (probability > 150)
				{
					codeLength = 2;
				}
				else if (probability > 100)
				{
					codeLength = 4;
				}
				else if (probability > 50)
				{
					codeLength = 6;
				}
				else if (probability > 0)
				{
					codeLength = 8;
				}
				else
				{
					codeLength = 0; // 零概率：不使用
				}
				
				table.codeLengths.push_back(codeLength);
			}
			
			// 构建符号数组（符号值就是索引）
			table.symbols.clear();
			for (uint32_t i = 0; i < numSymbols; ++i)
			{
				if (table.codeLengths[i] > 0)
				{
					table.symbols.push_back(i);
				}
			}
			
			// 验证：至少需要一个有效符号
			if (table.symbols.empty())
			{
				return std::unexpected("VP8 Huffman table has no valid symbols");
			}
			
			// 构建 Huffman 树
			auto treeResult = util::buildHuffmanTree(table.codeLengths.data(), numSymbols, 15);
			if (!treeResult.has_value())
			{
				return std::unexpected("Failed to build VP8 Huffman tree: " + treeResult.error());
			}
			table.tree = treeResult.value();
			table.isValid = true;
			
			return {};
		};
		
		// 读取所有 Huffman 表
		// DC 表（12个符号：-11 到 0）
		constexpr uint32_t DC_TABLE_SIZE = 12;
		if (auto result = readVP8HuffmanTable(dcTables[0], DC_TABLE_SIZE); !result.has_value())
		{
			return std::unexpected<std::string>("Failed to read Y_DC table: " + result.error());
		}
		if (auto result = readVP8HuffmanTable(dcTables[1], DC_TABLE_SIZE); !result.has_value())
		{
			return std::unexpected<std::string>("Failed to read Y2_DC table: " + result.error());
		}
		if (auto result = readVP8HuffmanTable(dcTables[2], DC_TABLE_SIZE); !result.has_value())
		{
			return std::unexpected<std::string>("Failed to read UV_DC table: " + result.error());
		}
		
		// AC 表（162个符号：零游程 + 系数大小）
		constexpr uint32_t AC_TABLE_SIZE = 162;
		if (auto result = readVP8HuffmanTable(acTables[0], AC_TABLE_SIZE); !result.has_value())
		{
			return std::unexpected<std::string>("Failed to read Y_AC table: " + result.error());
		}
		if (auto result = readVP8HuffmanTable(acTables[1], AC_TABLE_SIZE); !result.has_value())
		{
			return std::unexpected<std::string>("Failed to read Y2_AC table: " + result.error());
		}
		if (auto result = readVP8HuffmanTable(acTables[2], AC_TABLE_SIZE); !result.has_value())
		{
			return std::unexpected<std::string>("Failed to read UV_AC table: " + result.error());
		}

		// VP8 宏块解码
		// VP8 图像分为宏块（16x16），每个宏块包含：
		// - 4 个 Y 块（8x8）
		// - 1 个 Y2 块（4x4，DC 系数）
		// - 2 个 U 块（4x4）
		// - 2 个 V 块（4x4）
		
		// 计算宏块数量
		uint32_t mbWidth = (_width + 15) / 16;
		uint32_t mbHeight = (_height + 15) / 16;
		
		// 预分配 YUV 缓冲区
		std::vector<int16_t> yPlane(_width * _height, 0);
		std::vector<int16_t> uPlane((_width / 2) * (_height / 2), 0);
		std::vector<int16_t> vPlane((_width / 2) * (_height / 2), 0);
		
		// VP8 使用简单的量化表（固定值）
		// 这里使用简化的量化实现
		auto dequantize = [](int16_t coeff, uint32_t q) -> int16_t {
			return static_cast<int16_t>((coeff * q) >> 7);
		};
		
		// VP8 4x4 IDCT（参考 VP8 规范）
		auto idct4x4 = [](std::array<int16_t, 16>& block) {
			// VP8 使用简化的 4x4 IDCT（Walsh-Hadamard 变换）
			// 列变换
			for (int col = 0; col < 4; ++col)
			{
				int16_t a = block[col * 4 + 0];
				int16_t b = block[col * 4 + 1];
				int16_t c = block[col * 4 + 2];
				int16_t d = block[col * 4 + 3];
				
				int16_t e = a + d;
				int16_t f = a - d;
				int16_t g = b + c;
				int16_t h = b - c;
				
				block[col * 4 + 0] = e + g;
				block[col * 4 + 1] = f + h;
				block[col * 4 + 2] = e - g;
				block[col * 4 + 3] = f - h;
			}
			
			// 行变换
			for (int row = 0; row < 4; ++row)
			{
				int16_t a = block[row * 4 + 0];
				int16_t b = block[row * 4 + 1];
				int16_t c = block[row * 4 + 2];
				int16_t d = block[row * 4 + 3];
				
				int16_t e = a + d;
				int16_t f = a - d;
				int16_t g = b + c;
				int16_t h = b - c;
				
				block[row * 4 + 0] = std::clamp((e + g + 32) >> 6, 0, 255);
				block[row * 4 + 1] = std::clamp((f + h + 32) >> 6, 0, 255);
				block[row * 4 + 2] = std::clamp((e - g + 32) >> 6, 0, 255);
				block[row * 4 + 3] = std::clamp((f - h + 32) >> 6, 0, 255);
			}
		};
		
		// VP8 8x8 IDCT（参考 JPEG 实现，但使用 VP8 的简化版本）
		auto idct8x8 = [](std::array<int16_t, 64>& block) -> std::array<int16_t, 64> {
			// VP8 使用简化的 8x8 IDCT（类似 JPEG 但更简单）
			#define VP8_IDCT_1D(s0, s1, s2, s3, s4, s5, s6, s7) \
				do { \
					int32_t a1 = s0 + s4; \
					int32_t b1 = s0 - s4; \
					int32_t a2 = s2 + s6; \
					int32_t b2 = (s2 - s6) * 2217 / 4096; \
					int32_t a3 = s1 + s7; \
					int32_t b3 = s1 - s7; \
					int32_t a4 = s3 + s5; \
					int32_t b4 = s3 - s5; \
					int32_t a5 = a1 + a2; \
					int32_t b5 = a1 - a2; \
					int32_t a6 = a3 + a4; \
					int32_t b6 = (a3 - a4) * 2217 / 4096; \
					int32_t a7 = b1 + b2; \
					int32_t b7 = b1 - b2; \
					int32_t a8 = (b3 + b4) * 2217 / 4096; \
					int32_t b8 = (b3 - b4) * 2217 / 4096; \
					s0 = (a5 + a6) >> 3; \
					s1 = (a7 + a8) >> 3; \
					s2 = (b5 + b6) >> 3; \
					s3 = (b7 + b8) >> 3; \
					s4 = (b5 - b6) >> 3; \
					s5 = (b7 - b8) >> 3; \
					s6 = (a5 - a6) >> 3; \
					s7 = (a7 - a8) >> 3; \
				} while (0)
			
			std::array<int16_t, 64> result{};
			std::array<int32_t, 64> temp{};
			
			// 列变换
			for (int col = 0; col < 8; ++col)
			{
				int32_t c0 = block[col * 8 + 0];
				int32_t c1 = block[col * 8 + 1];
				int32_t c2 = block[col * 8 + 2];
				int32_t c3 = block[col * 8 + 3];
				int32_t c4 = block[col * 8 + 4];
				int32_t c5 = block[col * 8 + 5];
				int32_t c6 = block[col * 8 + 6];
				int32_t c7 = block[col * 8 + 7];
				
				VP8_IDCT_1D(c0, c1, c2, c3, c4, c5, c6, c7);
				temp[col * 8 + 0] = c0;
				temp[col * 8 + 1] = c1;
				temp[col * 8 + 2] = c2;
				temp[col * 8 + 3] = c3;
				temp[col * 8 + 4] = c4;
				temp[col * 8 + 5] = c5;
				temp[col * 8 + 6] = c6;
				temp[col * 8 + 7] = c7;
			}
			
			// 行变换并输出
			for (int row = 0; row < 8; ++row)
			{
				int32_t t0 = temp[row * 8 + 0];
				int32_t t1 = temp[row * 8 + 1];
				int32_t t2 = temp[row * 8 + 2];
				int32_t t3 = temp[row * 8 + 3];
				int32_t t4 = temp[row * 8 + 4];
				int32_t t5 = temp[row * 8 + 5];
				int32_t t6 = temp[row * 8 + 6];
				int32_t t7 = temp[row * 8 + 7];
				
				VP8_IDCT_1D(t0, t1, t2, t3, t4, t5, t6, t7);
				
				result[row * 8 + 0] = static_cast<int16_t>(std::clamp(t0 + 128, 0, 255));
				result[row * 8 + 1] = static_cast<int16_t>(std::clamp(t1 + 128, 0, 255));
				result[row * 8 + 2] = static_cast<int16_t>(std::clamp(t2 + 128, 0, 255));
				result[row * 8 + 3] = static_cast<int16_t>(std::clamp(t3 + 128, 0, 255));
				result[row * 8 + 4] = static_cast<int16_t>(std::clamp(t4 + 128, 0, 255));
				result[row * 8 + 5] = static_cast<int16_t>(std::clamp(t5 + 128, 0, 255));
				result[row * 8 + 6] = static_cast<int16_t>(std::clamp(t6 + 128, 0, 255));
				result[row * 8 + 7] = static_cast<int16_t>(std::clamp(t7 + 128, 0, 255));
			}
			
			#undef VP8_IDCT_1D
			return result;
		};
		
		// ============================================================================
		// VP8 DCT 系数解码（使用 Huffman 表）
		// ============================================================================
		
		// VP8 DCT 系数解码函数（8x8 块）
		auto decodeDCTCoefficients = [&reader, &dcTables, &acTables, yDcQ, yAcQ](
			std::array<int16_t, 64>& block, uint32_t dcTableIdx, uint32_t acTableIdx) -> std::expected<void, std::string> {
			
			// 解码 DC 系数
			if (!reader.hasMoreData())
			{
				return std::unexpected("Insufficient data for DC coefficient");
			}
			
			int16_t dcValue = 0;
			if (dcTables[dcTableIdx].isValid)
			{
				// 使用 Huffman 表解码 DC 符号
				uint32_t dcSymbol = util::huffmanDecodeSymbol(reader, dcTables[dcTableIdx].tree);
				if (dcSymbol == 0xFFFF)
				{
					return std::unexpected("Failed to decode DC symbol");
				}
				
				// VP8 DC 符号解码（根据 RFC 6386）：
				// VP8 DC 系数使用有符号值，范围通常是 -1023 到 1023
				// 符号值表示需要读取的额外位数
				// - 符号值 0 = DC 值为 0
				// - 符号值 1-11 = 需要读取 1-11 位额外数据
				if (dcSymbol == 0)
				{
					dcValue = 0;
				}
				else if (dcSymbol <= 11)
				{
					uint32_t extraBits = dcSymbol;
					uint32_t extra = reader.readBits(extraBits);
					
					// VP8 使用有符号扩展：
					// - 如果最高位为 0，则为负数
					// - 如果最高位为 1，则为正数
					if (extraBits > 0)
					{
						uint32_t signBit = 1u << (extraBits - 1);
						if ((extra & signBit) == 0)
						{
							// 负数：补码表示
							dcValue = static_cast<int16_t>(-static_cast<int32_t>(extra));
						}
						else
						{
							// 正数
							dcValue = static_cast<int16_t>(extra);
						}
					}
					else
					{
						dcValue = 0;
					}
				}
				else
				{
					return std::unexpected("Invalid DC symbol value");
				}
			}
			
			// DC 逆量化
			block[0] = static_cast<int16_t>((dcValue * yDcQ) >> 7);
			
			// VP8 的 AC 系数解码
			// VP8 使用 zigzag 顺序
			constexpr uint8_t zigzag[64] = {
				0,  1,  5,  6, 14, 15, 27, 28,
				2,  4,  7, 13, 16, 26, 29, 42,
				3,  8, 12, 17, 25, 30, 41, 43,
				9, 11, 18, 24, 31, 40, 44, 53,
				10, 19, 23, 32, 39, 45, 52, 54,
				20, 22, 33, 38, 46, 51, 55, 60,
				21, 34, 37, 47, 50, 56, 59, 61,
				35, 36, 48, 49, 57, 58, 62, 63
			};
			
			// 初始化 AC 系数为 0
			for (int i = 1; i < 64; ++i)
			{
				block[zigzag[i]] = 0;
			}
			
			// 解码 AC 系数
			if (acTables[acTableIdx].isValid)
			{
				uint8_t acIndex = 1;
				while (acIndex < 64)
				{
					uint32_t acSymbol = util::huffmanDecodeSymbol(reader, acTables[acTableIdx].tree);
					if (acSymbol == 0xFFFF)
					{
						return std::unexpected("Failed to decode AC symbol");
					}
					
					// VP8 AC 符号格式（根据 RFC 6386）：
					// - 符号值 0 = EOB（End of Block）
					// - 符号值 1-161 = (零游程 << 4) | 系数大小
					//   零游程范围：0-15（4 bits）
					//   系数大小范围：0-11（4 bits，但实际使用 0-10）
					if (acSymbol == 0)
					{
						// EOB（End of Block）：块结束
						break;
					}
					
					if (acSymbol > 161)
					{
						return std::unexpected("Invalid AC symbol value");
					}
					
					// 解析零游程和系数大小
					uint8_t zeroRun = (acSymbol - 1) >> 4;  // 高 4 位：零游程（0-15）
					uint8_t acSize = ((acSymbol - 1) & 0x0F); // 低 4 位：系数大小（0-15，但通常 <= 10）
					
					// 跳过零游程
					acIndex += zeroRun;
					if (acIndex >= 64)
					{
						return std::unexpected("AC coefficient index out of range");
					}
					
					// 读取 AC 系数值
					if (acSize > 0 && acSize <= 11)
					{
						uint32_t extraBits = reader.readBits(acSize);
						
						// VP8 AC 系数使用有符号扩展（根据 RFC 6386）
						int16_t acValue = 0;
						if (acSize > 0)
						{
							uint32_t signBit = 1u << (acSize - 1);
							if ((extraBits & signBit) == 0)
							{
								// 负数：补码表示
								acValue = static_cast<int16_t>(-static_cast<int32_t>(extraBits));
							}
							else
							{
								// 正数
								acValue = static_cast<int16_t>(extraBits);
							}
						}
						
						// 将 AC 系数放入 zigzag 位置
						if (acIndex < 64)
						{
							block[zigzag[acIndex]] = acValue;
						}
					}
					
					// 移动到下一个位置
					acIndex++;
				}
			}
			
			// AC 逆量化
			for (int i = 1; i < 64; ++i)
			{
				block[i] = static_cast<int16_t>((block[i] * yAcQ) >> 7);
			}
			
			return {};
		};
		
		// VP8 DCT 系数解码函数（4x4 块）
		auto decodeDCTCoefficients4x4 = [&reader, &dcTables, &acTables, uvDcQ](
			std::array<int16_t, 16>& block, uint32_t dcTableIdx, uint32_t acTableIdx) -> std::expected<void, std::string> {
			
			// 解码 DC 系数
			if (!reader.hasMoreData())
			{
				return std::unexpected("Insufficient data for 4x4 DC coefficient");
			}
			
			int16_t dcValue = 0;
			if (dcTables[dcTableIdx].isValid)
			{
				uint32_t dcSymbol = util::huffmanDecodeSymbol(reader, dcTables[dcTableIdx].tree);
				if (dcSymbol == 0xFFFF)
				{
					return std::unexpected("Failed to decode 4x4 DC symbol");
				}
				
				// VP8 DC 符号解码（与 8x8 块相同）
				if (dcSymbol == 0)
				{
					dcValue = 0;
				}
				else if (dcSymbol <= 11)
				{
					uint32_t extraBits = dcSymbol;
					uint32_t extra = reader.readBits(extraBits);
					
					if (extraBits > 0)
					{
						uint32_t signBit = 1u << (extraBits - 1);
						if ((extra & signBit) == 0)
						{
							dcValue = static_cast<int16_t>(-static_cast<int32_t>(extra));
						}
						else
						{
							dcValue = static_cast<int16_t>(extra);
						}
					}
					else
					{
						dcValue = 0;
					}
				}
				else
				{
					return std::unexpected("Invalid 4x4 DC symbol value");
				}
			}
			
			block[0] = static_cast<int16_t>((dcValue * uvDcQ) >> 7);
			
			// AC 系数（4x4 块通常只有少量 AC 系数）
			for (int i = 1; i < 16; ++i)
			{
				block[i] = 0;
			}
			
			// 解码 AC 系数（简化：假设 4x4 块 AC 系数较少）
			if (acTables[acTableIdx].isValid)
			{
				uint8_t acIndex = 1;
				while (acIndex < 16)
				{
					uint32_t acSymbol = util::huffmanDecodeSymbol(reader, acTables[acTableIdx].tree);
					if (acSymbol == 0xFFFF)
					{
						return std::unexpected("Failed to decode 4x4 AC symbol");
					}
					
					if (acSymbol == 0)
					{
						break;
					}
					
					uint8_t zeroRun = (acSymbol >> 4) & 0x0F;
					uint8_t acSize = acSymbol & 0x0F;
					
					acIndex += zeroRun;
					if (acIndex >= 16)
					{
						break;
					}
					
					if (acSize != 0)
					{
						uint32_t extraBits = reader.readBits(acSize);
						
						int16_t acValue = 0;
						if (extraBits < (1u << (acSize - 1)))
						{
							acValue = static_cast<int16_t>(extraBits - (1u << acSize) + 1);
						}
						else
						{
							acValue = static_cast<int16_t>(extraBits);
						}
						
						block[acIndex] = acValue;
					}
					
					acIndex++;
				}
			}
			
			return {};
		};
		
		// ============================================================================
		// VP8 预测模式实现
		// ============================================================================
		
		// 预测模式应用函数
		auto applyIntraPrediction = [&](uint32_t mbX, uint32_t mbY, uint32_t mode, 
		                                std::array<int16_t, 64>& yBlock, int blockIdx) {
			// VP8 帧内预测模式
			// 0 = DC_PRED, 1 = TM_PRED, 2 = VE_PRED, 3 = HE_PRED
			
			uint32_t blockX = mbX * 16 + (blockIdx % 2) * 8;
			uint32_t blockY = mbY * 16 + (blockIdx / 2) * 8;
			
			if (mode == 0) // DC_PRED
			{
				// DC 预测：计算周围像素的平均值
				int32_t sum = 0;
				int32_t count = 0;
				
				// 上方像素
				if (blockY > 0)
				{
					for (int x = 0; x < 8; ++x)
					{
						uint32_t px = blockX + x;
						uint32_t py = blockY - 1;
						if (px < _width && py < _height)
						{
							sum += yPlane[py * _width + px];
							++count;
						}
					}
				}
				
				// 左侧像素
				if (blockX > 0)
				{
					for (int y = 0; y < 8; ++y)
					{
						uint32_t px = blockX - 1;
						uint32_t py = blockY + y;
						if (px < _width && py < _height)
						{
							sum += yPlane[py * _width + px];
							++count;
						}
					}
				}
				
				// 应用 DC 预测
				if (count > 0)
				{
					int16_t dcPred = static_cast<int16_t>(sum / count);
					for (int i = 0; i < 64; ++i)
					{
						yBlock[i] += dcPred;
					}
				}
			}
			else if (mode == 1) // TM_PRED (TrueMotion Prediction)
			{
				// TM 预测：使用左上角像素作为基准
				int16_t base = 128; // 默认值
				
				// 获取上方和左侧的参考像素
				if (blockY > 0 && blockX > 0)
				{
					// 左上角像素
					base = yPlane[(blockY - 1) * _width + (blockX - 1)];
				}
				else if (blockY > 0)
				{
					base = yPlane[(blockY - 1) * _width + blockX];
				}
				else if (blockX > 0)
				{
					base = yPlane[blockY * _width + (blockX - 1)];
				}
				
				// 应用 TM 预测：pred = base + top - left
				for (int y = 0; y < 8; ++y)
				{
					int16_t top = (blockY > 0 && (blockY + y - 1) < _height) ?
						yPlane[(blockY + y - 1) * _width + blockX] : base;
					
					for (int x = 0; x < 8; ++x)
					{
						int16_t left = (blockX > 0 && (blockX + x - 1) < _width) ?
							yPlane[(blockY + y) * _width + (blockX + x - 1)] : base;
						
						int idx = y * 8 + x;
						yBlock[idx] += static_cast<int16_t>(base + top - left);
					}
				}
			}
			else if (mode == 2) // VE_PRED (Vertical Prediction)
			{
				// 垂直预测：使用上方像素
				for (int y = 0; y < 8; ++y)
				{
					int16_t pred = 128; // 默认值
					
					if (blockY > 0)
					{
						uint32_t py = blockY + y - 1;
						if (py < _height)
						{
							// 使用上方对应列的像素
							pred = yPlane[py * _width + blockX];
						}
					}
					
					// 应用到整行
					for (int x = 0; x < 8; ++x)
					{
						int idx = y * 8 + x;
						yBlock[idx] += pred;
					}
				}
			}
			else if (mode == 3) // HE_PRED (Horizontal Prediction)
			{
				// 水平预测：使用左侧像素
				for (int x = 0; x < 8; ++x)
				{
					int16_t pred = 128; // 默认值
					
					if (blockX > 0)
					{
						uint32_t px = blockX + x - 1;
						if (px < _width)
						{
							// 使用左侧对应行的像素
							pred = yPlane[blockY * _width + px];
						}
					}
					
					// 应用到整列
					for (int y = 0; y < 8; ++y)
					{
						int idx = y * 8 + x;
						yBlock[idx] += pred;
					}
				}
			}
		};
		
		// 解码每个宏块
		for (uint32_t mbY = 0; mbY < mbHeight; ++mbY)
		{
			for (uint32_t mbX = 0; mbX < mbWidth; ++mbX)
			{
				// 读取宏块模式
				// VP8 使用简化的模式编码（实际需要从位流读取）
				uint32_t mbMode = reader.readBits(2); // 0-3: DC_PRED, TM_PRED, VE_PRED, HE_PRED
				
				// 解码 Y 块（4 个 8x8 块）
				for (int yBlock = 0; yBlock < 2; ++yBlock)
				{
					for (int xBlock = 0; xBlock < 2; ++xBlock)
					{
						std::array<int16_t, 64> yBlockData{};
						
						// 解码 DCT 系数（使用 Y_DC 和 Y_AC 表）
						auto coeffResult = decodeDCTCoefficients(yBlockData, 0, 0);
						if (!coeffResult.has_value())
						{
							return std::unexpected("Failed to decode Y block coefficients: " + coeffResult.error());
						}
						
						// 应用预测
						int blockIdx = yBlock * 2 + xBlock;
						applyIntraPrediction(mbX, mbY, mbMode, yBlockData, blockIdx);
						
						// IDCT
						auto idctResult = idct8x8(yBlockData);
						
						// 写入 Y 平面
						uint32_t baseX = mbX * 16 + xBlock * 8;
						uint32_t baseY = mbY * 16 + yBlock * 8;
						
						for (int y = 0; y < 8 && (baseY + y) < _height; ++y)
						{
							for (int x = 0; x < 8 && (baseX + x) < _width; ++x)
							{
								uint32_t px = baseX + x;
								uint32_t py = baseY + y;
								yPlane[py * _width + px] = idctResult[y * 8 + x];
							}
						}
					}
				}
				
				// 解码 Y2 块（1 个 4x4 块，DC 系数）
				std::array<int16_t, 16> y2Block{};
				auto y2Result = decodeDCTCoefficients4x4(y2Block, 1, 1); // 使用 Y2_DC 和 Y2_AC 表
				if (!y2Result.has_value())
				{
					return std::unexpected("Failed to decode Y2 block: " + y2Result.error());
				}
				idct4x4(y2Block);
				
				// 解码 U 块（2 个 4x4 块）
				for (int uBlock = 0; uBlock < 2; ++uBlock)
				{
					std::array<int16_t, 16> uBlockData{};
					auto uResult = decodeDCTCoefficients4x4(uBlockData, 2, 2); // 使用 UV_DC 和 UV_AC 表
					if (!uResult.has_value())
					{
						return std::unexpected("Failed to decode U block: " + uResult.error());
					}
					idct4x4(uBlockData);
					
					uint32_t baseX = mbX * 8 + uBlock * 4;
					uint32_t baseY = mbY * 8;
					
					for (int y = 0; y < 4 && (baseY + y) < (_height / 2); ++y)
					{
						for (int x = 0; x < 4 && (baseX + x) < (_width / 2); ++x)
						{
							uint32_t px = baseX + x;
							uint32_t py = baseY + y;
							uPlane[py * (_width / 2) + px] = uBlockData[y * 4 + x];
						}
					}
				}
				
				// 解码 V 块（2 个 4x4 块）
				for (int vBlock = 0; vBlock < 2; ++vBlock)
				{
					std::array<int16_t, 16> vBlockData{};
					auto vResult = decodeDCTCoefficients4x4(vBlockData, 2, 2); // 使用 UV_DC 和 UV_AC 表
					if (!vResult.has_value())
					{
						return std::unexpected("Failed to decode V block: " + vResult.error());
					}
					idct4x4(vBlockData);
					
					uint32_t baseX = mbX * 8 + vBlock * 4;
					uint32_t baseY = mbY * 8;
					
					for (int y = 0; y < 4 && (baseY + y) < (_height / 2); ++y)
					{
						for (int x = 0; x < 4 && (baseX + x) < (_width / 2); ++x)
						{
							uint32_t px = baseX + x;
							uint32_t py = baseY + y;
							vPlane[py * (_width / 2) + px] = vBlockData[y * 4 + x];
						}
					}
				}
			}
		}
		
		// YUV 到 RGB 转换（参考 JPEG 实现）
		output.resize(_width * _height * 4);
		for (uint32_t y = 0; y < _height; ++y)
		{
			for (uint32_t x = 0; x < _width; ++x)
			{
				uint32_t yIdx = y * _width + x;
				uint32_t uvIdx = (y / 2) * (_width / 2) + (x / 2);
				
				int16_t yVal = yPlane[yIdx];
				int16_t uVal = uvIdx < uPlane.size() ? uPlane[uvIdx] : 128;
				int16_t vVal = uvIdx < vPlane.size() ? vPlane[uvIdx] : 128;
				
				// YUV 到 RGB 转换（参考 JPEG）
				int32_t r = yVal + ((vVal - 128) * 1436) / 1024;
				int32_t g = yVal - ((uVal - 128) * 352) / 1024 - ((vVal - 128) * 731) / 1024;
				int32_t b = yVal + ((uVal - 128) * 1814) / 1024;
				
				uint32_t outIdx = yIdx * 4;
				output[outIdx + 0] = static_cast<uint8_t>(std::clamp(r, 0, 255)); // R
				output[outIdx + 1] = static_cast<uint8_t>(std::clamp(g, 0, 255)); // G
				output[outIdx + 2] = static_cast<uint8_t>(std::clamp(b, 0, 255)); // B
				output[outIdx + 3] = 255; // A
			}
		}
		
		return {};
	}

	// ============================================================================
	// ALPH Alpha 通道解码器实现
	// ============================================================================

	std::expected<void, std::string> webp::decodeALPH(const uint8_t* data, size_t size, std::vector<uint8_t>& output)
	{
		if (!data || size < 1)
		{
			return std::unexpected("Invalid ALPH data");
		}

		// ALPH 格式：
		// - 压缩方法（1字节）：0 = 无压缩，1 = WebP 无损压缩
		// - 后续是压缩的 Alpha 数据

		uint8_t compressionMethod = data[0];
		
		if (compressionMethod == 0)
		{
			// 无压缩：直接复制数据
			if (size - 1 != output.size())
			{
				return std::unexpected("ALPH uncompressed data size mismatch");
			}
			std::memcpy(output.data(), data + 1, output.size());
			return {};
		}
		else if (compressionMethod == 1)
		{
			// WebP 无损压缩：使用 VP8L 解码
			// ALPH 块使用 VP8L 格式，但只包含 Alpha 通道
			// 创建一个临时的 VP8L 数据，包含 Alpha 通道
			
			// VP8L Alpha 数据格式：
			// - 签名（1字节）：0x2E（低2位必须是 10，Alpha 标志位为 1）
			// - 尺寸（3字节）：14位宽度 + 14位高度
			// - VP8L 压缩的 Alpha 数据
			
			if (size < 5)
			{
				return std::unexpected("ALPH VP8L data too small");
			}
			
			// 构建 VP8L 格式的数据（只包含 Alpha）
			std::vector<uint8_t> vp8lData;
			vp8lData.reserve(size + 3);
			
			// VP8L 签名（包含 Alpha 标志）
			uint8_t signature = 0x2E | 0x10; // 0x2E + Alpha 标志
			vp8lData.push_back(signature);
			
			// 尺寸（14位宽度 + 14位高度）
			uint32_t width = _width;
			uint32_t height = _height;
			vp8lData.push_back(static_cast<uint8_t>(width & 0xFF));
			vp8lData.push_back(static_cast<uint8_t>((width >> 8) & 0xFF) | 
			                   static_cast<uint8_t>((height & 0x3F) << 6));
			vp8lData.push_back(static_cast<uint8_t>((height >> 6) & 0xFF));
			
			// 添加 VP8L 压缩的 Alpha 数据（跳过压缩方法字节）
			vp8lData.insert(vp8lData.end(), data + 1, data + size);
			
			// 使用 VP8L 解码器解码（但只提取 Alpha 通道）
			std::vector<uint8_t> rgbaData(_width * _height * 4);
			auto decodeResult = decodeVP8L(vp8lData.data(), vp8lData.size(), rgbaData);
			if (!decodeResult.has_value())
			{
				return std::unexpected("ALPH VP8L decoding failed: " + decodeResult.error());
			}
			
			// 提取 Alpha 通道（优化：直接内存访问）
			const uint8_t* rgbaSrc = rgbaData.data() + 3; // Alpha 通道偏移
			uint8_t* alphaDst = output.data();
			const size_t pixelCount = output.size();
			
			for (size_t i = 0; i < pixelCount; ++i)
			{
				alphaDst[i] = rgbaSrc[i * 4];
			}
			
			return {};
		}
		else
		{
			return std::unexpected("Unsupported ALPH compression method");
		}
	}

} // namespace shine::image

