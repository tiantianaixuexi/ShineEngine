#include "TextureAssetImporter.h"
#include "ImporterAutoRegistry.h"

#include <chrono>
#include <fstream>
#include <system_error>

#include <fmt/chrono.h>
#include "imgui/imgui.h"

#include "AssetTypes.h"
#include "AssetUuidHelper.h"

#include "image/png.h"
#include "image/jpeg.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Binary texture format written alongside the .sasset:
    //
    //    [width    : u32]
    //    [height   : u32]
    //    [channels : u32]  — always 4 (RGBA)
    //    [pixels   : u8 * width * height * channels]
    // -----------------------------------------------------------------------

    std::string_view TextureAssetImporter::GetName() const noexcept
    {
        return "Texture Importer";
    }

    std::vector<std::string_view> TextureAssetImporter::SupportedExtensions() const noexcept
    {
        return { ".png", ".jpg", ".jpeg" };
    }

    ImportResult TextureAssetImporter::Import(const AssetImportContext& ctx)
    {
        ImportResult result;

        // Parse import settings
        TextureImportSettings settings{};
        if (!ctx.savedImportSettings.str.empty())
        {
            if (auto parsed = ParseImportSettings<TextureImportSettings>(ctx.savedImportSettings))
                settings = *parsed;
        }

        if (ctx.onProgress)
            ctx.onProgress("Loading texture file...", 0.0f);

        const std::string ext = ctx.sourceFile.extension().string();
        // Normalise extension to lower-case for comparison
        std::string extLower = ext;
        for (char& c : extLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        // Decode to RGBA using the appropriate built-in loader
        uint32_t width   = 0;
        uint32_t height  = 0;
        std::vector<uint8_t> rgbaData;

        if (extLower == ".png")
        {
            shine::image::png pngLoader;
            if (!pngLoader.loadFromFile(ctx.sourceFile.string().c_str()))
            {
                result.errorMessage = "Failed to load PNG file: " + ctx.sourceFile.string();
                return result;
            }
            if (auto dec = pngLoader.decode(); !dec)
            {
                result.errorMessage = "Failed to decode PNG: " + dec.error();
                return result;
            }
            width    = pngLoader.getWidth();
            height   = pngLoader.getHeight();
            rgbaData = pngLoader.getImageData();
        }
        else if (extLower == ".jpg" || extLower == ".jpeg")
        {
            shine::image::jpeg jpegLoader;
            if (!jpegLoader.loadFromFile(ctx.sourceFile.string().c_str()))
            {
                result.errorMessage = "Failed to load JPEG file: " + ctx.sourceFile.string();
                return result;
            }
            if (auto dec = jpegLoader.decode(); !dec)
            {
                result.errorMessage = "Failed to decode JPEG: " + dec.error();
                return result;
            }
            width    = jpegLoader.getWidth();
            height   = jpegLoader.getHeight();
            rgbaData = jpegLoader.getImageData();
        }
        else
        {
            result.errorMessage = "Unsupported texture format: " + ext;
            return result;
        }

        if (rgbaData.empty())
        {
            result.errorMessage = "Decoded texture data is empty: " + ctx.sourceFile.string();
            return result;
        }

        if (ctx.onProgress)
            ctx.onProgress("Writing texture binary...", 0.6f);

        // Write binary pixel data alongside the .sasset
        const std::string stem         = ctx.sourceFile.stem().string();
        const std::string binFilename  = stem + ".bin";
        const auto        binPath      = ctx.outputSAssetPath.parent_path() / binFilename;

        {
            std::error_code ec;
            std::filesystem::create_directories(ctx.outputSAssetPath.parent_path(), ec);
            if (ec)
            {
                result.errorMessage = "Failed to create output directory: " +
                                      ctx.outputSAssetPath.parent_path().string();
                return result;
            }

            std::ofstream out(binPath, std::ios::binary | std::ios::trunc);
            if (!out)
            {
                result.errorMessage = "Failed to write texture binary: " + binPath.string();
                return result;
            }

            constexpr uint32_t channels = 4u;
            out.write(reinterpret_cast<const char*>(&width),    sizeof(width));
            out.write(reinterpret_cast<const char*>(&height),   sizeof(height));
            out.write(reinterpret_cast<const char*>(&channels), sizeof(channels));
            out.write(reinterpret_cast<const char*>(rgbaData.data()),
                      static_cast<std::streamsize>(rgbaData.size()));

            if (!out.good())
            {
                result.errorMessage = "I/O error while writing texture binary: " + binPath.string();
                return result;
            }
        }

        // Build AssetMetadata
        auto& meta  = result.metadata;
        meta.formatVersion = "2.0";

        auto& asset = meta.asset;
        asset.uuid           = ctx.rootUUID;
        asset.type           = std::string(AssetTypeId::Texture);
        asset.sourceFile     = ctx.sourceFile.string();
        asset.imported       = true;
        asset.lastImportTime = fmt::format("{:%FT%TZ}",
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
        asset.importSettings = ctx.savedImportSettings;

        // Single sub-asset representing the raw texture
        {
            SubAssetEntry sub;
            sub.uuid = GenerateV7UUIDString().to_string();
            sub.type = std::string(SubAssetTypeId::Texture);
            sub.name = stem;

            const std::string props =
                "{\"width\":"        + std::to_string(width)   +
                ",\"height\":"       + std::to_string(height)  +
                ",\"channels\":4"
                ",\"generateMipmaps\":" + (settings.generateMipmaps ? "true" : "false") +
                ",\"sRGB\":"            + (settings.sRGB ? "true" : "false") +
                ",\"binaryPath\":\""    + binFilename + "\"}";
            sub.properties = glz::raw_json{ props };

            asset.subAssets.push_back(std::move(sub));
        }

        if (ctx.onProgress)
            ctx.onProgress("Import complete", 1.0f);

        result.succeeded = true;
        return result;
    }

    bool TextureAssetImporter::RenderImportSettingsUI(glz::raw_json& inOutSettings)
    {
        TextureImportSettings settings{};
        if (!inOutSettings.str.empty())
            (void)glz::read_json(settings, inOutSettings.str);

        bool changed = false;
        changed |= ImGui::Checkbox("生成 Mipmap", &settings.generateMipmaps);
        changed |= ImGui::Checkbox("sRGB 色彩空间", &settings.sRGB);

        if (changed)
        {
            if (auto r = SerializeImportSettings(settings); r)
                inOutSettings = std::move(*r);
        }
        return changed;
    }

    REGISTER_IMPORTER(TextureAssetImporter)

} // namespace shine::editor::asset
