#include "editor/asset/Cooking/BundleBuilder.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "editor/asset/Cooking/AssetRegistryBuilder.h"
#include "util/file_util.ixx"
#include "util/path_util.h"

namespace shine::editor::asset
{
    namespace
    {
        [[nodiscard]] SString MakeErrorText(STextView text)
        {
            return SString::from_view(text);
        }

        [[nodiscard]] SString MakeBundleFileName(
            STextView bundleFilePrefix,
            std::uint32_t bundleIndex)
        {
            SString fileName = SString::from_view(bundleFilePrefix.empty() ? STextView{"bundle"} : bundleFilePrefix);
            fileName.append("_");
            fileName.append(std::to_string(bundleIndex));
            fileName.append(".bundle");
            return fileName;
        }

        [[nodiscard]] SString GetDirectoryPath(STextView path)
        {
            return SString::from_view(util::get_directory(path));
        }

        [[nodiscard]] bool EnsureOutputDirectory(
            STextView bundlePath,
            const BundleWriteOptions& options)
        {
            if (!options.createOutputDirectory)
            {
                return true;
            }

            const SString directory = GetDirectoryPath(bundlePath);
            if (directory.empty())
            {
                return true;
            }

            return util::CreateDirRecursive(directory.view());
        }

        [[nodiscard]] bool CanWriteBundlePath(
            STextView bundlePath,
            const BundleWriteOptions& options)
        {
            if (options.overwriteExisting)
            {
                return true;
            }

            return !util::file_exists(bundlePath);
        }

        [[nodiscard]] std::vector<std::byte> BuildZeroPadding(std::uint64_t size)
        {
            return std::vector<std::byte>(static_cast<std::size_t>(size), std::byte{0});
        }

        [[nodiscard]] bool AppendBytes(
            std::vector<std::byte>& output,
            std::span<const std::byte> bytes)
        {
            try
            {
                output.insert(output.end(), bytes.begin(), bytes.end());
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]] bool AppendPadding(
            std::vector<std::byte>& output,
            std::uint64_t paddingBytes)
        {
            if (paddingBytes == 0)
            {
                return true;
            }

            const auto padding = BuildZeroPadding(paddingBytes);
            return AppendBytes(output, std::span<const std::byte>(padding.data(), padding.size()));
        }

