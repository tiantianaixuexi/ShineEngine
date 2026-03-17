#include "jpeg.h"

#include <fstream>
#include <string>

#include "turbojpeg.h"

namespace shine::image
{

// 鈹€鈹€ helpers 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€

static std::expected<std::vector<uint8_t>, std::string> read_file_bytes(const char* path)
{
	std::ifstream f(path, std::ios::binary | std::ios::ate);
	if (!f) return std::unexpected(std::string("cannot open file: ") + path);
	const auto sz = f.tellg();
	if (sz <= 0) return std::unexpected(std::string("empty file: ") + path);
	std::vector<uint8_t> buf(static_cast<size_t>(sz));
	f.seekg(0);
	f.read(reinterpret_cast<char*>(buf.data()), sz);
	return buf;
}

// 鈹€鈹€ IAssetLoader 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€

bool jpeg::loadFromFile(const char* filePath)
{
	setState(loader::EAssetLoadState::READING_FILE);

	auto bytes = read_file_bytes(filePath);
	if (!bytes)
	{
		setError(loader::EAssetLoaderError::FILE_NOT_FOUND, bytes.error());
		setState(loader::EAssetLoadState::FAILD);
		return false;
	}
	_name = filePath;
	return loadFromMemory(bytes->data(), bytes->size());
}

bool jpeg::loadFromMemory(const void* data, size_t size)
{
	setState(loader::EAssetLoadState::PARSING_DATA);

	if (!data || size < 2)
	{
		setError(loader::EAssetLoaderError::INVALID_PARAMETER);
		setState(loader::EAssetLoadState::FAILD);
		return false;
	}
	auto* p = static_cast<const uint8_t*>(data);
	if (p[0] != 0xFF || p[1] != 0xD8)
	{
		setError(loader::EAssetLoaderError::INVALID_FORMAT);
		setState(loader::EAssetLoadState::FAILD);
		return false;
	}

	// 璇诲彇澶翠俊鎭紙瀹介珮锛?
	tjhandle h = tj3Init(TJINIT_DECOMPRESS);
	if (!h)
	{
		setError(loader::EAssetLoaderError::MEMORY_ALLOCATION_FAILED);
		setState(loader::EAssetLoadState::FAILD);
		return false;
	}
	if (tj3DecompressHeader(h, p, size) != 0)
	{
		std::string err = tj3GetErrorStr(h);
		tj3Destroy(h);
		setError(loader::EAssetLoaderError::PARSE_ERROR, err);
		setState(loader::EAssetLoadState::FAILD);
		return false;
	}
	_width  = static_cast<uint32_t>(tj3Get(h, TJPARAM_JPEGWIDTH));
	_height = static_cast<uint32_t>(tj3Get(h, TJPARAM_JPEGHEIGHT));
	tj3Destroy(h);

	_jpegBuf.assign(p, p + size);
	_loaded = true;
	setState(loader::EAssetLoadState::COMPLETE);
	return true;
}

void jpeg::unload()
{
	_jpegBuf.clear();
	_rgba.clear();
	_width = _height = 0;
	_loaded = false;
	setState(loader::EAssetLoadState::NONE);
}


std::expected<void, std::string> jpeg::decode()
{
	if (_jpegBuf.empty())
		return std::unexpected(std::string("no JPEG data loaded"));

	tjhandle h = tj3Init(TJINIT_DECOMPRESS);
	if (!h)
		return std::unexpected(std::string("tj3Init failed"));

	if (tj3DecompressHeader(h, _jpegBuf.data(), _jpegBuf.size()) != 0)
	{
		std::string err = tj3GetErrorStr(h);
		tj3Destroy(h);
		return std::unexpected(err);
	}

	_width  = static_cast<uint32_t>(tj3Get(h, TJPARAM_JPEGWIDTH));
	_height = static_cast<uint32_t>(tj3Get(h, TJPARAM_JPEGHEIGHT));

	_rgba.resize(static_cast<size_t>(_width) * _height * 4);

	if (tj3Decompress8(h, _jpegBuf.data(), _jpegBuf.size(),
	                    _rgba.data(), 0, TJPF_RGBA) != 0)
	{
		std::string err = tj3GetErrorStr(h);
		tj3Destroy(h);
		_rgba.clear();
		return std::unexpected(err);
	}
	tj3Destroy(h);
	return {};
}

std::expected<std::vector<uint8_t>, std::string> jpeg::decodeRGB()
{
	if (_jpegBuf.empty())
		return std::unexpected(std::string("no JPEG data loaded"));

	tjhandle h = tj3Init(TJINIT_DECOMPRESS);
	if (!h)
		return std::unexpected(std::string("tj3Init failed"));

	if (tj3DecompressHeader(h, _jpegBuf.data(), _jpegBuf.size()) != 0)
	{
		std::string err = tj3GetErrorStr(h);
		tj3Destroy(h);
		return std::unexpected(err);
	}

	int w = tj3Get(h, TJPARAM_JPEGWIDTH);
	int ht = tj3Get(h, TJPARAM_JPEGHEIGHT);
	std::vector<uint8_t> rgb(static_cast<size_t>(w) * ht * 3);

	if (tj3Decompress8(h, _jpegBuf.data(), _jpegBuf.size(),
	                    rgb.data(), 0, TJPF_RGB) != 0)
	{
		std::string err = tj3GetErrorStr(h);
		tj3Destroy(h);
		return std::unexpected(err);
	}
	tj3Destroy(h);
	return rgb;
}

} // namespace shine::image
