#include "jpeg.h"

#include <array>
#include <algorithm>
#include <string_view>
#include <string>
#include <expected>
#include <bit>
#include <cmath>

#include "fmt/format.h"

#include "shine_define.h"
#include "util/timer/function_timer.h"
#include "util/file_util.ixx"

#include "util/encoding/byte_convert.ixx"
#include "util/encoding/bit_reader.ixx"



/**
 * @file jpeg.cpp
 * @brief JPEG 图像解码器实现
 * 
 * JPEG 格式规范：
 * - 文件头：2 字节签名 (FF D8)
 * - SOF 段：帧开始，包含图像尺寸和颜色分量信息
 * - DQT 段：定义量化表
 * - DHT 段：定义 Huffman 表
 * - SOS 段：扫描开始，包含压缩的图像数据
 * - EOI：文件结束标记 (FF D9)
 * 
 * @note JPEG 解码流程：
 * 1. 解析段（SOF, DQT, DHT, SOS）
 * 2. Huffman 解码扫描数据
 * 3. 反量化 DCT 系数
 * 4. IDCT（逆离散余弦变换）
 * 5. 颜色空间转换（YCbCr -> RGB）
 * 
 * @see ISO/IEC 10918-1 (JPEG Standard)
 */

namespace shine::image
{
	using namespace shine::util;

	// Overloads for std::span
	inline u8 read_u8(std::span<const std::byte> d, size_t o) {
		return shine::util::read_u8(reinterpret_cast<const unsigned char*>(d.data()), d.size(), o);
	}
	inline u16 read_u16(std::span<const std::byte> d, size_t o) {
		return shine::util::read_be16(reinterpret_cast<const unsigned char*>(d.data()), d.size(), o);
	}

	/**
	 * @brief 读取大端序数据引用
	 * @tparam T 数据类型
	 * @param d 数据缓冲区
	 * @param out 输出引用
	 * @param offset 偏移量
	 */
	template<typename T>
	void read_be_ref(std::span<const std::byte> d, T& out, size_t offset = 0) {
		if constexpr (sizeof(T) == 1) out = static_cast<T>(read_u8(d, offset));
		else if constexpr (sizeof(T) == 2) out = static_cast<T>(read_u16(d, offset));
		// else if constexpr (sizeof(T) == 4) out = static_cast<T>(read_u32(d, offset)); // JPEG usually doesn't need u32, but if needed add it
	}

	// ============================================================================
	// JPEG 格式常量定义
	// ============================================================================

	/// JPEG 文件头签名（SOI: Start of Image）
	static constexpr std::array jpeg_header{
		std::byte{0xFF}, std::byte{0xD8}
	};

	/// JPEG 段标记
	static constexpr uint8_t MARKER_SOI = 0xD8;      ///< Start of Image
	static constexpr uint8_t MARKER_EOI = 0xD9;      ///< End of Image
	static constexpr uint8_t MARKER_SOF0 = 0xC0;     ///< Start of Frame (Baseline DCT)
	static constexpr uint8_t MARKER_SOF1 = 0xC1;     ///< Start of Frame (Extended DCT)
	static constexpr uint8_t MARKER_SOF2 = 0xC2;     ///< Start of Frame (Progressive DCT)
	static constexpr uint8_t MARKER_DHT = 0xC4;       ///< Define Huffman Table
	static constexpr uint8_t MARKER_DQT = 0xDB;      ///< Define Quantization Table
	static constexpr uint8_t MARKER_SOS = 0xDA;      ///< Start of Scan
	static constexpr uint8_t MARKER_APP0 = 0xE0;     ///< Application Data (JFIF)
	static constexpr uint8_t MARKER_APP15 = 0xEF;    ///< Application Data
	static constexpr uint8_t MARKER_COM = 0xFE;       ///< Comment

	// ============================================================================
	// IAssetLoader 接口实现
	// ============================================================================

	std::string_view jpeg::getFileName() const noexcept
	{
		return _name;
	}