        [[nodiscard]] bool ReadSourceBytes(
            const BundleBuildInputAsset& asset,
            std::vector<std::byte>* outBytes)
        {
            if (outBytes == nullptr)
            {
                return false;
            }

            outBytes->clear();

            if (asset.sourcePath.empty())
            {
                return false;
            }

#ifndef SHINE_PLATFORM_WASM
            auto readResult = util::read_file_bytes(asset.sourcePath.view());
            if (!readResult.has_value())
            {
                return false;
            }

            std::vector<std::byte> sourceData = std::move(readResult.value());
#else
            bool readSuccess = false;
            std::vector<std::byte> sourceData = util::read_file_bytes(asset.sourcePath.view(), &readSuccess);
            if (!readSuccess)
            {
                return false;
            }
#endif

            if (asset.sourceOffset >= sourceData.size())
            {
                return false;
            }

            const std::uint64_t availableBytes =
                static_cast<std::uint64_t>(sourceData.size()) - asset.sourceOffset;

            const std::uint64_t requestedBytes =
                asset.sourceSizeBytes > 0 ? asset.sourceSizeBytes : availableBytes;

            const std::uint64_t copiedBytes = std::min(availableBytes, requestedBytes);

            const auto beginIt =
                sourceData.begin() + static_cast<std::ptrdiff_t>(asset.sourceOffset);
            const auto endIt =
                beginIt + static_cast<std::ptrdiff_t>(copiedBytes);

            try
            {
                outBytes->assign(beginIt, endIt);
            }
            catch (...)
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] bool BuildBundlePayload(
            const BundleBuildRequest& request,
            const BundleBuildResult& plan,
            const BundleWriteOptions& options,
            std::vector<std::byte>* outPayload,
            SString* outError)
        {
            if (outPayload == nullptr)
            {
                if (outError != nullptr)
                {
                    *outError = MakeErrorText("Bundle payload output buffer is null.");
                }
                return false;
            }

            outPayload->clear();

            if (request.assets.size() < plan.entries.size())
            {
                if (outError != nullptr)
                {
                    *outError = MakeErrorText("Bundle planning result does not match input asset count.");
                }
                return false;
            }

            std::size_t plannedEntryIndex = 0;

            for (const auto& asset : request.assets)
            {
                if (asset.cookedSizeBytes == 0 && options.skipAssetsWithZeroCookedSize)
                {
                    continue;
                }

                if (plannedEntryIndex >= plan.entries.size())
                {
                    if (outError != nullptr)
                    {
                        *outError = MakeErrorText("Bundle planning result is missing cooked entries.");
                    }
                    return false;
                }

                const CookedBundleEntry& entry = plan.entries[plannedEntryIndex];
                ++plannedEntryIndex;

                if (entry.assetID != asset.assetID)
                {
                    if (outError != nullptr)
                    {
                        *outError = MakeErrorText("Bundle planning result asset order mismatch.");
                    }
                    return false;
                }

                const auto currentSize = static_cast<std::uint64_t>(outPayload->size());
                if (entry.byteOffset < currentSize)
                {
                    if (outError != nullptr)
                    {
                        *outError = MakeErrorText("Bundle planning produced overlapping payload offsets.");
                    }
                    return false;
                }

                const std::uint64_t paddingBytes = entry.byteOffset - currentSize;
                if (!AppendPadding(*outPayload, paddingBytes))
                {
                    if (outError != nullptr)
                    {
                        *outError = MakeErrorText("Failed to append bundle alignment padding.");
                    }
                    return false;
                }

                std::vector<std::byte> sourceBytes;
                if (!ReadSourceBytes(asset, &sourceBytes))
                {
                    if (outError != nullptr)
                    {
                        *outError = MakeErrorText("Failed to read cooked source payload for bundle asset.");
                    }
                    return false;
                }

                if (options.writeSparseFromSource)
                {
                    if (static_cast<std::uint64_t>(sourceBytes.size()) > entry.byteSize)
                    {
                        sourceBytes.resize(static_cast<std::size_t>(entry.byteSize));
                    }
                    else if (static_cast<std::uint64_t>(sourceBytes.size()) < entry.byteSize)
                    {
                        const std::uint64_t missingBytes =
                            entry.byteSize - static_cast<std::uint64_t>(sourceBytes.size());
                        try
                        {
                            sourceBytes.insert(
                                sourceBytes.end(),
                                static_cast<std::size_t>(missingBytes),
                                std::byte{0});
                        }
                        catch (...)
                        {
                            if (outError != nullptr)
                            {
                                *outError = MakeErrorText("Failed to expand sparse bundle payload.");
                            }
                            return false;
                        }
                    }
                }
                else
                {
                    if (static_cast<std::uint64_t>(sourceBytes.size()) != entry.byteSize)
                    {
                        if (outError != nullptr)
                        {
                            *outError = MakeErrorText("Bundle source payload size does not match cooked entry size.");
                        }
                        return false;
                    }
                }

                if (!AppendBytes(
                        *outPayload,
                        std::span<const std::byte>(sourceBytes.data(), sourceBytes.size())))
                {
                    if (outError != nullptr)
                    {
                        *outError = MakeErrorText("Failed to append cooked payload bytes.");
                    }
                    return false;
                }
            }

            if (plannedEntryIndex != plan.entries.size())
            {
                if (outError != nullptr)
                {
                    *outError = MakeErrorText("Bundle planning result contains unused cooked entries.");
                }
                return false;
            }

            if (static_cast<std::uint64_t>(outPayload->size()) != plan.totalSizeBytes)
            {
                if (outError != nullptr)
                {
                    *outError = MakeErrorText("Bundle payload size does not match planned total size.");
                }
                return false;
            }

            return true;
        }

        [[nodiscard]] bool WriteBundlePayloadToDisk(
            STextView bundlePath,
            std::span<const std::byte> payload)
        {
#ifndef SHINE_PLATFORM_WASM
            return util::SaveData(bundlePath, payload);
#else
            return util::SaveData(bundlePath, payload.data(), payload.size());
#endif
        }
    }

