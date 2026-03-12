#include "editor/asset/Cooking/AssetRegistryBuilder.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "util/file_util.ixx"
#include "util/timer/TimerUtil.h"

namespace shine::editor::asset
{
    namespace
    {
        [[nodiscard]] SString MakeError(STextView text)
        {
            return SString::from_view(text);
        }

        [[nodiscard]] std::uint64_t ResolveBuildTimestamp(std::uint64_t buildTimestamp) noexcept
        {
            if (buildTimestamp != 0)
            {
                return buildTimestamp;
            }

            return static_cast<std::uint64_t>(
                shine::util::get_now_ms_platform<unsigned long long>());
        }
    }

    std::uint32_t AssetRegistryBuilder::BuildEntryFlags(const AssetCookProfile& cookProfile) noexcept
    {
        auto flags = static_cast<std::uint32_t>(shine::ERegistryEntryFlags::None);

        if (cookProfile.compressed)
        {
            flags |= static_cast<std::uint32_t>(shine::ERegistryEntryFlags::Compressed);
        }

        if (cookProfile.streamable)
        {
            flags |= static_cast<std::uint32_t>(shine::ERegistryEntryFlags::Streamable);
        }

        return flags;
    }

    std::vector<RuntimeRegistryBuildItem> AssetRegistryBuilder::CollectBuildItems(
        const AssetRegistry& registry)
    {
        std::vector<RuntimeRegistryBuildItem> items;

        const auto& records = registry.GetAllRecords();
        items.reserve(records.size());

        for (const auto& record : records)
        {
            if (!record.IsValid())
            {
                continue;
            }

            if (!record.buildState.cooked)
            {
                continue;
            }

            const auto asset = registry.GetAsset(record.assetID);
            if (!asset)
            {
                continue;
            }

            const auto& cookProfile = asset->GetCookProfile();

            CookedAssetLocation location;
            location.assetID = record.assetID;
            location.bundleIndex = cookProfile.bundleId;
            location.byteOffset = 0;
            location.byteSize = cookProfile.cookedSizeBytes;
            location.flags = BuildEntryFlags(cookProfile);

            if (!location.IsValid())
            {
                continue;
            }

            RuntimeRegistryBuildItem item;
            item.assetID = record.assetID;
            item.kind = record.kind;
            item.logicalPath = SString::from_view(registry.ResolveString(record.logicalPathId));
            item.location = location;

            items.push_back(std::move(item));
        }

        return items;
    }

    std::vector<RuntimeRegistryBuildItem> AssetRegistryBuilder::CollectBuildItems(
        const AssetRegistry& registry,
        std::span<const CookedAssetLocation> locations)
    {
        std::vector<RuntimeRegistryBuildItem> items;

        const auto& records = registry.GetAllRecords();
        items.reserve(records.size());

        std::unordered_map<shine::AssetID, CookedAssetLocation> locationsByAssetId;
        locationsByAssetId.reserve(locations.size());

        for (const auto& location : locations)
        {
            if (!location.IsValid())
            {
                continue;
            }

            locationsByAssetId.insert_or_assign(location.assetID, location);
        }

        for (const auto& record : records)
        {
            if (!record.IsValid())
            {
                continue;
            }

            if (!record.buildState.cooked)
            {
                continue;
            }

            const auto locationIt = locationsByAssetId.find(record.assetID);
            if (locationIt == locationsByAssetId.end())
            {
                continue;
            }

            const auto asset = registry.GetAsset(record.assetID);
            if (!asset)
            {
                continue;
            }

            CookedAssetLocation location = locationIt->second;
            const auto& cookProfile = asset->GetCookProfile();

            if (location.flags == static_cast<std::uint32_t>(shine::ERegistryEntryFlags::None))
            {
                location.flags = BuildEntryFlags(cookProfile);
            }

            if (location.bundleIndex == shine::InvalidHandle)
            {
                location.bundleIndex = cookProfile.bundleId;
            }

            if (location.byteSize == 0)
            {
                location.byteSize = cookProfile.cookedSizeBytes;
            }

            if (!location.IsValid())
            {
                continue;
            }

            RuntimeRegistryBuildItem item;
            item.assetID = record.assetID;
            item.kind = record.kind;
            item.logicalPath = SString::from_view(registry.ResolveString(record.logicalPathId));
            item.location = location;

            items.push_back(std::move(item));
        }

        return items;
    }

    bool AssetRegistryBuilder::ValidateItems(
        std::span<const RuntimeRegistryBuildItem> items,
        shine::SString* errorMessage)
    {
        if (items.empty())
        {
            return true;
        }

        for (const auto& item : items)
        {
            if (!item.IsValid())
            {
                if (errorMessage != nullptr)
                {
                    *errorMessage = MakeError(
                        "Runtime registry build item is invalid or missing cooked location data.");
                }
                return false;
            }
        }

        for (std::size_t i = 1; i < items.size(); ++i)
        {
            if (items[i - 1].assetID >= items[i].assetID)
            {
                if (errorMessage != nullptr)
                {
                    *errorMessage = MakeError(
                        "Runtime registry build items must be strictly sorted by asset ID with no duplicates.");
                }
                return false;
            }
        }

        return true;
    }

