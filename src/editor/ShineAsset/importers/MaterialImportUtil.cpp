#include "MaterialImportUtil.h"

#include <chrono>
#include <fstream>

#include <fmt/chrono.h>

#include "AssetTypes.h"
#include "AssetUuidHelper.h"

namespace shine::editor::asset
{
    namespace
    {
        // Current UTC timestamp in ISO-8601 format.
        static std::string NowIso8601()
        {
            return fmt::format("{:%FT%TZ}",
                std::chrono::floor<std::chrono::seconds>(
                    std::chrono::system_clock::now()));
        }

        // Append a JSON key-value where value is a quoted string (STextView).
        static void AppendJsonStr(SString& out, STextView key, STextView value)
        {
            out += ",\"";
            out += key;
            out += "\":\"";
            out += value;
            out += "\"";
        }
    } // anonymous namespace

    // -----------------------------------------------------------------------
    bool WriteTextureBin(
        const std::filesystem::path&      path,
        uint32_t                          width,
        uint32_t                          height,
        const std::vector<unsigned char>& rgba)
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        constexpr uint32_t channels = 4u;
        out.write(reinterpret_cast<const char*>(&width),    sizeof(width));
        out.write(reinterpret_cast<const char*>(&height),   sizeof(height));
        out.write(reinterpret_cast<const char*>(&channels), sizeof(channels));
        out.write(reinterpret_cast<const char*>(rgba.data()),
                  static_cast<std::streamsize>(rgba.size()));
        return out.good();
    }

    // -----------------------------------------------------------------------
    AssetMetadata MakeTextureMeta(
        STextView uuid,
        STextView sourceFile,
        STextView name,
        STextView binFilename,
        uint32_t  width,
        uint32_t  height,
        bool      generateMipmaps,
        bool      sRGB)
    {
        AssetMetadata meta;
        meta.formatVersion    = "2.0";
        meta.asset.uuid       = uuid;
        meta.asset.type       = std::string(AssetTypeId::Texture);
        meta.asset.sourceFile = sourceFile;
        meta.asset.imported   = true;
        meta.asset.lastImportTime = NowIso8601();

        SString props("{\"width\":");
        props += std::to_string(width);
        props += ",\"height\":";
        props += std::to_string(height);
        props += ",\"channels\":4,\"generateMipmaps\":";
        props += (generateMipmaps ? "true" : "false");
        props += ",\"sRGB\":";
        props += (sRGB ? "true" : "false");
        props += ",\"binaryPath\":\"";
        props += binFilename;
        props += "\"}";

        SubAssetEntry sub;
        sub.uuid       = GenerateV7UUIDString().to_string();
        sub.type       = std::string(SubAssetTypeId::Texture);
        sub.name       = std::string(name.sv());
        sub.properties = glz::raw_json{ props.to_string() };
        meta.asset.subAssets.push_back(std::move(sub));
        return meta;
    }

    // -----------------------------------------------------------------------
    AssetMetadata MakeMaterialMeta(
        STextView                 uuid,
        STextView                 sourceFile,
        const MaterialImportData& data)
    {
        AssetMetadata meta;
        meta.formatVersion    = "2.0";
        meta.asset.uuid       = uuid;
        meta.asset.type       = std::string(AssetTypeId::Material);
        meta.asset.sourceFile = sourceFile;
        meta.asset.imported   = true;
        meta.asset.lastImportTime = NowIso8601();

        SString props("{\"name\":\"");
        props += data.name;
        props += "\"";
        AppendJsonStr(props, "alphaMode", data.alphaMode);
        props += ",\"doubleSided\":";
        props += (data.doubleSided ? "true" : "false");

        const auto& bcf = data.baseColorFactor;
        props += ",\"baseColorFactor\":[";
        props += std::to_string(bcf[0]); props += ",";
        props += std::to_string(bcf[1]); props += ",";
        props += std::to_string(bcf[2]); props += ",";
        props += std::to_string(bcf[3]); props += "]";

        props += ",\"metallicFactor\":";
        props += std::to_string(data.metallicFactor);
        props += ",\"roughnessFactor\":";
        props += std::to_string(data.roughnessFactor);

        if (!data.baseColorTextureUuid.empty())
            AppendJsonStr(props, "baseColorTextureUuid", data.baseColorTextureUuid);
        if (!data.metallicRoughnessTextureUuid.empty())
            AppendJsonStr(props, "metallicRoughnessTextureUuid", data.metallicRoughnessTextureUuid);
        if (!data.normalTextureUuid.empty())
            AppendJsonStr(props, "normalTextureUuid", data.normalTextureUuid);
        if (!data.emissiveTextureUuid.empty())
            AppendJsonStr(props, "emissiveTextureUuid", data.emissiveTextureUuid);

        props += "}";

        SubAssetEntry sub;
        sub.uuid       = GenerateV7UUIDString().to_string();
        sub.type       = std::string(SubAssetTypeId::Material);
        sub.name       = data.name.to_string();
        sub.properties = glz::raw_json{ props.to_string() };
        meta.asset.subAssets.push_back(std::move(sub));
        return meta;
    }

} // namespace shine::editor::asset