	bool jpeg::loadFromFile(const char* filePath)
	{
		setState(shine::loader::EAssetLoadState::READING_FILE);
		
		auto result = parseJpegFile(filePath);
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
	
	bool jpeg::loadFromMemory(const void* data, size_t size)
	{
		setState(shine::loader::EAssetLoadState::PARSING_DATA);
		
		if (!data || size == 0)
		{
			setError(shine::loader::EAssetLoaderError::INVALID_PARAMETER);
			setState(shine::loader::EAssetLoadState::FAILD);
			return false;
		}
		
		std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(data), size);
		
		// 验证 JPEG 文件格式
		if (!IsJpegFile(dataSpan))
		{
			setError(shine::loader::EAssetLoaderError::INVALID_FORMAT);
			setState(shine::loader::EAssetLoadState::FAILD);
			return false;
		}
		
		// 保存原始数据
		rawJpegData.assign(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size);
		
		setState(shine::loader::EAssetLoadState::PROCESSING);
		
		// 解析 JPEG 段
		auto parseResult = parseSegments(dataSpan);
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
	
	void jpeg::unload()
	{
		rawJpegData.clear();
		imageData.clear();
		scanData.clear();
		components.clear();
		scanComponentIds.clear();
		
		// 重置表
		for (auto& table : quantizationTables)
		{
			table.isValid = false;
		}
		for (auto& table : dcHuffmanTables)
		{
			table.isValid = false;
		}
		for (auto& table : acHuffmanTables)
		{
			table.isValid = false;
		}
		
		_width = 0;
		_height = 0;
		componentCount = 0;
		_loaded = false;
		setState(shine::loader::EAssetLoadState::NONE);
	}

	// ============================================================================
	// JPEG 文件解析
	// ============================================================================

	std::expected<void, std::string> jpeg::parseJpegFile(std::string_view filePath)
	{
		FunctionTimer timer("解析 JPEG 文件", TimerPrecision::Nanoseconds);

		setState(loader::EAssetLoadState::READING_FILE);
		
		// 读取文件内容
		auto result = read_full_file(SString::from_utf8(std::string(filePath)));
		if (!result.has_value())
		{
			setError(loader::EAssetLoaderError::FILE_NOT_FOUND, result.error());
			auto errorState = loader::EAssetLoadState::FAILD;
			setState(errorState);
			return std::unexpected(result.error());
		}

		setState(loader::EAssetLoadState::PARSING_DATA);
		
		auto file_data_view = std::move(result->view);

		// 验证 JPEG 文件格式
		if (!IsJpegFile(file_data_view.content))
		{
			setError(shine::loader::EAssetLoaderError::INVALID_FORMAT);
			auto errorState = shine::loader::EAssetLoadState::FAILD;
			setState(errorState);
			return std::unexpected("无效的 JPEG 文件格式");
		}

		setState(shine::loader::EAssetLoadState::PROCESSING);

		// 保存完整的 JPEG 文件数据用于后续解码
		rawJpegData.assign(
			reinterpret_cast<const uint8_t*>(file_data_view.content.data()),
			reinterpret_cast<const uint8_t*>(file_data_view.content.data()) + file_data_view.content.size()
		);

		// 解析 JPEG 段
		auto parseResult = parseSegments(file_data_view.content);
		if (!parseResult.has_value())
		{
			setError(shine::loader::EAssetLoaderError::PARSE_ERROR, parseResult.error());
			auto errorState = shine::loader::EAssetLoadState::FAILD;
			setState(errorState);
			return std::unexpected(parseResult.error());
		}

		return {};
	}

	constexpr bool jpeg::IsJpegFile(std::span<const std::byte> content) noexcept
	{
		// 检查文件大小（至少需要文件头）
		if (content.size() < 2)
		{
			return false;
		}

		// 验证 JPEG 文件头签名（SOI: 0xFF 0xD8）
		return content[0] == std::byte{0xFF} && content[1] == std::byte{0xD8};
	}

	// ============================================================================
	// JPEG 段解析
	// ============================================================================

	std::expected<void, std::string> jpeg::parseSegments(std::span<const std::byte> data)
	{
		if (data.size() < 2)
		{
			return std::unexpected("JPEG 数据太小");
		}

		// 验证 SOI 标记
		if (data[0] != std::byte{0xFF} || data[1] != std::byte{0xD8})
		{
			return std::unexpected("无效的 JPEG 文件头");
		}

		size_t pos = 2;  // 跳过 SOI 标记

		while (pos < data.size())
		{
			// 查找标记（0xFF）
			if (data[pos] != std::byte{0xFF})
			{
				// 如果没有找到标记，可能是扫描数据
				// 扫描数据会一直持续到 EOI 标记
				break;
			}

			pos++;
			if (pos >= data.size())
			{
				return std::unexpected("JPEG 数据不完整");
			}

			uint8_t marker = static_cast<uint8_t>(data[pos]);
			pos++;

			// 跳过填充字节（0xFF 0xFF）
			while (marker == 0xFF && pos < data.size())
			{
				marker = static_cast<uint8_t>(data[pos]);
				pos++;
			}

			// 检查 EOI 标记
			if (marker == MARKER_EOI)
			{
				break;
			}

			// 读取段长度（大端序，2 字节）
			if (pos + 2 > data.size())
			{
				return std::unexpected("JPEG 段长度数据不足");
			}

			uint16_t segmentLength = read_u16(data, pos);
			pos += 2;

			// 段长度包括长度字段本身（2 字节）
			if (segmentLength < 2)
			{
				return std::unexpected("无效的 JPEG 段长度");
			}

			size_t segmentDataLength = segmentLength - 2;  // 减去长度字段

			if (pos + segmentDataLength > data.size())
			{
				return std::unexpected("JPEG 段数据不完整");
			}

			std::span<const std::byte> segmentData = data.subspan(pos, segmentDataLength);

			// 根据标记类型解析段
			bool parseSuccess = false;
			switch (marker)
			{
			case MARKER_SOF0:  // Baseline DCT
			case MARKER_SOF1:  // Extended DCT
			case MARKER_SOF2:  // Progressive DCT
				parseSuccess = parseSOF(segmentData, segmentDataLength);
				break;

			case MARKER_DQT:
				parseSuccess = parseDQT(segmentData, segmentDataLength);
				break;

			case MARKER_DHT:
				parseSuccess = parseDHT(segmentData, segmentDataLength);
				break;

			case MARKER_SOS:
			{
				parseSuccess = parseSOS(segmentData, segmentDataLength);
				// SOS 之后是扫描数据，需要特殊处理
				pos += segmentDataLength;
				
				// 读取扫描数据（直到下一个标记或 EOI）
				size_t scanStart = pos;
				while (pos < data.size())
				{
					if (data[pos] == std::byte{0xFF})
					{
						// 检查是否是标记
						if (pos + 1 < data.size())
						{
							uint8_t nextByte = static_cast<uint8_t>(data[pos + 1]);
							if (nextByte != 0x00 && nextByte != 0xFF)
							{
								// 找到下一个标记，扫描数据结束
								break;
							}
						}
					}
					pos++;
				}
				
				// 保存扫描数据
				if (pos > scanStart)
				{
					scanData.assign(
						reinterpret_cast<const uint8_t*>(data.data() + scanStart),
						reinterpret_cast<const uint8_t*>(data.data() + pos)
					);
				}
				
				// 如果找到 EOI，退出循环
				if (pos + 1 < data.size() && data[pos] == std::byte{0xFF} && 
				    static_cast<uint8_t>(data[pos + 1]) == MARKER_EOI)
				{
					return {};
				}
				break;
			}

			case MARKER_APP0:
			case MARKER_APP15:
			case MARKER_COM:
				// 应用程序数据和注释段，暂时跳过
				break;

			default:
				// 未知段，跳过
				fmt::println("警告: 未知的 JPEG 段标记: 0x{:02X}", marker);
				break;
			}

			if (!parseSuccess && marker != MARKER_APP0 && marker != MARKER_APP15 && marker != MARKER_COM)
			{
				return std::unexpected(fmt::format("解析 JPEG 段失败: 标记 0x{:02X}", marker));
			}

			pos += segmentDataLength;
		}

		// 验证是否解析到了必要的段
		if (_width == 0 || _height == 0)
		{
			return std::unexpected("未找到 SOF 段或图像尺寸无效");
		}

		if (components.empty())
		{
			return std::unexpected("未找到颜色分量信息");
		}

		return {};
	}

	bool jpeg::parseSOF(std::span<const std::byte> data, size_t length)
	{
		if (length < 6)
		{
			return false;
		}

		// SOF 段格式：
		// 1 字节：精度（通常为 8）
		// 2 字节：图像高度（大端序）
		// 2 字节：图像宽度（大端序）
		// 1 字节：颜色分量数量
		// 每个分量 3 字节：ID, 采样因子, 量化表ID

		precision = read_u8(data, 0);
		read_be_ref(data, _height, 1);
		read_be_ref(data, _width, 3);
		componentCount = read_u8(data, 5);

		if (precision != 8)
		{
			fmt::println("警告: 不支持的 JPEG 精度: {} 位", precision);
			return false;
		}

		if (_width == 0 || _height == 0)
		{
			return false;
		}

		if (componentCount == 0 || componentCount > 4)
		{
			return false;
		}

		// 解析颜色分量信息
		components.clear();
		components.reserve(componentCount);

		size_t offset = 6;
		for (uint8_t i = 0; i < componentCount && offset + 3 <= length; ++i)
		{
			JpegComponent component;
			component.id = read_u8(data, offset);
			uint8_t sampling = read_u8(data, offset + 1);
			component.sampling.h = (sampling >> 4) & 0x0F;
			component.sampling.v = sampling & 0x0F;
			component.quantizationTableId = read_u8(data, offset + 2);

			components.push_back(component);
			offset += 3;
		}

		fmt::println("JPEG 图像信息: {}x{}, 精度={}, 分量数={}",
			_width, _height, precision, componentCount);

		return true;
	}

	bool jpeg::parseDQT(std::span<const std::byte> data, size_t length)
	{
		size_t pos = 0;

		while (pos < length)
		{
			if (pos + 1 > length)
			{
				return false;
			}

			uint8_t tableInfo = read_u8(data, pos);
			uint8_t tableId = tableInfo & 0x0F;
			uint8_t precision = (tableInfo >> 4) & 0x0F;  // 0=8位, 1=16位

			if (tableId >= 4)
			{
				return false;
			}

			pos++;

			// 量化表大小：8位精度为64字节，16位精度为128字节
			size_t tableSize = (precision == 0) ? 64 : 128;

			if (pos + tableSize > length)
			{
				return false;
			}

			JpegQuantizationTable& table = quantizationTables[tableId];

			if (precision == 0)
			{
				// 8 位精度
				for (size_t i = 0; i < 64; ++i)
				{
					table.coefficients[i] = read_u8(data, pos + i);
				}
			}
			else
			{
				// 16 位精度（大端序）
				for (size_t i = 0; i < 64; ++i)
				{
					table.coefficients[i] = read_u16(data, pos + i * 2);
				}
			}

			table.isValid = true;
			pos += tableSize;

			fmt::println("读取量化表: ID={}, 精度={}位", tableId, precision == 0 ? 8 : 16);
		}

		return true;
	}

	bool jpeg::parseDHT(std::span<const std::byte> data, size_t length)
	{
		size_t pos = 0;

		while (pos < length)
		{
			if (pos + 1 > length)
			{
				return false;
			}

			uint8_t tableInfo = read_u8(data, pos);
			uint8_t tableId = tableInfo & 0x0F;
			uint8_t tableClass = (tableInfo >> 4) & 0x0F;  // 0=DC, 1=AC

			if (tableId >= 4)
			{
				return false;
			}

			pos++;

			// 读取每个码长的符号数量（16 字节）
			if (pos + 16 > length)
			{
				return false;
			}

			JpegHuffmanTable& table = (tableClass == 0) ? dcHuffmanTables[tableId] : acHuffmanTables[tableId];

			table.codeLengths.resize(16);
			uint32_t totalSymbols = 0;

			for (size_t i = 0; i < 16; ++i)
			{
				table.codeLengths[i] = read_u8(data, pos + i);
				totalSymbols += table.codeLengths[i];
			}

			pos += 16;

			// 读取符号值
			if (pos + totalSymbols > length)
			{
				return false;
			}

			table.symbols.resize(totalSymbols);
			for (size_t i = 0; i < totalSymbols; ++i)
			{
				table.symbols[i] = read_u8(data, pos + i);
			}

			pos += totalSymbols;

			// 构建 Huffman 查找表
			buildHuffmanLookupTable(table);
			table.isValid = true;

			fmt::println("读取 Huffman 表: {}表, ID={}, 符号数={}",
				tableClass == 0 ? "DC" : "AC", tableId, totalSymbols);
		}

		return true;
	}

	bool jpeg::parseSOS(std::span<const std::byte> data, size_t length)
	{
		if (length < 2)
		{
			return false;
		}

		// SOS 段格式：
		// 1 字节：扫描中的分量数量
		// 每个分量 2 字节：分量ID, Huffman表选择
		// 3 字节：频谱选择开始, 频谱选择结束, 逐次逼近

		scanComponentCount = read_u8(data, 0);

		if (scanComponentCount == 0 || scanComponentCount > 4)
		{
			return false;
		}

		scanComponentIds.clear();
		scanComponentIds.reserve(scanComponentCount);

		size_t offset = 1;
		for (uint8_t i = 0; i < scanComponentCount && offset + 2 <= length; ++i)
		{
			uint8_t componentId = read_u8(data, offset);
			uint8_t huffmanTableSelect = read_u8(data, offset + 1);

			scanComponentIds.push_back(componentId);

			// 更新分量的 Huffman 表 ID
			for (auto& comp : components)
			{
				if (comp.id == componentId)
				{
					comp.huffmanDCTableId = huffmanTableSelect & 0x0F;
					comp.huffmanACTableId = (huffmanTableSelect >> 4) & 0x0F;
					break;
				}
			}

			offset += 2;
		}

		fmt::println("读取 SOS 段: 扫描分量数={}", scanComponentCount);

		return true;
	}

	// ============================================================================
	// Huffman 解码
	// ============================================================================

	void jpeg::buildHuffmanLookupTable(JpegHuffmanTable& table)
	{
		// 构建 Huffman 码值查找表
		// 使用更高效的查找表结构

		table.codes.clear();
		table.codes.resize(256, 0xFFFF);  // 初始化查找表，0xFFFF 表示无效

		uint16_t code = 0;
		size_t symbolIndex = 0;

		for (uint8_t length = 1; length <= 16; ++length)
		{
			uint8_t count = table.codeLengths[length - 1];
			for (uint8_t i = 0; i < count; ++i)
			{
				if (symbolIndex < table.symbols.size())
				{
					// 对于短码（<=8位），直接建立查找表
					if (length <= 8)
					{
						// 将 code 左移到 8 位位置，填充所有可能的低位组合
						uint16_t baseCode = code << (8 - length);
						uint16_t numEntries = 1u << (8 - length);
						for (uint16_t j = 0; j < numEntries; ++j)
						{
							uint16_t lookupCode = baseCode | j;
							if (lookupCode < 256)
							{
								table.codes[lookupCode] = (table.symbols[symbolIndex] << 8) | length;
							}
						}
					}
					symbolIndex++;
				}
				code++;
			}
			// 移动到下一个长度的起始码值
			if (count > 0)
			{
				code <<= 1;
			}
		}
	}

	int32_t jpeg::decodeHuffmanSymbol(util::BitReader& reader, const JpegHuffmanTable& table)
	{
		if (!table.isValid)
		{
			return -1;
		}

		// 先尝试快速查找（8位）
		uint16_t peekCode = reader.peekBits(8);
		if (peekCode < table.codes.size() && table.codes[peekCode] != 0xFFFF)
		{
			uint16_t entry = table.codes[peekCode];
			uint8_t length = entry & 0xFF;
			uint8_t symbol = (entry >> 8) & 0xFF;
			reader.advanceBits(length);
			return symbol;
		}

		// 慢速查找（超过8位）- 使用正确的 Huffman 解码算法
		uint16_t code = 0;
		size_t symbolIndex = 0;

		for (uint8_t length = 1; length <= 16; ++length)
		{
			code = (code << 1) | reader.readBits(1);

			uint8_t count = table.codeLengths[length - 1];
			if (count == 0)
			{
				continue;  // 该长度没有符号，继续
			}

			if (symbolIndex + count > table.symbols.size())
			{
				return -1;
			}

			// 计算该长度的起始码值
			// 对于长度 L，起始码值 = (前 L-1 长度的所有符号数) << 1
			uint16_t startCode = 0;
			for (uint8_t i = 0; i < length - 1; ++i)
			{
				if (i < table.codeLengths.size())
				{
					startCode = (startCode << 1) + table.codeLengths[i];
				}
			}
			startCode <<= 1;

			// 检查当前码值是否在该长度范围内
			if (code >= startCode && code < startCode + count)
			{
				size_t index = symbolIndex + (code - startCode);
				if (index < table.symbols.size())
				{
					return table.symbols[index];
				}
			}

			symbolIndex += count;
		}

		return -1;
	}

	// ============================================================================
	// JPEG 图像解码
	// ============================================================================

	std::expected<void, std::string> jpeg::decode()
	{
		return decodeInternal();
	}

	std::expected<void, std::string> jpeg::decodeInternal()
	{
		if (scanData.empty())
		{
			return std::unexpected("没有扫描数据可解码");
		}

		if (_width == 0 || _height == 0)
		{
			return std::unexpected("图像尺寸无效");
		}

		if (components.empty())
		{
			return std::unexpected("没有颜色分量信息");
		}

		util::FunctionTimer timer("JPEG 解码", util::TimerPrecision::Nanoseconds);

		// 1. Huffman 解码扫描数据
		auto decodeResult = decodeScanData(scanData);
		if (!decodeResult.has_value())
		{
			return std::unexpected("扫描数据解码失败: " + decodeResult.error());
		}

		std::vector<int16_t> mcuData = std::move(decodeResult.value());

		// 2. 计算 MCU 信息
		uint8_t maxHSampling = 0;
		uint8_t maxVSampling = 0;
		for (const auto& comp : components)
		{
			if (comp.sampling.h > maxHSampling) maxHSampling = comp.sampling.h;
			if (comp.sampling.v > maxVSampling) maxVSampling = comp.sampling.v;
		}

		uint32_t mcuWidth = maxHSampling * 8;
		uint32_t mcuHeight = maxVSampling * 8;
		uint32_t mcuCols = (_width + mcuWidth - 1) / mcuWidth;
		uint32_t mcuRows = (_height + mcuHeight - 1) / mcuHeight;

		// 3. 处理每个分量
		// 计算每个 MCU 中每个分量的块数
		std::vector<uint32_t> blocksPerMCUPerComponent(componentCount);
		uint32_t totalBlocksPerMCU = 0;
		for (uint32_t compIdx = 0; compIdx < componentCount; ++compIdx)
		{
			blocksPerMCUPerComponent[compIdx] = components[compIdx].sampling.h * components[compIdx].sampling.v;
			totalBlocksPerMCU += blocksPerMCUPerComponent[compIdx];
		}

		std::vector<std::vector<int16_t>> componentPixels(componentCount);

		// 处理每个分量
		for (uint32_t compIdx = 0; compIdx < componentCount; ++compIdx)
		{
			const auto& comp = components[compIdx];
			uint32_t blocksPerMCU = blocksPerMCUPerComponent[compIdx];
			uint32_t componentWidth = (_width * comp.sampling.h + maxHSampling - 1) / maxHSampling;
			uint32_t componentHeight = (_height * comp.sampling.v + maxVSampling - 1) / maxVSampling;
			uint32_t totalBlocks = blocksPerMCU * mcuCols * mcuRows;

			// 提取该分量的所有块数据（从 mcuData 中按 MCU 顺序提取）
			std::vector<int16_t> componentMCUData;
			componentMCUData.reserve(totalBlocks * 64);
			
			// mcuData 结构：MCU0[分量0块0...块N, 分量1块0...块M, ...], MCU1[...], ...
			// 需要提取：分量compIdx的所有块（跨所有MCU）
			for (uint32_t mcu = 0; mcu < mcuCols * mcuRows; ++mcu)
			{
				// 计算当前 MCU 中该分量之前的块数
				uint32_t blocksBefore = 0;
				for (uint32_t c = 0; c < compIdx; ++c)
				{
					blocksBefore += blocksPerMCUPerComponent[c];
				}
				
				// 提取该 MCU 中该分量的所有块
				size_t mcuStartPos = mcu * totalBlocksPerMCU * 64 + blocksBefore * 64;
				for (uint32_t block = 0; block < blocksPerMCU; ++block)
				{
					for (uint32_t i = 0; i < 64; ++i)
					{
						size_t pos = mcuStartPos + block * 64 + i;
						if (pos < mcuData.size())
						{
							componentMCUData.push_back(mcuData[pos]);
						}
					}
				}
			}

			auto dequantized = dequantize(componentMCUData, comp.id);
			if (dequantized.empty())
			{
				return std::unexpected(fmt::format("分量 {} 反量化失败", comp.id));
			}

			// IDCT 和重组
			componentPixels[compIdx].resize(componentWidth * componentHeight);
			uint32_t blockIdx = 0;

			for (uint32_t mcuRow = 0; mcuRow < mcuRows; ++mcuRow)
			{
				for (uint32_t mcuCol = 0; mcuCol < mcuCols; ++mcuCol)
				{
					for (uint32_t vBlock = 0; vBlock < comp.sampling.v; ++vBlock)
					{
						for (uint32_t hBlock = 0; hBlock < comp.sampling.h; ++hBlock)
						{
							// 提取 8x8 块
							std::array<int16_t, 64> block{};
							for (uint32_t i = 0; i < 64; ++i)
							{
								block[i] = dequantized[blockIdx * 64 + i];
							}

							// IDCT
							auto idctResult = idct(block);

							// 写入到分量图像
							uint32_t baseX = mcuCol * mcuWidth + hBlock * 8;
							uint32_t baseY = mcuRow * mcuHeight + vBlock * 8;

							for (uint32_t y = 0; y < 8; ++y)
							{
								uint32_t pixelY = baseY + y;
								if (pixelY >= componentHeight) break;

								for (uint32_t x = 0; x < 8; ++x)
								{
									uint32_t pixelX = baseX + x;
									if (pixelX >= componentWidth) break;

									uint32_t pixelIdx = pixelY * componentWidth + pixelX;
									componentPixels[compIdx][pixelIdx] = idctResult[y * 8 + x];
								}
							}

							blockIdx++;
						}
					}
				}
			}
		}

		// 4. 颜色空间转换
		imageData.clear();
		imageData.reserve(_width * _height * 4);

		if (componentCount == 1)
		{
			// 灰度图像
			convertGrayscaleToRGBA(componentPixels[0], imageData);
		}
		else if (componentCount == 3)
		{
			// YCbCr 图像
			// 需要上采样色度分量（如果采样因子不同）
			std::vector<int16_t> yData = componentPixels[0];
			std::vector<int16_t> cbData = componentPixels[1];
			std::vector<int16_t> crData = componentPixels[2];

			// 简单的上采样（最近邻）
			if (components[1].sampling.h < maxHSampling || components[1].sampling.v < maxVSampling)
			{
				uint32_t cbWidth = (_width * components[1].sampling.h + maxHSampling - 1) / maxHSampling;
				uint32_t cbHeight = (_height * components[1].sampling.v + maxVSampling - 1) / maxVSampling;
				
				std::vector<int16_t> upsampledCb(_width * _height);
				std::vector<int16_t> upsampledCr(_width * _height);

				for (uint32_t y = 0; y < _height; ++y)
				{
					uint32_t srcY = (y * cbHeight) / _height;
					if (srcY >= cbHeight) srcY = cbHeight - 1;

					for (uint32_t x = 0; x < _width; ++x)
					{
						uint32_t srcX = (x * cbWidth) / _width;
						if (srcX >= cbWidth) srcX = cbWidth - 1;

						uint32_t srcIdx = srcY * cbWidth + srcX;
						uint32_t dstIdx = y * _width + x;

						upsampledCb[dstIdx] = cbData[srcIdx];
						upsampledCr[dstIdx] = crData[srcIdx];
					}
				}

				cbData = std::move(upsampledCb);
				crData = std::move(upsampledCr);
			}

			convertYCbCrToRGBA(yData, cbData, crData, imageData);
		}
		else
		{
			return std::unexpected(fmt::format("不支持的分量数量: {}", componentCount));
		}

		// 裁剪到实际图像尺寸（如果 MCU 边界超出）
		// 注意：由于 MCU 对齐，imageData 可能包含超出边界的像素
		if (imageData.size() >= _width * _height * 4)
		{
			// 如果大小正好，不需要裁剪
			if (imageData.size() == _width * _height * 4)
			{
				// 大小正确，无需裁剪
			}
			else
			{
				// 需要裁剪：只保留前 _width * _height * 4 个字节
				imageData.resize(_width * _height * 4);
			}
		}

		return {};
	}

	std::expected<std::vector<uint8_t>, std::string> jpeg::decodeRGB()
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

	// ============================================================================
	// 辅助函数实现
	// ============================================================================

	std::expected<std::vector<int16_t>, std::string> jpeg::decodeScanData(
		std::span<const uint8_t> scanDataSpan)
	{
		if (scanDataSpan.empty())
		{
			return std::unexpected("扫描数据为空");
		}

		util::BitReader reader(scanDataSpan.data(), scanDataSpan.size());

		// 计算 MCU（最小编码单元）数量
		// MCU 大小取决于采样因子
		uint8_t maxHSampling = 0;
		uint8_t maxVSampling = 0;
		for (const auto& comp : components)
		{
			if (comp.sampling.h > maxHSampling) maxHSampling = comp.sampling.h;
			if (comp.sampling.v > maxVSampling) maxVSampling = comp.sampling.v;
		}

		uint32_t mcuWidth = maxHSampling * 8;
		uint32_t mcuHeight = maxVSampling * 8;
		uint32_t mcuCols = (_width + mcuWidth - 1) / mcuWidth;
		uint32_t mcuRows = (_height + mcuHeight - 1) / mcuHeight;
		uint32_t totalMCUs = mcuCols * mcuRows;

		// 每个 MCU 包含多个 8x8 块（根据采样因子）
		uint32_t blocksPerMCU = 0;
		for (const auto& comp : components)
		{
			blocksPerMCU += comp.sampling.h * comp.sampling.v;
		}

		std::vector<int16_t> mcuData;
		mcuData.reserve(totalMCUs * blocksPerMCU * 64);

		// DC 预测器（每个分量一个）
		std::vector<int16_t> dcPredictors(componentCount, 0);

		// 解码每个 MCU
		for (uint32_t mcu = 0; mcu < totalMCUs; ++mcu)
		{
			uint32_t componentIndex = 0;
			for (const auto& comp : components)
			{
				// 获取该分量的 Huffman 表
				if (comp.huffmanDCTableId >= 4 || comp.huffmanACTableId >= 4)
				{
					return std::unexpected(fmt::format("分量 {} 的 Huffman 表 ID 无效", comp.id));
				}

				const auto& dcTable = dcHuffmanTables[comp.huffmanDCTableId];
				const auto& acTable = acHuffmanTables[comp.huffmanACTableId];

				if (!dcTable.isValid || !acTable.isValid)
				{
					return std::unexpected(fmt::format("分量 {} 的 Huffman 表无效", comp.id));
				}

				// 解码该分量的所有块（根据采样因子）
				uint32_t blocksInComponent = comp.sampling.h * comp.sampling.v;
				for (uint32_t block = 0; block < blocksInComponent; ++block)
				{
					std::array<int16_t, 64> blockData{};
					
					// 解码 DC 系数
					int32_t dcSymbol = decodeHuffmanSymbol(reader, dcTable);
					if (dcSymbol < 0)
					{
						return std::unexpected("DC 系数解码失败");
					}

					int16_t dcValue = 0;
					if (dcSymbol != 0)
					{
						// 读取额外的位数
						uint32_t extraBits = reader.readBits(dcSymbol);
						// 扩展符号（如果最高位为0，则为负数）
						if (extraBits < (1u << (dcSymbol - 1)))
						{
							dcValue = static_cast<int16_t>(extraBits - (1u << dcSymbol) + 1);
						}
						else
						{
							dcValue = static_cast<int16_t>(extraBits);
						}
					}

					// DC 预测
					dcValue += dcPredictors[componentIndex];
					dcPredictors[componentIndex] = dcValue;
					blockData[0] = dcValue;

					// 解码 AC 系数（Zigzag 顺序）
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

					uint8_t acIndex = 1;
					while (acIndex < 64)
					{
						int32_t acSymbol = decodeHuffmanSymbol(reader, acTable);
						if (acSymbol < 0)
						{
							return std::unexpected("AC 系数解码失败");
						}

						if (acSymbol == 0)
						{
							// EOB（End of Block）
							break;
						}

						uint8_t zeroRun = (acSymbol >> 4) & 0x0F;
						uint8_t acSize = acSymbol & 0x0F;

						acIndex += zeroRun;
						if (acIndex >= 64)
						{
							return std::unexpected("AC 系数索引超出范围");
						}

						if (acSize != 0)
						{
							int16_t acValue = 0;
							uint32_t extraBits = reader.readBits(acSize);
							// 扩展符号（如果最高位为0，则为负数）
							if (extraBits < (1u << (acSize - 1)))
							{
								acValue = static_cast<int16_t>(extraBits - (1u << acSize) + 1);
							}
							else
							{
								acValue = static_cast<int16_t>(extraBits);
							}

							blockData[zigzag[acIndex]] = acValue;
						}

						acIndex++;
					}

					// 将块数据添加到 MCU 数据
					for (int16_t val : blockData)
					{
						mcuData.push_back(val);
					}
				}

				componentIndex++;
			}
		}

		return mcuData;
	}

	std::vector<int16_t> jpeg::dequantize(const std::vector<int16_t>& mcuData, uint8_t componentId)
	{
		// 查找对应的量化表
		const JpegComponent* comp = nullptr;
		for (const auto& c : components)
		{
			if (c.id == componentId)
			{
				comp = &c;
				break;
			}
		}

		if (!comp || comp->quantizationTableId >= 4)
		{
			return {};
		}

		const auto& qTable = quantizationTables[comp->quantizationTableId];
		if (!qTable.isValid)
		{
			return {};
		}

		// 反量化：每个 8x8 块
		// 注意：mcuData 中的数据已经是正常顺序（在 decodeScanData 中通过 zigzag[acIndex] 索引存储）
		std::vector<int16_t> dequantized;
		dequantized.reserve(mcuData.size());

		size_t blockCount = mcuData.size() / 64;
		for (size_t block = 0; block < blockCount; ++block)
		{
			// 直接反量化，数据已经是正常顺序
			for (uint8_t i = 0; i < 64; ++i)
			{
				int16_t quantizedValue = mcuData[block * 64 + i];
				dequantized.push_back(quantizedValue * static_cast<int16_t>(qTable.coefficients[i]));
			}
		}

		return dequantized;
	}

	std::array<int16_t, 64> jpeg::idct(const std::array<int16_t, 64>& coefficients)
	{
		// IDCT 1D 变换宏（参考 stb_image 实现）
		#define IDCT_1D(s0, s1, s2, s3, s4, s5, s6, s7) \
			do { \
				int32_t x0 = s0, x1 = s1, x2 = s2, x3 = s3, x4 = s4, x5 = s5, x6 = s6, x7 = s7; \
				int32_t x8 = x7 - x1; \
				int32_t x9 = x1 + x7; \
				int32_t xa = x5 - x3; \
				int32_t xb = x3 + x5; \
				int32_t xc = x0 - x4; \
				int32_t xd = x0 + x4; \
				int32_t xe = (x8 + xa) * 181; \
				x8 = (x8 - xa) * 181; \
				xa = x9 - xb; \
				xb = x9 + xb; \
				x9 = (xa + xc) * 181; \
				xa = (xa - xc) * 181; \
				xc = xd + xb; \
				xd = xd - xb; \
				xb = x6 + x2; \
				x2 = x6 - x2; \
				x6 = x2 + x8; \
				x2 = x2 - x8; \
				x8 = xb + x4; \
				x4 = xb - x4; \
				xb = x8 + xc; \
				x8 = x8 - xc; \
				xc = x4 + x6; \
				x4 = x4 - x6; \
				x6 = x2 + xa; \
				x2 = x2 - xa; \
				xa = x6 + x9; \
				x6 = x6 - x9; \
				x9 = xa + xc; \
				xa = xa - xc; \
				s0 = (x9 + xb) >> 10; \
				s7 = (x9 - xb) >> 10; \
				s1 = (xa + x8) >> 10; \
				s6 = (xa - x8) >> 10; \
				s2 = (x6 + x4) >> 10; \
				s5 = (x6 - x4) >> 10; \
				s3 = (x2 + xc) >> 10; \
				s4 = (x2 - xc) >> 10; \
			} while (0)

		std::array<int16_t, 64> result{};
		std::array<int32_t, 64> temp{};

		// 列变换
		for (int col = 0; col < 8; ++col)
		{
			int32_t c0 = coefficients[col * 8 + 0];
			int32_t c1 = coefficients[col * 8 + 1];
			int32_t c2 = coefficients[col * 8 + 2];
			int32_t c3 = coefficients[col * 8 + 3];
			int32_t c4 = coefficients[col * 8 + 4];
			int32_t c5 = coefficients[col * 8 + 5];
			int32_t c6 = coefficients[col * 8 + 6];
			int32_t c7 = coefficients[col * 8 + 7];
			
			// 如果列全为零，快速路径
			if (c1 == 0 && c2 == 0 && c3 == 0 && c4 == 0 &&
			    c5 == 0 && c6 == 0 && c7 == 0)
			{
				int32_t dcterm = c0 * 4;
				temp[col * 8 + 0] = dcterm;
				temp[col * 8 + 1] = dcterm;
				temp[col * 8 + 2] = dcterm;
				temp[col * 8 + 3] = dcterm;
				temp[col * 8 + 4] = dcterm;
				temp[col * 8 + 5] = dcterm;
				temp[col * 8 + 6] = dcterm;
				temp[col * 8 + 7] = dcterm;
			}
			else
			{
				IDCT_1D(c0, c1, c2, c3, c4, c5, c6, c7);
				temp[col * 8 + 0] = c0;
				temp[col * 8 + 1] = c1;
				temp[col * 8 + 2] = c2;
				temp[col * 8 + 3] = c3;
				temp[col * 8 + 4] = c4;
				temp[col * 8 + 5] = c5;
				temp[col * 8 + 6] = c6;
				temp[col * 8 + 7] = c7;
			}
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
			
			IDCT_1D(t0, t1, t2, t3, t4, t5, t6, t7);
			
			// 添加偏移并限制范围到 0-255
			result[row * 8 + 0] = static_cast<int16_t>(std::clamp(t0 + 128, 0, 255));
			result[row * 8 + 1] = static_cast<int16_t>(std::clamp(t1 + 128, 0, 255));
			result[row * 8 + 2] = static_cast<int16_t>(std::clamp(t2 + 128, 0, 255));
			result[row * 8 + 3] = static_cast<int16_t>(std::clamp(t3 + 128, 0, 255));
			result[row * 8 + 4] = static_cast<int16_t>(std::clamp(t4 + 128, 0, 255));
			result[row * 8 + 5] = static_cast<int16_t>(std::clamp(t5 + 128, 0, 255));
			result[row * 8 + 6] = static_cast<int16_t>(std::clamp(t6 + 128, 0, 255));
			result[row * 8 + 7] = static_cast<int16_t>(std::clamp(t7 + 128, 0, 255));
		}

		#undef IDCT_1D
		return result;
	}

	void jpeg::convertYCbCrToRGBA(
		const std::vector<int16_t>& yData,
		const std::vector<int16_t>& cbData,
		const std::vector<int16_t>& crData,
		std::vector<uint8_t>& output)
	{
		size_t pixelCount = yData.size();
		output.clear();
		output.reserve(pixelCount * 4);

		for (size_t i = 0; i < pixelCount; ++i)
		{
			int16_t y = yData[i];
			int16_t cb = cbData[i];
			int16_t cr = crData[i];

			// YCbCr 到 RGB 转换公式
			int32_t r = y + ((cr - 128) * 1436) / 1024;
			int32_t g = y - ((cb - 128) * 352) / 1024 - ((cr - 128) * 731) / 1024;
			int32_t b = y + ((cb - 128) * 1814) / 1024;

			// 限制范围到 0-255
			output.push_back(static_cast<uint8_t>(std::clamp(r, 0, 255)));
			output.push_back(static_cast<uint8_t>(std::clamp(g, 0, 255)));
			output.push_back(static_cast<uint8_t>(std::clamp(b, 0, 255)));
			output.push_back(255);  // Alpha
		}
	}

	void jpeg::convertGrayscaleToRGBA(
		const std::vector<int16_t>& grayData,
		std::vector<uint8_t>& output)
	{
		size_t pixelCount = grayData.size();
		output.clear();
		output.reserve(pixelCount * 4);

		for (size_t i = 0; i < pixelCount; ++i)
		{
			uint8_t gray = static_cast<uint8_t>(std::clamp(grayData[i], int16_t(0), int16_t(255)));
			output.push_back(gray);
			output.push_back(gray);
			output.push_back(gray);
			output.push_back(255);  // Alpha
		}
	}

} // namespace shine::image

