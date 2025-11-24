#include "png.h"

#include <array>
#include <algorithm>
#include <string_view>
#include <string>
#include <expected>

#include "fmt/format.h"
#include "fast_float/fast_float.h"

#include "shine_define.h"
#include "util/timer/function_timer.h"
#include "util/file_util.h"

#include "util/encoding/byte_convert.h"


//////////// PNG格式
//// 文件头(8字节): 89 50 4E 47 0D 0A 1A 0A
//// IHDR 块结构: 长度 (4字节,大端序13) | 标识类型 (4字节) | Width(宽4字节) Height(高4字节) BitDepth(位深度1,2,4,8,16),1字节) ColorType(颜色类型,0-6,1字节) | CRC (4字节)
/// https://www.w3.org/TR/png-3/#abstract

namespace shine::image
{
	using namespace util;

	// png 文件头字节
	static constexpr std::array png_header{
		std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47},
		std::byte{0x0D}, std::byte{0x0A}, std::byte{0x1A}, std::byte{0x0A},
		std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x0D},
		std::byte{'I'} , std::byte{'H'} , std::byte{'D'} , std::byte{'R'}
	};

	static constexpr  std::array IDAT{ std::byte{'I'} ,std::byte{'D'}, std::byte{'A'}, std::byte{'T'} }; //数据块(IDAT)
	static constexpr  std::array PLTE{ std::byte{'P'}, std::byte{'L'}, std::byte{'T'}, std::byte{'E'} }; //调色板块 (PLTE)
	static constexpr  std::array TEXT{ std::byte{'t'}, std::byte{'E'}, std::byte{'X'}, std::byte{'T'} }; //文本块(tEXt)
	static constexpr  std::array TIME{ std::byte{'t'}, std::byte{'I'}, std::byte{'M'}, std::byte{'E'} }; //图片时间块(tIME)
	static constexpr  std::array IEND{ std::byte{'I'}, std::byte{'E'}, std::byte{'N'}, std::byte{'D'} }; //图片结束块(IEND)
	static constexpr  std::array TRNS{ std::byte{'t'}, std::byte{'R'}, std::byte{'N'}, std::byte{'S'} }; //调色板透明度块 (tRNS)
	static constexpr  std::array BKGD{ std::byte{'b'}, std::byte{'K'}, std::byte{'G'}, std::byte{'D'} }; //背景颜色块(bKGD)
	static constexpr  std::array ZTXT{ std::byte{'z'}, std::byte{'T'}, std::byte{'X'}, std::byte{'T'} }; //压缩文本块(zTXt)
	static constexpr  std::array ITXT{ std::byte{'i'}, std::byte{'T'}, std::byte{'X'}, std::byte{'T'} }; //国际化文本块(iTXt)
	static constexpr  std::array PHYS{ std::byte{'p'}, std::byte{'H'}, std::byte{'Y'}, std::byte{'S'} }; //物理像素块(pHYs)
	static constexpr  std::array GAMA{ std::byte{'g'}, std::byte{'A'}, std::byte{'M'}, std::byte{'A'} }; //伽马值块 (gAMA)
	static constexpr  std::array CHRM{ std::byte{'c'}, std::byte{'H'}, std::byte{'R'}, std::byte{'M'} }; //色度块(cHRM)
	static constexpr  std::array SRGB{ std::byte{'s'}, std::byte{'R'}, std::byte{'G'}, std::byte{'B'} }; //sRGB块(sRGB)
	static constexpr  std::array ICCP{ std::byte{'i'}, std::byte{'C'}, std::byte{'C'}, std::byte{'P'} }; //ICCP块(iCCP)
	static constexpr  std::array CICP{ std::byte{'c'}, std::byte{'I'}, std::byte{'C'}, std::byte{'P'} }; //CICP块(cICP)
	static constexpr  std::array MDCV{ std::byte{'m'}, std::byte{'D'}, std::byte{'C'}, std::byte{'V'} }; //主显示色彩体积块 (mDCV)
	static constexpr  std::array CLLI{ std::byte{'c'}, std::byte{'L'}, std::byte{'L'}, std::byte{'I'} }; //颜色亮度块(cLLI)
	static constexpr  std::array EXIF{ std::byte{'e'}, std::byte{'X'}, std::byte{'I'}, std::byte{'F'} }; //EXIF块(eXIf)
	static constexpr  std::array SBIT{ std::byte{'s'}, std::byte{'B'}, std::byte{'I'}, std::byte{'T'} }; //位深度块 (sBIT)

	template<typename T>
	constexpr  bool JudgeChunkType(std::span<const std::byte> content, T str) noexcept
	{
		return std::equal(str.begin(), str.end(), content.begin());
	}

	std::string_view png::getName() const noexcept
	{
		return _name;
	}

	std::expected<void, std::string> png::parsePngFile(std::string_view filePath)
	{

		util::FunctionTimer ___timer___("解析Png的时间", util::TimerPrecision::Nanoseconds);

		// 打开文件读取
		auto resule = util::read_full_file(filePath);
		if (!resule.has_value())
		{
			return std::unexpected(resule.error());
		}

		auto file_data_view = std::move(resule->view);

		std::string_view data_view(
			reinterpret_cast<const char*>(file_data_view.content.data()),
			file_data_view.content.size()
		);

		if (!IsPngFile(file_data_view.content))
		{
			return std::unexpected("解析 PNG 图片出错");
		}

		return   {};

	}

	constexpr bool png::IsPngFile(std::span<const std::byte> content) noexcept
	{

		if (content.size() < 32)
		{
			fmt::println("png 文件大小，小于32字节");
			return false;
		}

		// 字节比较前8字节
		if (!std::equal(png_header.begin(), png_header.end(), content.begin()))
		{
			fmt::println("PNG 文件头无效");
			return false;
		}

		// 4. 解析宽度和高度
		read_be_ref(content, _width, 16);               // 图片宽度
		read_be_ref(content, _height, 20);              // 图片高度
		read_be_ref(content, bitDepth, 24);             // 位深度
		read_be_ref(content, colorType, 25);            // 颜色类型
		read_be_ref(content, compressionMethod, 26);    // 压缩     , 0 标准 deflate
		read_be_ref(content, filterMethod, 27);         // 滤波     , 0 标准滤波  : 包含5种滤波算法
		read_be_ref(content, interlaceMethod, 28);      // 交织
		uint32_t crc = {};
		read_be_ref(content, crc, 29);                    // CRC 校验

		fmt::println("图片宽度: {}, 高度: {}", _width, _height);
		fmt::println("位深度: {}, 颜色类型: {}, 压缩: {}, 滤波: {}, 交织: {}, crc校验:{}", bitDepth, static_cast<u8>(colorType), compressionMethod, filterMethod, interlaceMethod, crc);

		if (_width == 0 || _height == 0)
		{
			fmt::println("PNG 图片宽度或高度为0");
			return false;
		}

		if (compressionMethod != 0)
		{
			fmt::println("PNG 图片的压缩位必须是0");
			return false;
		}

		switch (colorType)
		{
		case PngColorType::GERY:
			if (!(bitDepth == 1 || bitDepth == 2 || bitDepth == 4 || bitDepth == 8 || bitDepth == 16))
			{
				fmt::println("PNG 颜色类型为灰度时，位深度只能是1,2,4,8,16");
				return false;
			}
			break;
		case PngColorType::RGB:
			if (!(bitDepth == 8 || bitDepth == 16))
			{
				fmt::println("PNG 颜色类型为真彩色时，位深度只能是 8,16");
				return false;
			}
			break;
		case PngColorType::PALETTE:
			if (!(bitDepth == 1 || bitDepth == 2 || bitDepth == 4 || bitDepth == 8))
			{
				fmt::println("PNG 颜色类型为调色板时，位深度只能是 1,2,4,8");
				return false;
			}
			break;
		case PngColorType::GERY_ALPHA:
			if (!(bitDepth == 8 || bitDepth == 16))
			{
				fmt::println("PNG 颜色类型为灰度Alpha时，位深度只能是 8,16");
				return false;
			}
			break;
		case PngColorType::RGBA:
			if (!(bitDepth == 8 || bitDepth == 16))
			{
				fmt::println("PNG 颜色类型为RGBA 时，位深度只能是 8,16");
				return false;
			}
			break;
		default:;
		}

		bool isEnd = false;
		std::span<const std::byte> Data = content.subspan(33);

		// 5. 开始处理后续的块
		while (!isEnd)
		{
			unsigned chunkLength = 0;
			read_be_ref(Data, chunkLength);

			std::span<const std::byte> type = Data.subspan(4);
			std::span<const std::byte> chunkData = Data.subspan(8, chunkLength);

			if (JudgeChunkType(type, IDAT))
			{

				// 处理 PLTE 块
				// 必须出现在颜色类型3 中
				// 也可以出现在 2 和6 中
				// 它是不会出现在 0 和4 中

				fmt::println("长度: {},类型:{}", chunkLength, "IDAT");
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
			}else if (JudgeChunkType(type,ICCP))
			{
				//TODO : 释放内存
			}
			else if (JudgeChunkType(type, CICP))
			{
				read_cicp(chunkData, chunkLength);
			}else if (JudgeChunkType(type,MDCV))
			{
				read_mdcv(chunkData, chunkLength);
			}else if (JudgeChunkType(type,CLLI))
			{
				read_clli(chunkData, chunkLength);
			}else if (JudgeChunkType(type, EXIF))
			{
				read_exif(chunkData, chunkLength);
			}else if (JudgeChunkType(type,SBIT))
			{
				read_sbit(chunkData, chunkLength);
			}

			//4 字节长度 + 4 字节类型 + chunkLength 字节数据 + 4 字节 CRC
			Data = Data.subspan(12 + chunkLength);
		}

		if (interlaceMethod == 0)
		{
			
		}
		return true;
	}

	bool png::read_plte(std::span<const std::byte> data, size_t length)
	{
		// PNG调色板格式 每个颜色3字节(RGB)，最多256种颜色
		palettesize = length / 3;
		if (palettesize == 0 || palettesize > 256) return false;

		palettColors.clear();
		palettColors.reserve(palettesize);

		// 直接按RGB格式读取，每3字节为一颜色
		for (size_t i = 0; i < length; i += 3)
		{
			if (i + 2 >= length) break; // 防止越界

			const uint8_t r = read_u8(data, i * 3);
			const uint8_t g = read_u8(data, i * 3 + 1);
			const uint8_t b = read_u8(data, i * 3 + 2);

			// 存储为RGBA格式，最后一个字节留给tRNS块透明度
			palettColors.push_back({ r, g, b, 255 }); // 默认不透明
		}

		fmt::println("读取调色板 {} 种颜色", palettColors.size());
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

	// 透明度查询方法实现
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

		// TODO: 解压缩文本数据
		// 这里需要实现zlib解压缩
		// 暂时存储压缩数据作为占位符
		std::string decompressedText = "[压缩文本数据，未解压缩]";
		textInfos.emplace_back(std::string{ keywordView }, decompressedText);

			fmt::println("读取 zTXt 块: 关键字'{}', 压缩文本长度={}",
			keywordView, compressedDataLength);

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

			// TODO: 实现 zlib 解压缩
			const std::span<const std::byte> compressedData = data.subspan(textStart, textLength);
			textContent = "[压缩的UTF-8文本数据，未解压缩]";
			textInfos.emplace_back(std::string{ keywordView }, textContent);
			fmt::println("读取 iTXt 块: 关键字'{}', 语言='{}', 翻译='{}', 压缩文本长度={}",
				keywordView, languageTag, translatedKeyword, textLength);
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

#if defined(__AVX2__)

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
		chrm = PngChrm{
			read_u32(data,0),
			read_u32(data,4),
			read_u32(data,8),
	        read_u32(data,12),
		    read_u32(data,16),
		  	read_u32(data,20),
			read_u32(data,24),
			read_u32(data,26),
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

#if defined(__AVX2__)

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

		mdcv = PngMdcv{
			read_u8(data,0),
			 read_u8(data,2),
			read_u8(data,4),
			read_u8(data,6),
			read_u8(data,8),
			read_u8(data,10),
			read_u8(data,12),
			read_u8(data,14),
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
		uint8_t depth = colorType == PngColorType::PALETTE ? 8 : bitDepth;
		if (colorType == PngColorType::GERY)
		{
			const uint8_t g = read_u8(data, 0);
			sbit = PngSbit{
				g,g,g
			};
		}else if (colorType == PngColorType::RGB ||colorType == PngColorType::PALETTE)
		{
			sbit = PngSbit{
				read_u8(data,0),
				read_u8(data,1),
				read_u8(data,2)
			};
		}else if (colorType == PngColorType::GERY_ALPHA)
		{
			const uint8_t g = read_u8(data, 0);
			sbit = PngSbit{
				g,g,g,read_u8(data,1)
			};
		}else if ( colorType == PngColorType::RGBA)
		{
			sbit = PngSbit{
				read_u8(data,0),
				read_u8(data,1),
				read_u8(data,2),
				read_u8(data,3)
			};
		}

		return true;
	}
}



