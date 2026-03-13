#include "CookingPipeline.h"

#include "EditorAssetRegistry.h"

namespace shine::editor::asset
{
    void CookingPipeline::RegisterCooker(std::shared_ptr<IAssetCooker> cooker)
    {
        for (std::string_view typeId : cooker->SupportedTypeIds())
        {
            m_cookers[SString(typeId)] = cooker;
        }
    }

    std::size_t CookingPipeline::CookAll(
        EditorAssetRegistry& registry,
        ECookPlatform platform,
        const std::filesystem::path& contentRoot,
        const std::filesystem::path& outputDir)
    {
        std::size_t cooked = 0;

        registry.ForEach([&](const EditorAssetEntry& entry)
        {
            if (entry.isDangling)
                return;

            auto result = CookSingle(registry, entry.uuid, platform, contentRoot, outputDir);
            if (result.succeeded)
                ++cooked;
        });

        return cooked;
    }

    CookResult CookingPipeline::CookSingle(
        EditorAssetRegistry& registry,
        STextView uuid,
        ECookPlatform platform,
        const std::filesystem::path& contentRoot,
        const std::filesystem::path& outputDir)
    {
        const auto* entry = registry.Find(uuid);
        if (!entry || entry->isDangling)
            return { false, "Asset not found or dangling", {} };

        SString typeKey(entry->record.type);
        auto it = m_cookers.find(typeKey);
        if (it == m_cookers.end())
            return { false, "No cooker registered for type: " + entry->record.type, {} };

        auto& cooker = it->second;
        if (!cooker->SupportsPlafform(platform))
            return { false, "Cooker does not support target platform", {} };

        // Read the full metadata from disk
        auto metaResult = ReadAssetMetadataFile(entry->diskPath.sv());
        if (!metaResult)
            return { false, "Failed to read .sasset file: " + entry->diskPath.to_string(), {} };

        AssetCookContext ctx;
        ctx.metadata       = std::move(metaResult.value());
        ctx.platform       = platform;
        ctx.contentRoot    = contentRoot;
        ctx.outputDir      = outputDir;
        ctx.editorRegistry = &registry;

        return cooker->Cook(ctx);
    }

} // namespace shine::editor::asset
