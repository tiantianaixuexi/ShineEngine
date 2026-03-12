#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "EngineCore/asset/AssetRegistryFormat.h"
#include "EngineCore/asset/shared/AssetTypes.h"
#include "editor/asset/AssetRegistry.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::editor::asset
{
    /**
     * @brief 已完成 cook 的资产定位信息。
     *
     * 该结构描述某个资产在最终运行时 bundle 中的真实位置，
     * 用于生成 RuntimeRegistry 可直接消费的索引文件。
     */
    struct CookedAssetLocation
    {
        shine::AssetID assetID = shine::InvalidAssetID;
        std::uint32_t bundleIndex = shine::InvalidHandle;
        std::uint64_t byteOffset = 0;
        std::uint64_t byteSize = 0;
        std::uint32_t flags = static_cast<std::uint32_t>(shine::ERegistryEntryFlags::None);

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return assetID != shine::InvalidAssetID
                && bundleIndex != shine::InvalidHandle
                && byteSize > 0;
        }

        [[nodiscard]] constexpr shine::RegistryEntry ToRegistryEntry() const noexcept
        {
            return shine::RegistryEntry{
                .assetID = assetID,
                .bundleIndex = bundleIndex,
                .flags = flags,
                .offset = byteOffset,
                .size = byteSize
            };
        }
    };

    /**
     * @brief RuntimeRegistryBuilder 的中间输入项。
     *
     * 该结构以编辑器资产记录为基础，同时携带已经计算好的 cooked 位置。
     * 当外部 cook / bundle 系统已经知道资产最终写入位置时，推荐使用此结构
     * 作为导出 RuntimeRegistry 的显式输入。
     */
    struct RuntimeRegistryBuildItem
    {
        shine::AssetID assetID = shine::InvalidAssetID;
        shine::EAssetKind kind = shine::EAssetKind::Unknown;
        shine::SString logicalPath;
        CookedAssetLocation location{};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return location.IsValid() && assetID == location.assetID;
        }

        [[nodiscard]] constexpr shine::RegistryEntry ToRegistryEntry() const noexcept
        {
            return location.ToRegistryEntry();
        }
    };

    /**
     * @brief 外部显式提供的 cooked 位置表。
     *
     * 用于把 cook / bundle 阶段生成的真实资产物理位置输入到
     * AssetRegistryBuilder 中，再导出 RuntimeRegistry 文件。
     */
    struct RuntimeRegistryLocationTable
    {
        std::vector<CookedAssetLocation> locations;

        [[nodiscard]] bool empty() const noexcept
        {
            return locations.empty();
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return locations.size();
        }
    };

    struct RuntimeRegistryBuildResult
    {
        bool success = false;
        std::size_t exportedCount = 0;
        shine::SString errorMessage;

        [[nodiscard]] constexpr bool Succeeded() const noexcept
        {
            return success;
        }
    };

    class AssetRegistryBuilder final
    {
    public:
        AssetRegistryBuilder() = default;
        ~AssetRegistryBuilder() = default;

        AssetRegistryBuilder(const AssetRegistryBuilder&) = delete;
        AssetRegistryBuilder& operator=(const AssetRegistryBuilder&) = delete;
        AssetRegistryBuilder(AssetRegistryBuilder&&) = delete;
        AssetRegistryBuilder& operator=(AssetRegistryBuilder&&) = delete;

        /**
         * @brief 从编辑器资产注册表导出运行时 registry 文件。
         *
         * 当前策略依赖资产自身的 cookProfile / buildState 信息。
         * 当调用方没有单独维护 bundle 内精确 offset/size 表时，可使用该接口。
         *
         * @param registry 编辑器资产注册表
         * @param outputPath 输出 registry 文件路径
         * @param buildTimestamp 可选构建时间戳，默认写入 0
         * @return 导出结果
         */
        [[nodiscard]] static RuntimeRegistryBuildResult BuildFromRegistry(
            const AssetRegistry& registry,
            shine::STextView outputPath,
            std::uint64_t buildTimestamp = 0);

        /**
         * @brief 使用显式 cooked 位置表导出运行时 registry 文件。
         *
         * 这是推荐的正式 cook 管线接口。调用方先产出每个资产的 bundleIndex /
         * byteOffset / byteSize，再由 builder 负责校验、排序并写出最终
         * RuntimeRegistry 文件。
         *
         * @param registry 编辑器资产注册表
         * @param locations 已完成 cook 的资产位置表
         * @param outputPath 输出 registry 文件路径
         * @param buildTimestamp 可选构建时间戳，默认写入 0
         * @return 导出结果
         */
        [[nodiscard]] static RuntimeRegistryBuildResult BuildFromRegistryWithLocations(
            const AssetRegistry& registry,
            std::span<const CookedAssetLocation> locations,
            shine::STextView outputPath,
            std::uint64_t buildTimestamp = 0);

        /**
         * @brief 将外部收集好的 runtime registry 项写入文件。
         *
         * @param items 已具备真实 cooked 定位信息的 registry 条目
         * @param outputPath 输出 registry 文件路径
         * @param buildTimestamp 可选构建时间戳，默认写入 0
         * @return 导出结果
         */
        [[nodiscard]] static RuntimeRegistryBuildResult BuildFromItems(
            std::span<const RuntimeRegistryBuildItem> items,
            shine::STextView outputPath,
            std::uint64_t buildTimestamp = 0);

        /**
         * @brief 从编辑器注册表收集可导出的 runtime registry 项。
         *
         * 当前默认策略：
         * - 资产记录有效
         * - buildState.cooked == true
         * - buildState.bundleId 有效
         * - 资产存在且 cook profile 具备有效 cookedSizeBytes
         *
         * 当尚未接入精确 bundle offset 表时，该接口会基于资产现有 buildState /
         * cookProfile 生成构建项。
         *
         * @param registry 编辑器资产注册表
         * @return 可直接用于 BuildFromItems 的条目列表
         */
        [[nodiscard]] static std::vector<RuntimeRegistryBuildItem> CollectBuildItems(
            const AssetRegistry& registry);

        /**
         * @brief 使用显式 cooked 位置表收集 runtime registry 构建项。
         *
         * 该接口会将 registry 中的资产信息与外部位置表做匹配，仅收集同时存在于：
         * - 资产注册表
         * - cooked 位置表
         *
         * 中的资产。
         *
         * @param registry 编辑器资产注册表
         * @param locations 已完成 cook 的资产位置表
         * @return 可直接用于 BuildFromItems 的条目列表
         */
        [[nodiscard]] static std::vector<RuntimeRegistryBuildItem> CollectBuildItems(
            const AssetRegistry& registry,
            std::span<const CookedAssetLocation> locations);

        /**
         * @brief 验证 runtime registry 构建项是否可写出。
         *
         * 验证内容包括：
         * - AssetID 非空
         * - cooked 位置有效
         * - 无重复 AssetID
         * - 已按 AssetID 严格升序
         *
         * @param items runtime registry 构建项
         * @param errorMessage 失败原因输出
         * @return 验证通过返回 true
         */
        [[nodiscard]] static bool ValidateItems(
            std::span<const RuntimeRegistryBuildItem> items,
            shine::SString* errorMessage = nullptr);

        /**
         * @brief 验证 cooked 资产位置表。
         *
         * 验证内容包括：
         * - AssetID 非空
         * - bundleIndex 有效
         * - byteSize > 0
         * - 无重复 AssetID
         *
         * @param locations cooked 位置表
         * @param errorMessage 失败原因输出
         * @return 验证通过返回 true
         */
        [[nodiscard]] static bool ValidateLocations(
            std::span<const CookedAssetLocation> locations,
            shine::SString* errorMessage = nullptr);

    private:
        [[nodiscard]] static std::vector<RuntimeRegistryBuildItem> SortAndDeduplicate(
            std::span<const RuntimeRegistryBuildItem> items,
            shine::SString* errorMessage);

        [[nodiscard]] static RuntimeRegistryBuildResult WriteRegistryFile(
            std::span<const RuntimeRegistryBuildItem> items,
            shine::STextView outputPath,
            std::uint64_t buildTimestamp);

        [[nodiscard]] static std::uint32_t BuildEntryFlags(const AssetCookProfile& cookProfile) noexcept;
    };
}