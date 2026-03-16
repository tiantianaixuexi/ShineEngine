#include "TextureAssetImporter.h"
#include "ImporterAutoRegistry.h"
#include "MaterialImportUtil.h"

#include <cstring>
#include <system_error>

#include "imgui/imgui.h"

#include "util/image_util.h"
#include "image/jpeg.h"

namespace shine::editor::asset
{
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
            auto imgResult = shine::util::load_image(ctx.sourceFile.string(), 4);
            if (!imgResult.has_value())
            {
                result.errorMessage = "Failed to load PNG: " + imgResult.error();
                return result;
            }
            width  = static_cast<uint32_t>(imgResult->width);
            height = static_cast<uint32_t>(imgResult->height);
            rgbaData.resize(imgResult->data.size());
            std::memcpy(rgbaData.data(), imgResult->data.data(), imgResult->data.size());
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
        const SString stem(ctx.sourceFile.stem().string());
        SString binFilename = stem;
        binFilename += ".bin";
        const auto binPath = ctx.outputSAssetPath.parent_path() / binFilename.c_str();

        {
            std::error_code ec;
            std::filesystem::create_directories(ctx.outputSAssetPath.parent_path(), ec);
            if (ec)
            {
                result.errorMessage = "Failed to create output directory: " +
                                      ctx.outputSAssetPath.parent_path().string();
                return result;
            }

            if (!WriteTextureBin(binPath, width, height, rgbaData))
            {
                result.errorMessage = "Failed to write texture binary: " + binPath.string();
                return result;
            }
        }

        // Build AssetMetadata via shared helper
        const SString rootUUID(ctx.rootUUID);
        const SString sourceStr(ctx.sourceFile.string());
        result.metadata = MakeTextureMeta(
            rootUUID, sourceStr, stem, binFilename, width, height,
            settings.generateMipmaps, settings.sRGB);
        result.metadata.asset.importSettings = ctx.savedImportSettings;

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
