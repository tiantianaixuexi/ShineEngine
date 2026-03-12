#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "EngineCore/asset/RuntimeBundleReader.h"
#include "EngineCore/asset/RuntimeRegistry.h"
#include "EngineCore/asset/shared/AssetTypes.h"
#include "EngineCore/subsystem.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine
{
    class AssetBase;

    /**
     * @brief 运行时资产加载器。
     *
     * 职责：
     * - 持有并协调 RuntimeRegistry 与 RuntimeBundleReader
     * - 通过 AssetID 查询 cooked 位置
     * - 从 bundle 中读取资产字节
     * - 通过反序列化器注册表，将字节还原为运行时资产对象
     * - 缓存已加载资产，避免重复加载
     *
     * 当前定位是“运行时通用加载骨架”：
     * - 不强绑定具体资产类型
     * - 通过 EAssetKind -> deserializer 的方式扩展
     * - 适合作为后续 Map / Texture / Material / Mesh 等资产的统一加载入口
     */
    class RuntimeAssetLoader final : public Subsystem
    {
    public:
        struct LoadRequest
        {
            AssetID assetID = InvalidAssetID;
            bool forceReload = false;
            bool cacheResult = true;

            [[nodiscard]] constexpr bool IsValid() const noexcept
            {
                return assetID != InvalidAssetID;
            }
        };

        struct LoadedAssetRecord
        {
            AssetID assetID = InvalidAssetID;
            EAssetKind kind = EAssetKind::Unknown;
            std::shared_ptr<AssetBase> asset;
            RegistryEntry registryEntry{};
            std::uint64_t loadedTimestamp = 0;

            [[nodiscard]] bool IsValid() const noexcept
            {
                return assetID != InvalidAssetID && asset != nullptr;
            }
        };

        struct RuntimeAssetLoadResult
        {
            bool success = false;
            AssetID assetID = InvalidAssetID;
            EAssetKind kind = EAssetKind::Unknown;
            std::shared_ptr<AssetBase> asset;
            RegistryEntry registryEntry{};
            SString errorMessage;

            [[nodiscard]] constexpr bool Succeeded() const noexcept
            {
                return success;
            }
        };

        struct DeserializerContext
        {
            AssetID assetID = InvalidAssetID;
            EAssetKind kind = EAssetKind::Unknown;
            RegistryEntry registryEntry{};
            std::span<const std::byte> bytes{};
            RuntimeRegistry* registry = nullptr;
            RuntimeBundleReader* bundleReader = nullptr;

            [[nodiscard]] bool IsValid() const noexcept
            {
                return assetID != InvalidAssetID
                    && kind != EAssetKind::Unknown
                    && !bytes.empty()
                    && registry != nullptr
                    && bundleReader != nullptr;
            }
        };

        using AssetDeserializer =
            std::function<std::expected<std::shared_ptr<AssetBase>, SString>(const DeserializerContext&)>;

        RuntimeAssetLoader() = default;
        ~RuntimeAssetLoader() override = default;

        RuntimeAssetLoader(const RuntimeAssetLoader&) = delete;
        RuntimeAssetLoader& operator=(const RuntimeAssetLoader&) = delete;
        RuntimeAssetLoader(RuntimeAssetLoader&&) = delete;
        RuntimeAssetLoader& operator=(RuntimeAssetLoader&&) = delete;

        bool Init(EngineContext& ctx) override
        {
            registry_ = ctx.GetSystem<RuntimeRegistry>();
            bundleReader_ = ctx.GetSystem<RuntimeBundleReader>();
            loadedAssets_.clear();
            return registry_ != nullptr && bundleReader_ != nullptr;
        }

        void Shutdown(EngineContext& ctx) override
        {
            (void)ctx;
            loadedAssets_.clear();
            registry_ = nullptr;
            bundleReader_ = nullptr;
        }

        /**
         * @brief 显式绑定外部 RuntimeRegistry。
         */
        void SetRegistry(RuntimeRegistry* registry) noexcept
        {
            registry_ = registry;
        }

        /**
         * @brief 显式绑定外部 RuntimeBundleReader。
         */
        void SetBundleReader(RuntimeBundleReader* bundleReader) noexcept
        {
            bundleReader_ = bundleReader;
        }

        /**
         * @brief 注册某种资产类型的反序列化器。
         */
        void RegisterDeserializer(EAssetKind kind, AssetDeserializer deserializer)
        {
            if (kind == EAssetKind::Unknown || !deserializer)
            {
                return;
            }

            deserializers_.insert_or_assign(kind, std::move(deserializer));
        }

        /**
         * @brief 移除某种资产类型的反序列化器。
         */
        [[nodiscard]] bool UnregisterDeserializer(EAssetKind kind) noexcept
        {
            return deserializers_.erase(kind) > 0;
        }

        /**
         * @brief 检查某种资产类型是否已注册反序列化器。
         */
        [[nodiscard]] bool HasDeserializer(EAssetKind kind) const noexcept
        {
            return deserializers_.find(kind) != deserializers_.end();
        }

        /**
         * @brief 清空所有已注册反序列化器。
         */
        void ClearDeserializers() noexcept
        {
            deserializers_.clear();
        }

        /**
         * @brief 根据 AssetID 加载运行时资产。
         */
        [[nodiscard]] RuntimeAssetLoadResult LoadAsset(AssetID assetID)
        {
            return LoadAsset(LoadRequest{ .assetID = assetID });
        }

        /**
         * @brief 根据加载请求加载运行时资产。
         */
        [[nodiscard]] RuntimeAssetLoadResult LoadAsset(const LoadRequest& request)
        {
            RuntimeAssetLoadResult result;
            result.assetID = request.assetID;

            if (!request.IsValid())
            {
                result.errorMessage = MakeError("Runtime asset load request is invalid.");
                return result;
            }

            if (registry_ == nullptr)
            {
                result.errorMessage = MakeError("RuntimeRegistry is not bound.");
                return result;
            }

            if (bundleReader_ == nullptr)
            {
                result.errorMessage = MakeError("RuntimeBundleReader is not bound.");
                return result;
            }

            if (!registry_->IsValid())
            {
                result.errorMessage = MakeError("RuntimeRegistry is not initialized or invalid.");
                return result;
            }

            if (!request.forceReload)
            {
                if (const auto cached = FindLoadedAsset(request.assetID))
                {
                    result.success = true;
                    result.assetID = cached->assetID;
                    result.kind = cached->kind;
                    result.asset = cached->asset;
                    result.registryEntry = cached->registryEntry;
                    return result;
                }
            }

            RegistryEntry entry{};
            if (!registry_->FindAsset(request.assetID, entry))
            {
                result.errorMessage = MakeError("Asset was not found in runtime registry.");
                return result;
            }

            result.registryEntry = entry;

            const auto kind = ResolveAssetKind(entry);
            result.kind = kind;

            if (kind == EAssetKind::Unknown)
            {
                result.errorMessage = MakeError("Unable to resolve asset kind for runtime asset.");
                return result;
            }

            const auto deserializerIt = deserializers_.find(kind);
            if (deserializerIt == deserializers_.end())
            {
                result.errorMessage = MakeError("No runtime deserializer registered for asset kind.");
                return result;
            }

            auto readResult = bundleReader_->Read(entry);
            if (!readResult.has_value())
            {
                result.errorMessage = SString::from_view(readResult.error());
                return result;
            }

            const auto& bundleRead = readResult.value();

            DeserializerContext context;
            context.assetID = request.assetID;
            context.kind = kind;
            context.registryEntry = entry;
            context.bytes = bundleRead.bytes;
            context.registry = registry_;
            context.bundleReader = bundleReader_;

            auto deserializeResult = deserializerIt->second(context);
            if (!deserializeResult.has_value())
            {
                result.errorMessage = deserializeResult.error();
                return result;
            }

            auto asset = std::move(deserializeResult.value());
            if (!asset)
            {
                result.errorMessage = MakeError("Deserializer returned a null asset.");
                return result;
            }

            result.success = true;
            result.asset = asset;

            if (request.cacheResult)
            {
                LoadedAssetRecord record;
                record.assetID = request.assetID;
                record.kind = kind;
                record.asset = asset;
                record.registryEntry = entry;
                record.loadedTimestamp = 0;
                loadedAssets_.insert_or_assign(request.assetID, std::move(record));
            }

            return result;
        }

        /**
         * @brief 批量加载多个资产。
         */
        [[nodiscard]] std::vector<RuntimeAssetLoadResult> LoadAssets(
            std::span<const AssetID> assetIDs,
            bool forceReload = false,
            bool cacheResult = true)
        {
            std::vector<RuntimeAssetLoadResult> results;
            results.reserve(assetIDs.size());

            for (const auto assetID : assetIDs)
            {
                results.push_back(LoadAsset(LoadRequest{
                    .assetID = assetID,
                    .forceReload = forceReload,
                    .cacheResult = cacheResult
                }));
            }

            return results;
        }

        /**
         * @brief 检查某个资产是否已加载并缓存。
         */
        [[nodiscard]] bool IsLoaded(AssetID assetID) const noexcept
        {
            return loadedAssets_.find(assetID) != loadedAssets_.end();
        }

        /**
         * @brief 获取已缓存的运行时资产。
         */
        [[nodiscard]] std::shared_ptr<AssetBase> GetLoadedAsset(AssetID assetID) const noexcept
        {
            if (const auto* record = FindLoadedAsset(assetID))
            {
                return record->asset;
            }

            return nullptr;
        }

        /**
         * @brief 获取已缓存的加载记录。
         */
        [[nodiscard]] const LoadedAssetRecord* GetLoadedRecord(AssetID assetID) const noexcept
        {
            return FindLoadedAsset(assetID);
        }

        /**
         * @brief 卸载某个已缓存资产。
         */
        [[nodiscard]] bool UnloadAsset(AssetID assetID) noexcept
        {
            return loadedAssets_.erase(assetID) > 0;
        }

        /**
         * @brief 清空所有已加载缓存。
         */
        void UnloadAll() noexcept
        {
            loadedAssets_.clear();
        }

        /**
         * @brief 获取已加载资产数量。
         */
        [[nodiscard]] std::size_t GetLoadedAssetCount() const noexcept
        {
            return loadedAssets_.size();
        }

        /**
         * @brief 获取当前绑定的 RuntimeRegistry。
         */
        [[nodiscard]] RuntimeRegistry* GetRegistry() const noexcept
        {
            return registry_;
        }

        /**
         * @brief 获取当前绑定的 RuntimeBundleReader。
         */
        [[nodiscard]] RuntimeBundleReader* GetBundleReader() const noexcept
        {
            return bundleReader_;
        }

        /**
         * @brief 直接调用指定 kind 的反序列化器。
         *
         * 可用于测试或某些跳过 registry 的特殊路径。
         */
        [[nodiscard]] std::expected<std::shared_ptr<AssetBase>, SString> Deserialize(
            EAssetKind kind,
            AssetID assetID,
            const RegistryEntry& entry,
            std::span<const std::byte> bytes) const
        {
            if (kind == EAssetKind::Unknown)
            {
                return std::unexpected(MakeError("Asset kind is unknown."));
            }

            const auto it = deserializers_.find(kind);
            if (it == deserializers_.end())
            {
                return std::unexpected(MakeError("No runtime deserializer registered for asset kind."));
            }

            if (registry_ == nullptr || bundleReader_ == nullptr)
            {
                return std::unexpected(MakeError("Runtime loader dependencies are not bound."));
            }

            DeserializerContext context;
            context.assetID = assetID;
            context.kind = kind;
            context.registryEntry = entry;
            context.bytes = bytes;
            context.registry = registry_;
            context.bundleReader = bundleReader_;

            if (!context.IsValid())
            {
                return std::unexpected(MakeError("Deserializer context is invalid."));
            }

            return it->second(context);
        }

        /**
         * @brief 尝试根据 RegistryEntry 解析资产类型。
         *
         * 当前版本只提供骨架，默认读取失败时返回 Unknown。
         * 推荐后续做法：
         * - 在 RuntimeRegistry format 中补 kind 字段；或
         * - 在 cooked payload 头中记录 kind；或
         * - 外部注册 assetID -> kind 映射表。
         */
        [[nodiscard]] EAssetKind ResolveAssetKind(const RegistryEntry& entry) const noexcept
        {
            if (entry.assetID == InvalidAssetID)
            {
                return EAssetKind::Unknown;
            }

            if (const auto it = kindOverrides_.find(entry.assetID); it != kindOverrides_.end())
            {
                return it->second;
            }

            return EAssetKind::Unknown;
        }

        /**
         * @brief 为当前运行时加载器注入 assetID -> kind 的覆盖映射。
         *
         * 用于在 RuntimeRegistry 尚未内建 kind 字段前，先保证加载链路可用。
         */
        void RegisterAssetKind(AssetID assetID, EAssetKind kind)
        {
            if (assetID == InvalidAssetID || kind == EAssetKind::Unknown)
            {
                return;
            }

            kindOverrides_.insert_or_assign(assetID, kind);
        }

        /**
         * @brief 批量注册 assetID -> kind 映射。
         */
        void RegisterAssetKinds(
            const std::unordered_map<AssetID, EAssetKind>& assetKinds)
        {
            for (const auto& [assetID, kind] : assetKinds)
            {
                RegisterAssetKind(assetID, kind);
            }
        }

        /**
         * @brief 移除 assetID -> kind 覆盖映射。
         */
        [[nodiscard]] bool UnregisterAssetKind(AssetID assetID) noexcept
        {
            return kindOverrides_.erase(assetID) > 0;
        }

        /**
         * @brief 清空 assetID -> kind 覆盖映射。
         */
        void ClearAssetKinds() noexcept
        {
            kindOverrides_.clear();
        }

    private:
        [[nodiscard]] static SString MakeError(STextView text)
        {
            return SString::from_view(text);
        }

        [[nodiscard]] const LoadedAssetRecord* FindLoadedAsset(AssetID assetID) const noexcept
        {
            const auto it = loadedAssets_.find(assetID);
            if (it == loadedAssets_.end())
            {
                return nullptr;
            }

            return &it->second;
        }

    private:
        RuntimeRegistry* registry_ = nullptr;
        RuntimeBundleReader* bundleReader_ = nullptr;
        std::unordered_map<EAssetKind, AssetDeserializer> deserializers_;
        std::unordered_map<AssetID, EAssetKind> kindOverrides_;
        std::unordered_map<AssetID, LoadedAssetRecord> loadedAssets_;
    };
}