#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "editor/asset/AssetRegistry.h"
#include "editor/asset/Cooking/AssetRegistryBuilder.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::editor::asset
{
    struct CookedBundleEntry
    {
        shine::AssetID assetID = shine::InvalidAssetID;
        std::uint32_t bundleIndex = shine::InvalidHandle;
        std::uint64_t byteOffset = 0;
        std::uint64_t byteSize = 0;
        std::uint64_t sourceOffset = 0;
        std::uint64_t sourceSize = 0;
        std::uint32_t flags = 0;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return assetID != shine::InvalidAssetID
                && bundleIndex != shine::InvalidHandle
                && byteSize > 0;
        }

        [[nodiscard]] constexpr CookedAssetLocation ToCookedLocation() const noexcept
        {
            return CookedAssetLocation{
                .assetID = assetID,
                .bundleIndex = bundleIndex,
                .byteOffset = byteOffset,
                .byteSize = byteSize,
                .flags = flags
            };
        }
    };

    struct CookedBundleLayout
    {
        std::uint32_t bundleIndex = shine::InvalidHandle;
        shine::SString bundlePath;
        std::uint64_t totalSizeBytes = 0;
        std::uint64_t payloadSizeBytes = 0;
        std::vector<CookedBundleEntry> entries;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return bundleIndex != shine::InvalidHandle;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return entries.empty();
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return entries.size();
        }

        [[nodiscard]] std::vector<CookedAssetLocation> ToCookedLocations() const
        {
            std::vector<CookedAssetLocation> locations;
            locations.reserve(entries.size());

            for (const auto& entry : entries)
            {
                if (!entry.IsValid())
                {
                    continue;
                }

                locations.push_back(entry.ToCookedLocation());
            }

            return locations;
        }
    };

    struct BundleBuildInputAsset
    {
        shine::AssetID assetID = shine::InvalidAssetID;
        shine::EAssetKind kind = shine::EAssetKind::Unknown;
        shine::SString logicalPath;
        shine::SString sourcePath;
        std::uint32_t bundleIndex = shine::InvalidHandle;
        std::uint64_t sourceOffset = 0;
        std::uint64_t sourceSizeBytes = 0;
        std::uint64_t cookedSizeBytes = 0;
        std::uint32_t flags = 0;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return assetID != shine::InvalidAssetID
                && bundleIndex != shine::InvalidHandle;
        }

        [[nodiscard]] CookedBundleEntry ToEntry(std::uint64_t byteOffset) const noexcept
        {
            return CookedBundleEntry{
                .assetID = assetID,
                .bundleIndex = bundleIndex,
                .byteOffset = byteOffset,
                .byteSize = cookedSizeBytes,
                .sourceOffset = sourceOffset,
                .sourceSize = sourceSizeBytes,
                .flags = flags
            };
        }
    };

    struct BundleBuildRequest
    {
        std::uint32_t bundleIndex = shine::InvalidHandle;
        shine::SString outputBundlePath;
        std::vector<BundleBuildInputAsset> assets;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return bundleIndex != shine::InvalidHandle && !outputBundlePath.empty();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return assets.empty();
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return assets.size();
        }
    };

    struct BundleWriteOptions
    {
        bool alignEntries = false;
        std::uint32_t alignment = 16;
        bool skipAssetsWithZeroCookedSize = true;
        bool allowBundleIndexOverride = false;
        bool writeBundleFile = true;
        bool overwriteExisting = true;
        bool createOutputDirectory = true;
        bool writeSparseFromSource = true;

        [[nodiscard]] constexpr std::uint64_t AlignOffset(std::uint64_t offset) const noexcept
        {
            if (!alignEntries || alignment <= 1)
            {
                return offset;
            }

            const auto a = static_cast<std::uint64_t>(alignment);
            const std::uint64_t remainder = offset % a;
            return remainder == 0 ? offset : (offset + (a - remainder));
        }
    };

    struct BundleBuildResult
    {
        bool success = false;
        bool wroteBundleFile = false;
        shine::SString bundlePath;
        std::uint32_t bundleIndex = shine::InvalidHandle;
        std::uint64_t totalSizeBytes = 0;
        std::uint64_t payloadSizeBytes = 0;
        std::size_t writtenAssetCount = 0;
        std::vector<CookedBundleEntry> entries;
        shine::SString errorMessage;

        [[nodiscard]] constexpr bool Succeeded() const noexcept
        {
            return success;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return entries.empty();
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return entries.size();
        }

        [[nodiscard]] CookedBundleLayout ToLayout() const
        {
            CookedBundleLayout layout;
            layout.bundleIndex = bundleIndex;
            layout.bundlePath = bundlePath;
            layout.totalSizeBytes = totalSizeBytes;
            layout.payloadSizeBytes = payloadSizeBytes;
            layout.entries = entries;
            return layout;
        }

        [[nodiscard]] std::vector<CookedAssetLocation> ToCookedLocations() const
        {
            return ToLayout().ToCookedLocations();
        }
    };

    /**
     * @brief Transitional facade for bundle cooking and runtime registry generation.
     *
     * 当前职责分为两层：
     * 1. 将待 cook 的资产描述整理成 bundle layout / build result
     * 2. 将 bundle layout 转换为 RuntimeRegistryBuilder 可消费的 cooked location 表
     *
     * 当前版本仍然是轻量管线骨架，重点是打通：
     * AssetRegistry -> BundleBuildRequest -> BundleBuildResult -> RuntimeRegistry
     */
    class BundleBuilder
    {
    public:
        /**
         * @brief 从编辑器资产注册表收集某个 bundle 的构建请求。
         *
         * 默认仅收集：
         * - record 有效
         * - buildState.cooked == true
         * - 资产存在
         * - 资产 cookProfile.bundleId == bundleIndex
         *
         * @param registry 编辑器资产注册表
         * @param bundleIndex 目标 bundle 索引
         * @param outputBundlePath 输出 bundle 路径
         * @return 构建请求
         */
        [[nodiscard]] static BundleBuildRequest CollectBuildRequest(
            const AssetRegistry& registry,
            std::uint32_t bundleIndex,
            shine::STextView outputBundlePath);

        /**
         * @brief 从编辑器资产注册表收集全部 bundle 的构建请求。
         *
         * @param registry 编辑器资产注册表
         * @param outputDirectory bundle 输出目录
         * @param bundleFilePrefix bundle 文件名前缀，默认 "bundle"
         * @return 多 bundle 构建请求列表
         */
        [[nodiscard]] static std::vector<BundleBuildRequest> CollectBuildRequests(
            const AssetRegistry& registry,
            shine::STextView outputDirectory,
            shine::STextView bundleFilePrefix = "bundle");

        /**
         * @brief 根据请求生成 bundle layout / build result。
         *
         * 当前实现定位为数据建模与偏移分配层：
         * - 计算每个资产在 bundle 中的 byteOffset
         * - 生成 CookedBundleEntry 列表
         * - 汇总 totalSizeBytes
         *
         * 真实二进制写盘逻辑可在 cpp 中逐步替换/增强，而不影响对外数据接口。
         *
         * @param request bundle 构建请求
         * @param options 写入选项
         * @return bundle 构建结果
         */
        [[nodiscard]] static BundleBuildResult BuildBundle(
            const BundleBuildRequest& request,
            const BundleWriteOptions& options = {});

        /**
         * @brief 仅规划 bundle 布局，不实际写文件。
         * @param request bundle 构建请求
         * @param options 写入选项
         * @return 仅包含布局结果的 bundle 构建结果
         */
        [[nodiscard]] static BundleBuildResult PlanBundle(
            const BundleBuildRequest& request,
            const BundleWriteOptions& options = {});

        /**
         * @brief 批量构建多个 bundle。
         * @param requests 多 bundle 构建请求
         * @param options 写入选项
         * @return 多个 bundle 构建结果
         */
        [[nodiscard]] static std::vector<BundleBuildResult> BuildBundles(
            std::span<const BundleBuildRequest> requests,
            const BundleWriteOptions& options = {});

        /**
         * @brief 批量规划多个 bundle 布局，不实际写文件。
         * @param requests 多 bundle 构建请求
         * @param options 写入选项
         * @return 多个 bundle 布局结果
         */
        [[nodiscard]] static std::vector<BundleBuildResult> PlanBundles(
            std::span<const BundleBuildRequest> requests,
            const BundleWriteOptions& options = {});

        /**
         * @brief 从单个 bundle 构建结果直接生成 runtime registry 文件。
         * @param registry 编辑器资产注册表
         * @param result 单个 bundle 构建结果
         * @param outputPath 输出 registry 文件路径
         * @return true if the registry file was generated successfully.
         */
        [[nodiscard]] static bool BuildRegistry(
            const AssetRegistry& registry,
            const BundleBuildResult& result,
            shine::STextView outputPath);

        /**
         * @brief 从多个 bundle 构建结果直接生成 runtime registry 文件。
         * @param registry 编辑器资产注册表
         * @param results 多个 bundle 构建结果
         * @param outputPath 输出 registry 文件路径
         * @return true if the registry file was generated successfully.
         */
        [[nodiscard]] static bool BuildRegistry(
            const AssetRegistry& registry,
            std::span<const BundleBuildResult> results,
            shine::STextView outputPath);

        /**
         * @brief Build a runtime asset registry file from the editor asset registry.
         * @param registry Source editor asset registry.
         * @param outputPath Destination registry file path.
         * @return true if the registry file was generated successfully.
         */
        [[nodiscard]] static bool BuildRegistry(
            const AssetRegistry& registry,
            shine::STextView outputPath);

        /**
         * @brief Build a runtime asset registry file using an explicit registry builder.
         * @param registryBuilder Registry builder implementation.
         * @param registry Source editor asset registry.
         * @param outputPath Destination registry file path.
         * @return true if the registry file was generated successfully.
         */
        [[nodiscard]] static bool BuildRegistry(
            AssetRegistryBuilder& registryBuilder,
            const AssetRegistry& registry,
            shine::STextView outputPath);

        /**
         * @brief Build a runtime asset registry file from one cooked bundle layout.
         * @param registry Source editor asset registry.
         * @param layout Cooked bundle layout containing final asset offsets and sizes.
         * @param outputPath Destination registry file path.
         * @return true if the registry file was generated successfully.
         */
        [[nodiscard]] static bool BuildRegistry(
            const AssetRegistry& registry,
            const CookedBundleLayout& layout,
            shine::STextView outputPath);

        /**
         * @brief Build a runtime asset registry file from one cooked bundle layout using an explicit registry builder.
         * @param registryBuilder Registry builder implementation.
         * @param registry Source editor asset registry.
         * @param layout Cooked bundle layout containing final asset offsets and sizes.
         * @param outputPath Destination registry file path.
         * @return true if the registry file was generated successfully.
         */
        [[nodiscard]] static bool BuildRegistry(
            AssetRegistryBuilder& registryBuilder,
            const AssetRegistry& registry,
            const CookedBundleLayout& layout,
            shine::STextView outputPath);

        /**
         * @brief Build a runtime asset registry file from multiple cooked bundle layouts.
         * @param registry Source editor asset registry.
         * @param layouts Cooked bundle layouts containing final asset offsets and sizes.
         * @param outputPath Destination registry file path.
         * @return true if the registry file was generated successfully.
         */
        [[nodiscard]] static bool BuildRegistry(
            const AssetRegistry& registry,
            std::span<const CookedBundleLayout> layouts,
            shine::STextView outputPath);

        /**
         * @brief Build a runtime asset registry file from multiple cooked bundle layouts using an explicit registry builder.
         * @param registryBuilder Registry builder implementation.
         * @param registry Source editor asset registry.
         * @param layouts Cooked bundle layouts containing final asset offsets and sizes.
         * @param outputPath Destination registry file path.
         * @return true if the registry file was generated successfully.
         */
        [[nodiscard]] static bool BuildRegistry(
            AssetRegistryBuilder& registryBuilder,
            const AssetRegistry& registry,
            std::span<const CookedBundleLayout> layouts,
            shine::STextView outputPath);

    private:
        [[nodiscard]] static std::uint32_t ResolveEntryFlags(const AssetBase& asset) noexcept;
        [[nodiscard]] static shine::SString MakeDefaultBundlePath(
            shine::STextView outputDirectory,
            shine::STextView bundleFilePrefix,
            std::uint32_t bundleIndex);

        [[nodiscard]] static std::vector<CookedAssetLocation> CollectLocationsFromLayout(
            const CookedBundleLayout& layout);

        [[nodiscard]] static std::vector<CookedAssetLocation> CollectLocationsFromLayouts(
            std::span<const CookedBundleLayout> layouts);

        [[nodiscard]] static std::vector<CookedAssetLocation> CollectLocationsFromResult(
            const BundleBuildResult& result);

        [[nodiscard]] static std::vector<CookedAssetLocation> CollectLocationsFromResults(
            std::span<const BundleBuildResult> results);

        [[nodiscard]] static BundleBuildResult BuildBundleInternal(
            const BundleBuildRequest& request,
            const BundleWriteOptions& options,
            bool writeBundleFile);
    };
}