    bool AssetRegistryBuilder::ValidateLocations(
        std::span<const CookedAssetLocation> locations,
        shine::SString* errorMessage)
    {
        if (locations.empty())
        {
            return true;
        }

        std::vector<shine::AssetID> assetIds;
        assetIds.reserve(locations.size());

        for (const auto& location : locations)
        {
            if (!location.IsValid())
            {
                if (errorMessage != nullptr)
                {
                    *errorMessage = MakeError(
                        "Cooked asset location is invalid. AssetID, bundleIndex and byteSize must be valid.");
                }
                return false;
            }

            assetIds.push_back(location.assetID);
        }

        std::ranges::sort(assetIds);

        for (std::size_t i = 1; i < assetIds.size(); ++i)
        {
            if (assetIds[i - 1] == assetIds[i])
            {
                if (errorMessage != nullptr)
                {
                    *errorMessage = MakeError(
                        "Duplicate asset ID detected in cooked asset locations.");
                }
                return false;
            }
        }

        return true;
    }

    std::vector<RuntimeRegistryBuildItem> AssetRegistryBuilder::SortAndDeduplicate(
        std::span<const RuntimeRegistryBuildItem> items,
        shine::SString* errorMessage)
    {
        std::vector<RuntimeRegistryBuildItem> sortedItems(items.begin(), items.end());

        std::ranges::sort(
            sortedItems,
            {},
            &RuntimeRegistryBuildItem::assetID);

        if (sortedItems.empty())
        {
            return sortedItems;
        }

        for (std::size_t i = 1; i < sortedItems.size(); ++i)
        {
            if (sortedItems[i - 1].assetID == sortedItems[i].assetID)
            {
                if (errorMessage != nullptr)
                {
                    *errorMessage = MakeError(
                        "Duplicate asset ID detected while building runtime registry.");
                }
                return {};
            }
        }

        return sortedItems;
    }

    RuntimeRegistryBuildResult AssetRegistryBuilder::WriteRegistryFile(
        std::span<const RuntimeRegistryBuildItem> items,
        shine::STextView outputPath,
        std::uint64_t buildTimestamp)
    {
        RuntimeRegistryBuildResult result;

        if (outputPath.empty())
        {
            result.errorMessage = MakeError("Runtime registry output path is empty.");
            return result;
        }

        SString validationError;
        if (!ValidateItems(items, &validationError))
        {
            result.errorMessage = std::move(validationError);
            return result;
        }

        std::vector<shine::RegistryEntry> entries;
        entries.reserve(items.size());

        for (const auto& item : items)
        {
            entries.push_back(item.ToRegistryEntry());
        }

        shine::RegistryHeader header;
        header.magic = shine::kRuntimeRegistryMagic;
        header.version = shine::kRuntimeRegistryVersion;
        header.headerSize = static_cast<std::uint16_t>(sizeof(shine::RegistryHeader));
        header.entryCount = static_cast<std::uint32_t>(entries.size());
        header.entrySize = static_cast<std::uint32_t>(sizeof(shine::RegistryEntry));
        header.flags = shine::kRuntimeRegistryFlagsNone;
        header.buildTimestamp = ResolveBuildTimestamp(buildTimestamp);
        header.reserved0 = 0;

        std::vector<std::byte> fileData;
        fileData.resize(sizeof(shine::RegistryHeader) + entries.size() * sizeof(shine::RegistryEntry));

        std::memcpy(
            fileData.data(),
            &header,
            sizeof(shine::RegistryHeader));

        if (!entries.empty())
        {
            auto payload = std::span<std::byte>(fileData).subspan(sizeof(shine::RegistryHeader));
            std::memcpy(
                payload.data(),
                entries.data(),
                entries.size() * sizeof(shine::RegistryEntry));
        }

#ifndef SHINE_PLATFORM_WASM
        const bool saved = shine::util::SaveData(
            outputPath,
            std::span<const std::byte>(fileData.data(), fileData.size()));
#else
        const bool saved = shine::util::SaveData(
            outputPath,
            fileData.data(),
            fileData.size());
#endif

        if (!saved)
        {
            result.errorMessage = MakeError("Failed to write runtime registry file.");
            return result;
        }

        result.success = true;
        result.exportedCount = entries.size();
        return result;
    }

    RuntimeRegistryBuildResult AssetRegistryBuilder::BuildFromItems(
        std::span<const RuntimeRegistryBuildItem> items,
        shine::STextView outputPath,
        std::uint64_t buildTimestamp)
    {
        RuntimeRegistryBuildResult result;

        SString errorMessage;
        std::vector<RuntimeRegistryBuildItem> sortedItems = SortAndDeduplicate(items, &errorMessage);
        if (!items.empty() && sortedItems.empty())
        {
            result.errorMessage = std::move(errorMessage);
            return result;
        }

        return WriteRegistryFile(sortedItems, outputPath, buildTimestamp);
    }

    RuntimeRegistryBuildResult AssetRegistryBuilder::BuildFromRegistry(
        const AssetRegistry& registry,
        shine::STextView outputPath,
        std::uint64_t buildTimestamp)
    {
        const std::vector<RuntimeRegistryBuildItem> items = CollectBuildItems(registry);
        return BuildFromItems(items, outputPath, buildTimestamp);
    }

    RuntimeRegistryBuildResult AssetRegistryBuilder::BuildFromRegistryWithLocations(
        const AssetRegistry& registry,
        std::span<const CookedAssetLocation> locations,
        shine::STextView outputPath,
        std::uint64_t buildTimestamp)
    {
        RuntimeRegistryBuildResult result;

        SString locationError;
        if (!ValidateLocations(locations, &locationError))
        {
            result.errorMessage = std::move(locationError);
            return result;
        }

        const std::vector<RuntimeRegistryBuildItem> items =
            CollectBuildItems(registry, locations);

        return BuildFromItems(items, outputPath, buildTimestamp);
    }
}