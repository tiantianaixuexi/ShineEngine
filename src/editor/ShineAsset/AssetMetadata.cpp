#include "AssetMetadata.h"

#include <fstream>

namespace shine::editor::asset
{
    namespace
    {
        // Forward compatibility: ignore keys introduced by later format versions.
        constexpr glz::opts kReadOpts{ .error_on_unknown_keys = false };

        // Human-readable output for .sasset files.
        constexpr glz::opts kWriteOpts{ .prettify = true };

        // Read a file's entire content into `out`. Returns false on I/O failure.
        bool ReadFileContent(std::string_view path, std::string& out)
        {
            std::ifstream f(std::string(path), std::ios::binary);
            if (!f) return false;
            out.assign(std::istreambuf_iterator<char>(f), {});
            return true;
        }

        // Write `content` to `path`. Returns false on I/O failure.
        bool WriteFileContent(std::string_view path, std::string_view content)
        {
            std::ofstream f(std::string(path), std::ios::binary);
            if (!f) return false;
            f.write(content.data(), static_cast<std::streamsize>(content.size()));
            return f.good();
        }
    }

    glz::expected<AssetMetadata, glz::error_ctx>
    ReadAssetMetadata(std::string_view json)
    {
        AssetMetadata meta{};
        if (auto ec = glz::read<kReadOpts>(meta, json); ec)
            return glz::unexpected(ec);
        return meta;
    }

    glz::expected<AssetMetadata, glz::error_ctx>
    ReadAssetMetadataFile(std::string_view filePath)
    {
        std::string content;
        if (!ReadFileContent(filePath, content))
            return glz::unexpected(glz::error_ctx{0, glz::error_code::file_open_failure});
        return ReadAssetMetadata(content);
    }

    glz::expected<std::string, glz::error_ctx>
    WriteAssetMetadata(const AssetMetadata& meta)
    {
        return glz::write<kWriteOpts>(meta);
    }

    glz::expected<std::string, glz::error_ctx>
    WriteAssetMetadataFile(const AssetMetadata& meta, std::string_view filePath)
    {
        auto res = glz::write<kWriteOpts>(meta);
        if (!res) return res;
        if (!WriteFileContent(filePath, res.value()))
            return glz::unexpected(glz::error_ctx{0, glz::error_code::file_open_failure});
        return res;
    }

} // namespace shine::editor::asset