    std::uint32_t BundleBuilder::ResolveEntryFlags(const AssetBase& asset) noexcept
    {
        std::uint32_t flags = 0;
        const auto& cookProfile = asset.GetCookProfile();

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

    SString BundleBuilder::MakeDefaultBundlePath(
        shine::STextView outputDirectory,
        shine::STextView bundleFilePrefix,
        std::uint32_t bundleIndex)
    {
        const SString fileName = MakeBundleFileName(bundleFilePrefix, bundleIndex);

        if (outputDirectory.empty())
        {
            return util::normalize_path(fileName);
        }

        return util::join_path(outputDirectory, fileName.view());
    }

    std::vector<CookedAssetLocation> BundleBuilder::CollectLocationsFromLayout(
        const CookedBundleLayout& layout)
    {
        return layout.ToCookedLocations();
    }

    std::vector<CookedAssetLocation> BundleBuilder::CollectLocationsFromLayouts(
        std::span<const CookedBundleLayout> layouts)
    {
        std::size_t totalCount = 0;
        for (const auto& layout : layouts)
        {
            totalCount += layout.entries.size();
        }

        std::vector<CookedAssetLocation> locations;
        locations.reserve(totalCount);

        for (const auto& layout : layouts)
        {
            auto layoutLocations = layout.ToCookedLocations();
            locations.insert(
                locations.end(),
                std::make_move_iterator(layoutLocations.begin()),
                std::make_move_iterator(layoutLocations.end()));
        }

        return locations;
    }

    std::vector<CookedAssetLocation> BundleBuilder::CollectLocationsFromResult(
        const BundleBuildResult& result)
    {
        return result.ToCookedLocations();
    }

    std::vector<CookedAssetLocation> BundleBuilder::CollectLocationsFromResults(
        std::span<const BundleBuildResult> results)
    {
        std::size_t totalCount = 0;
        for (const auto& result : results)
        {
            totalCount += result.entries.size();
        }

        std::vector<CookedAssetLocation> locations;
        locations.reserve(totalCount);

        for (const auto& result : results)
        {
            auto resultLocations = result.ToCookedLocations();
            locations.insert(
                locations.end(),
                std::make_move_iterator(resultLocations.begin()),
                std::make_move_iterator(resultLocations.end()));
        }

        return locations;
    }

    BundleBuildRequest BundleBuilder::CollectBuildRequest(
        const AssetRegistry& registry,
        std::uint32_t bundleIndex,
        shine::STextView outputBundlePath)
    {
        BundleBuildRequest request;
        request.bundleIndex = bundleIndex;
        request.outputBundlePath = util::normalize_path(SString::from_view(outputBundlePath));

        const auto& records = registry.GetAllRecords();
        request.assets.reserve(records.size());

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
            if (cookProfile.bundleId != bundleIndex)
            {
                continue;
            }

            BundleBuildInputAsset input;
            input.assetID = record.assetID;
            input.kind = record.kind;
            input.logicalPath = SString::from_view(asset->GetLogicalPath());
            input.sourcePath = SString::from_view(asset->GetSourcePath());
            input.bundleIndex = bundleIndex;
            input.sourceOffset = 0;
            input.sourceSizeBytes = cookProfile.sourceSizeBytes;
            input.cookedSizeBytes = cookProfile.cookedSizeBytes;
            input.flags = ResolveEntryFlags(*asset);

            request.assets.push_back(std::move(input));
        }

        std::ranges::sort(
            request.assets,
            {},
            &BundleBuildInputAsset::assetID);

        return request;
    }

    std::vector<BundleBuildRequest> BundleBuilder::CollectBuildRequests(
        const AssetRegistry& registry,
        shine::STextView outputDirectory,
        shine::STextView bundleFilePrefix)
    {
        std::unordered_map<std::uint32_t, std::vector<BundleBuildInputAsset>> groupedAssets;
        const auto& records = registry.GetAllRecords();

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

            BundleBuildInputAsset input;
            input.assetID = record.assetID;
            input.kind = record.kind;
            input.logicalPath = SString::from_view(asset->GetLogicalPath());
            input.sourcePath = SString::from_view(asset->GetSourcePath());
            input.bundleIndex = cookProfile.bundleId;
            input.sourceOffset = 0;
            input.sourceSizeBytes = cookProfile.sourceSizeBytes;
            input.cookedSizeBytes = cookProfile.cookedSizeBytes;
            input.flags = ResolveEntryFlags(*asset);

            groupedAssets[cookProfile.bundleId].push_back(std::move(input));
        }

        std::vector<BundleBuildRequest> requests;
        requests.reserve(groupedAssets.size());

        for (auto& [bundleIndex, assets] : groupedAssets)
        {
            std::ranges::sort(
                assets,
                {},
                &BundleBuildInputAsset::assetID);

            BundleBuildRequest request;
            request.bundleIndex = bundleIndex;
            request.outputBundlePath = MakeDefaultBundlePath(outputDirectory, bundleFilePrefix, bundleIndex);
            request.assets = std::move(assets);

            requests.push_back(std::move(request));
        }

        std::ranges::sort(
            requests,
            {},
            &BundleBuildRequest::bundleIndex);

