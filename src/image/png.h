#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <expected>
#include <span>
#include <optional>
#include <array>


namespace shine::image
{

	enum class PngColorType : uint8_t
	{
		GERY = 0,      // 灰度
		RGB = 2,       // 真彩色
		PALETTE = 3, // 索引色
		GERY_ALPHA = 4, // 灰度 + Alpha
		RGBA = 6            // RGBA
	};


	struct PngTime
	{
		uint16_t year;       // 年
		uint8_t  month;      // 月
		uint8_t  day;        // 日
		uint8_t  hour;       // 时
		uint8_t  minute;     // 分
		uint8_t  second;     // 秒
	};

	// PNG透明度信息结构体
	struct PngTransparency
	{
		// 灰度图像的透明色(颜色类型 0)
		uint16_t gray = 0;

		// RGB图像的透明色(颜色类型 2)
		uint16_t red = 0;
		uint16_t green = 0;
		uint16_t blue = 0;

		// 调色板图像的Alpha值数组(颜色类型 3)
		std::vector<uint8_t> paletteAlpha;

		bool hasTransparency = false;
		PngColorType colorType = PngColorType::RGBA;
	};

	struct PngBackground
	{
		uint16_t bg_red = 0;
		uint16_t bg_green = 0;
		uint16_t bg_blue = 0;
	};

	// PNG文本信息结构体
	struct PngTextInfo
	{
		std::string keyword;  // 关键字
		std::string text;     // 文本内容
	};

	// 图像显示的像素大小或宽高比
	struct PngPhysical
	{
		uint32_t x = 0;		// 每单位像素
		uint32_t y = 0;		// 每个单位像素
		uint8_t unit = 0;   //  单位标志， 0为未知，1为米
	};

	struct PngGamma
	{
		unsigned gamma = 0;
	};


	struct PngChrm
	{
		uint32_t chrm_white_x; /* White Point x times 100000 */
		uint32_t chrm_white_y; /* White Point y times 100000 */
		uint32_t chrm_red_x;   /* Red x times 100000 */
		uint32_t chrm_red_y;   /* Red y times 100000 */
		uint32_t chrm_green_x; /* Green x times 100000 */
		uint32_t chrm_green_y; /* Green y times 100000 */
		uint32_t chrm_blue_x;  /* Blue x times 100000 */
		uint32_t chrm_blue_y;  /* Blue y times 100000 */
	};

	struct PngSrgb
	{
		uint8_t srgb_intent  = 0;
	};


	struct PngCicp
	{
		uint8_t cicp_color_primaries = 0;
		uint8_t cicp_transfer_function = 0;
		uint8_t cicp_matrix_coefficients = 0;
		uint8_t cicp_video_full_range_flag = 0;

	};


	// https://www.w3.org/TR/png-3/#mDCV-chunk
	struct PngMdcv
	{
		uint16_t r_x  = 0;
		uint16_t r_y  = 0;
		uint16_t g_x  = 0;
		uint16_t g_y  = 0;
		uint16_t b_x  = 0;
		uint16_t b_y  = 0;
		uint16_t w_x  = 0;
		uint16_t w_y  = 0;
		uint32_t max_luminance = 0;
		uint32_t min_luminance = 0;
	};


	// https://www.w3.org/TR/png-3/#cLLI-chunk
	struct PngClli
	{
		uint32_t max_cll  = 0;
		uint32_t max_fall = 0;
	};

	// https://www.w3.org/TR/png-3/#11sBIT
	struct PngSbit
	{
		uint8_t r = 0;
		uint8_t g = 0;
		uint8_t b = 0;
		uint8_t a = 0;
	};


	class EXPORT_PNG png
	{

	public:

		png() = default;

	public:

		std::string_view   getName()   const noexcept;
		constexpr uint32_t getWidth()  const noexcept { return _width; }
		constexpr uint32_t getHeight() const noexcept { return _height; }

		const PngTransparency& getTransparency() const noexcept { return transparency; }

		// 透明度查询方法
		bool hasTransparency() const noexcept { return transparency.hasTransparency; }
		bool isPixelTransparent(uint32_t x, uint32_t y, uint16_t grayValue) const noexcept;
		bool isPixelTransparent(uint32_t x, uint32_t y, uint16_t r, uint16_t g, uint16_t b) const noexcept;


		std::expected<void, std::string> parsePngFile(std::string_view filePath);

		constexpr  bool IsPngFile(std::span<const std::byte> content) noexcept;


		bool read_plte(std::span<const std::byte> data, size_t length);
		bool read_trns(std::span<const std::byte> data, size_t length);
		bool read_bkgd(std::span<const std::byte> data, size_t length);
		bool read_text(std::span<const std::byte> data, size_t length);
		bool read_ztxt(std::span<const std::byte> data, size_t length);
		bool read_itxt(std::span<const std::byte> data, size_t length);
		bool read_phys(std::span<const std::byte> data, size_t length);
		bool read_gama(std::span<const std::byte> data, size_t length);
		bool read_chrm(std::span<const std::byte> data, size_t length);
		bool read_srgb(std::span<const std::byte> data, size_t length);
		bool read_cicp(std::span<const std::byte> data, size_t length);
		bool read_mdcv(std::span<const std::byte> data, size_t length);
		bool read_clli(std::span<const std::byte> data, size_t length);
		bool read_exif(std::span<const std::byte> data, size_t length);
		bool read_sbit(std::span<const std::byte> data, size_t length);

		// 获取文本信息
		const std::vector<PngTextInfo>& getTextInfos() const noexcept { return textInfos; }

		PngColorType getColorType() const noexcept{ return colorType; }


	private:

		std::string _name;

		uint32_t _width = 0;
		uint32_t _height = 0;

		uint8_t bitDepth  		  = {};
		uint8_t compressionMethod = {};
		uint8_t filterMethod      = {};
		uint8_t interlaceMethod   = {};
		
		// 颜色类型 , 0 灰色,2 真彩色,3 索引色,4 灰度+Alpha,6 RGBA
		PngColorType colorType = PngColorType::RGBA;



		size_t palettesize = 0;
		std::vector<std::array<uint8_t, 4>> palettColors;

		// 时间信息
		PngTime time;

		// 透明度信息
		PngTransparency transparency;

		// 背景颜色信息
		std::optional<PngBackground> background;

		std::optional<PngPhysical> phy;

		std::optional<PngGamma> gama;

		std::optional<PngChrm> chrm;

		std::optional<PngSrgb> srgb;

		std::optional<PngCicp> cicp;

		std::optional<PngMdcv> mdcv;

		std::optional<PngClli> clli;

		std::optional<PngSbit> sbit;

		// 文本信息
		std::vector<PngTextInfo> textInfos;
	};


}

