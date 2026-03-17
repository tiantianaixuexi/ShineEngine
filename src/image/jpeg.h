#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <expected>

#include "loader/core/loader.h"
#include "loader/image/image_loader.h"

namespace shine::image
{
	/// JPEG 图像解码器（基于 TurboJPEG API）
	class jpeg : public loader::IImageLoader
	{
	public:
		jpeg()
		{
			addSupportedExtension("jpg");
			addSupportedExtension("jpeg");
		}
		~jpeg() override = default;

		// IAssetLoader
		bool loadFromFile(const char* filePath) override;
		bool loadFromMemory(const void* data, size_t size) override;
		void unload() override;
		[[nodiscard]] const char* getName() const override { return "jpegLoader"; }
		[[nodiscard]] const char* getVersion() const override { return "2.0.0"; }

		// IImageLoader
		[[nodiscard]] std::string_view getFileName() const noexcept override { return _name; }
		[[nodiscard]] constexpr uint32_t getWidth()  const noexcept override { return _width; }
		[[nodiscard]] constexpr uint32_t getHeight() const noexcept override { return _height; }
		[[nodiscard]] bool isLoaded()  const noexcept override { return _loaded; }
		[[nodiscard]] bool isDecoded() const noexcept override { return !_rgba.empty(); }
		[[nodiscard]] const std::vector<uint8_t>& getImageData() const noexcept override { return _rgba; }

		std::expected<void, std::string> decode() override;
		std::expected<std::vector<uint8_t>, std::string> decodeRGB() override;

	private:
		std::string           _name;
		bool                  _loaded = false;
		uint32_t              _width  = 0;
		uint32_t              _height = 0;
		std::vector<uint8_t>  _jpegBuf;   // 原始 JPEG 数据
		std::vector<uint8_t>  _rgba;      // 解码后 RGBA
	};

} // namespace shine::image