        return requests;
    }

    BundleBuildResult BundleBuilder::BuildBundleInternal(
        const BundleBuildRequest& request,
        const BundleWriteOptions& options,
        bool writeBundleFile)
    {
        BundleBuildResult result;
        result.bundleIndex = request.bundleIndex;
        result.bundlePath = request.outputBundlePath;
        result.wroteBundleFile = false;
        result.totalSizeBytes = 0;
        result.payloadSizeBytes = 0;
        result.writtenAssetCount = 0;

        if (!request.IsValid())
        {
            result.errorMessage = MakeErrorText("Bundle build request is invalid.");
            return result;
        }

        if (request.assets.empty())
        {
            if (writeBundleFile)
            {
                if (!EnsureOutputDirectory(result.bundlePath.view(), options))
                {
                    result.errorMessage = MakeErrorText("Failed to create bundle output directory.");
                    return result;
                }

                if (!CanWriteBundlePath(result.bundlePath.view(), options))
                {
                    result.errorMessage = MakeErrorText("Bundle output path already exists and overwrite is disabled.");
                    return result;
                }

                const std::vector<std::byte> emptyPayload;
                if (!WriteBundlePayloadToDisk(
                        result.bundlePath.view(),
                        std::span<const std::byte>(emptyPayload.data(), emptyPayload.size())))
                {
                    result.errorMessage = MakeErrorText("Failed to write empty bundle file.");
                    return result;
                }

                result.wroteBundleFile = true;
            }

            result.success = true;
            return result;
        }

        std::uint64_t currentOffset = 0;
        result.entries.reserve(request.assets.size());

        for (const auto& asset : request.assets)
        {
            if (!asset.IsValid())
            {
                result.errorMessage = MakeErrorText("Bundle build request contains an invalid asset.");
                result.entries.clear();
                result.totalSizeBytes = 0;
                result.payloadSizeBytes = 0;
                result.writtenAssetCount = 0;
                return result;
            }

            std::uint32_t resolvedBundleIndex = asset.bundleIndex;
            if (resolvedBundleIndex != request.bundleIndex)
            {
                if (!options.allowBundleIndexOverride)
                {
                    result.errorMessage = MakeErrorText("Bundle build request contains an asset assigned to a different bundle.");
                    result.entries.clear();
                    result.totalSizeBytes = 0;
                    result.payloadSizeBytes = 0;
                    result.writtenAssetCount = 0;
                    return result;
                }

                resolvedBundleIndex = request.bundleIndex;
            }

            if (asset.cookedSizeBytes == 0 && options.skipAssetsWithZeroCookedSize)
            {
                continue;
            }

            currentOffset = options.AlignOffset(currentOffset);

            CookedBundleEntry entry;
            entry.assetID = asset.assetID;
            entry.bundleIndex = resolvedBundleIndex;
            entry.byteOffset = currentOffset;
            entry.byteSize = asset.cookedSizeBytes;
            entry.sourceOffset = asset.sourceOffset;
            entry.sourceSize = asset.sourceSizeBytes;
            entry.flags = asset.flags;

            if (!entry.IsValid())
            {
                result.errorMessage = MakeErrorText("Bundle build produced an invalid cooked entry.");
                result.entries.clear();
                result.totalSizeBytes = 0;
                result.payloadSizeBytes = 0;
                result.writtenAssetCount = 0;
                return result;
            }

            result.entries.push_back(entry);
            currentOffset += entry.byteSize;
            result.payloadSizeBytes += entry.byteSize;
        }

        result.totalSizeBytes = currentOffset;
        result.writtenAssetCount = result.entries.size();

        if (!writeBundleFile)
        {
            result.success = true;
            return result;
        }

        if (!EnsureOutputDirectory(result.bundlePath.view(), options))
        {
            result.entries.clear();
            result.totalSizeBytes = 0;
            result.payloadSizeBytes = 0;
            result.writtenAssetCount = 0;
            result.errorMessage = MakeErrorText("Failed to create bundle output directory.");
            return result;
        }

        if (!CanWriteBundlePath(result.bundlePath.view(), options))
        {
            result.entries.clear();
            result.totalSizeBytes = 0;
            result.payloadSizeBytes = 0;
            result.writtenAssetCount = 0;
            result.errorMessage = MakeErrorText("Bundle output path already exists and overwrite is disabled.");
            return result;
        }

        std::vector<std::byte> payload;
        SString payloadError;
        if (!BuildBundlePayload(request, result, options, &payload, &payloadError))
        {
            result.entries.clear();
            result.totalSizeBytes = 0;
            result.payloadSizeBytes = 0;
            result.writtenAssetCount = 0;
            result.errorMessage = std::move(payloadError);
            return result;
        }

        if (!WriteBundlePayloadToDisk(
                result.bundlePath.view(),
                std::span<const std::byte>(payload.data(), payload.size())))
        {
            result.entries.clear();
            result.totalSizeBytes = 0;
            result.payloadSizeBytes = 0;
            result.writtenAssetCount = 0;
            result.errorMessage = MakeErrorText("Failed to write bundle payload to disk.");
            return result;
        }

        result.wroteBundleFile = true;
        result.success = true;
        return result;
    }

    BundleBuildResult BundleBuilder::BuildBundle(
        const BundleBuildRequest& request,
        const BundleWriteOptions& options)
    {
        return BuildBundleInternal(request, options, options.writeBundleFile);
    }

    BundleBuildResult BundleBuilder::PlanBundle(
        const BundleBuildRequest& request,
        const BundleWriteOptions& options)
    {
        return BuildBundleInternal(request, options, false);
    }

    std::vector<BundleBuildResult> BundleBuilder::BuildBundles(
        std::span<const BundleBuildRequest> requests,
        const BundleWriteOptions& options)
    {
        std::vector<BundleBuildResult> results;
        results.reserve(requests.size());

        for (const auto& request : requests)
        {
            results.push_back(BuildBundle(request, options));
        }

        return results;
    }

    std::vector<BundleBuildResult> BundleBuilder::PlanBundles(
        std::span<const BundleBuildRequest> requests,
        const BundleWriteOptions& options)
    {
        std::vector<BundleBuildResult> results;
        results.reserve(requests.size());

        for (const auto& request : requests)
        {
            results.push_back(PlanBundle(request, options));
        }

        return results;
    }

    bool BundleBuilder::BuildRegistry(
        const AssetRegistry& registry,
        const BundleBuildResult& result,
        shine::STextView outputPath)
    {
        if (!result.Succeeded())
        {
            return false;
        }

        const auto locations = CollectLocationsFromResult(result);
        const auto buildResult =
            AssetRegistryBuilder::BuildFromRegistryWithLocations(registry, locations, outputPath);

        return buildResult.Succeeded();
    }

    bool BundleBuilder::BuildRegistry(
        const AssetRegistry& registry,
        std::span<const BundleBuildResult> results,
        shine::STextView outputPath)
    {
        for (const auto& result : results)
        {
            if (!result.Succeeded())
            {
                return false;
            }
        }

        const auto locations = CollectLocationsFromResults(results);
        const auto buildResult =
            AssetRegistryBuilder::BuildFromRegistryWithLocations(registry, locations, outputPath);

        return buildResult.Succeeded();
    }

    bool BundleBuilder::BuildRegistry(
        const AssetRegistry& registry,
        shine::STextView outputPath)
    {
        const auto result = AssetRegistryBuilder::BuildFromRegistry(registry, outputPath);
        return result.Succeeded();
    }

    bool BundleBuilder::BuildRegistry(
        AssetRegistryBuilder& registryBuilder,
        const AssetRegistry& registry,
        shine::STextView outputPath)
    {
        (void)registryBuilder;
        const auto result = AssetRegistryBuilder::BuildFromRegistry(registry, outputPath);
        return result.Succeeded();
    }

    bool BundleBuilder::BuildRegistry(
        const AssetRegistry& registry,
        const CookedBundleLayout& layout,
        shine::STextView outputPath)
    {
        const auto locations = CollectLocationsFromLayout(layout);
        const auto result =
            AssetRegistryBuilder::BuildFromRegistryWithLocations(registry, locations, outputPath);

        return result.Succeeded();
    }

    bool BundleBuilder::BuildRegistry(
        AssetRegistryBuilder& registryBuilder,
        const AssetRegistry& registry,
        const CookedBundleLayout& layout,
        shine::STextView outputPath)
    {
        (void)registryBuilder;
        const auto locations = CollectLocationsFromLayout(layout);
        const auto result =
            AssetRegistryBuilder::BuildFromRegistryWithLocations(registry, locations, outputPath);

        return result.Succeeded();
    }

    bool BundleBuilder::BuildRegistry(
        const AssetRegistry& registry,
        std::span<const CookedBundleLayout> layouts,
        shine::STextView outputPath)
    {
        const auto locations = CollectLocationsFromLayouts(layouts);
        const auto result =
            AssetRegistryBuilder::BuildFromRegistryWithLocations(registry, locations, outputPath);

        return result.Succeeded();
    }

    bool BundleBuilder::BuildRegistry(
        AssetRegistryBuilder& registryBuilder,
        const AssetRegistry& registry,
        std::span<const CookedBundleLayout> layouts,
        shine::STextView outputPath)
    {
        (void)registryBuilder;
        const auto locations = CollectLocationsFromLayouts(layouts);
        const auto result =
            AssetRegistryBuilder::BuildFromRegistryWithLocations(registry, locations, outputPath);

        return result.Succeeded();
    }
}