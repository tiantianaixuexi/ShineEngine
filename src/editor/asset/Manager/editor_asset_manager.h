#pragma once

#include <expected>
#include <memory>
#include <string>

#include "EngineCore/asset/shared/AssetTypes.h"
#include "EngineCore/engine_context.h"
#include "EngineCore/subsystem.h"
#include "editor/asset/AssetRegistry.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::editor::asset
{
    using EditorAssetBuildState = AssetRegistryBuildState;
    using EditorAssetDependency = AssetRegistryDependency;

    struct EditorAssetRecord
    {
        shine::AssetID assetID = shine::InvalidAssetID;
        shine::StringId logicalPathId = shine::InvalidStringId;
        shine::StringId sourcePathId = shine::InvalidStringId;
        shine::EAssetKind kind = shine::EAssetKind::Unknown;

        std::uint32_t depOffset = 0;
        std::uint32_t depCount = 0;

        EditorAssetBuildState buildState{};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return assetID != shine::InvalidAssetID;
        }
    };

    class EditorAssetManager final : public shine::Subsystem
    {
    public:
        EditorAssetManager() = default;
        ~EditorAssetManager() override = default;

        EditorAssetManager(const EditorAssetManager&) = delete;
        EditorAssetManager& operator=(const EditorAssetManager&) = delete;
        EditorAssetManager(EditorAssetManager&&) = delete;
        EditorAssetManager& operator=(EditorAssetManager&&) = delete;

        bool Init(EngineContext& ctx) override
        {
            registry_ = ctx.GetSystem<AssetRegistry>();
            return registry_ != nullptr;
        }

        void Shutdown(EngineContext& ctx) override
        {
            (void)ctx;
            registry_ = nullptr;
        }

        void RegisterAsset(const std::shared_ptr<AssetBase>& asset)
        {
            if (registry_ != nullptr)
            {
                registry_->RegisterAsset(asset);
            }
        }

        [[nodiscard]] bool RemoveAsset(shine::AssetID id)
        {
            return registry_ != nullptr && registry_->RemoveAsset(id);
        }

        [[nodiscard]] std::shared_ptr<AssetBase> GetAsset(shine::AssetID id) const
        {
            return registry_ != nullptr ? registry_->GetAsset(id) : nullptr;
        }

        [[nodiscard]] std::shared_ptr<AssetBase> GetAssetByPath(shine::STextView logicalPath) const
        {
            return registry_ != nullptr ? registry_->GetAssetByLogicalPath(logicalPath) : nullptr;
        }

        [[nodiscard]] bool UpsertAssetRecord(const EditorAssetRecord& record)
        {
            if (registry_ == nullptr || !record.IsValid())
            {
                return false;
            }

            auto asset = registry_->GetAsset(record.assetID);
            if (!asset)
            {
                return false;
            }

            registry_->RegisterAsset(asset);

            for (const auto existingDependency : registry_->GetDependencies(record.assetID))
            {
                [[maybe_unused]] const bool removed =
                    registry_->RemoveDependency(record.assetID, existingDependency.assetID);
                (void)removed;
            }

            return true;
        }

        [[nodiscard]] std::expected<EditorAssetRecord, std::string> GetAssetRecord(shine::AssetID id) const
        {
            if (registry_ == nullptr)
            {
                return std::unexpected(std::string("Asset registry is not available"));
            }

            const auto result = registry_->GetRecord(id);
            if (!result.has_value())
            {
                return std::unexpected(result.error().to_string());
            }

            return ToLegacyRecord(result.value());
        }

        [[nodiscard]] std::expected<EditorAssetRecord, std::string> GetAssetRecordByPath(shine::STextView anyPath) const
        {
            if (registry_ == nullptr)
            {
                return std::unexpected(std::string("Asset registry is not available"));
            }

            const auto result = registry_->GetRecordByLogicalPath(anyPath);
            if (!result.has_value())
            {
                return std::unexpected(result.error().to_string());
            }

            return ToLegacyRecord(result.value());
        }

        [[nodiscard]] bool AddDependency(
            shine::AssetID owner,
            shine::AssetID dependency,
            bool hardReference = true)
        {
            return registry_ != nullptr && registry_->AddDependency(owner, dependency, hardReference);
        }

        [[nodiscard]] std::vector<shine::AssetID> BuildPreloadList(const std::vector<shine::AssetID>& roots) const
        {
            return registry_ != nullptr ? registry_->BuildPreloadList(roots) : std::vector<shine::AssetID>{};
        }

        [[nodiscard]] std::expected<void, std::string> ValidateForCook(shine::AssetID id) const
        {
            if (registry_ == nullptr)
            {
                return std::unexpected(std::string("Asset registry is not available"));
            }

            const auto result = registry_->ValidateForCook(id);
            if (!result.has_value())
            {
                return std::unexpected(result.error().to_string());
            }

            return {};
        }

        [[nodiscard]] std::vector<shine::SString> BuildCookManifest() const
        {
            std::vector<shine::SString> manifest;
            if (registry_ == nullptr)
            {
                return manifest;
            }

            const auto& records = registry_->GetAllRecords();
            manifest.reserve(records.size());

            for (const auto& record : records)
            {
                const auto path = registry_->ResolveString(record.logicalPathId);
                if (!path.empty())
                {
                    manifest.push_back(shine::SString::from_view(path));
                }
            }

            return manifest;
        }

        void RegisterAssetFactory(
            shine::EAssetKind kind,
            std::unique_ptr<shine::IAssetFactory> factory)
        {
            if (registry_ != nullptr && factory != nullptr)
            {
                if (factory->GetSupportedKind() == kind)
                {
                    registry_->RegisterFactory(std::move(factory));
                }
            }
        }

        [[nodiscard]] std::shared_ptr<AssetBase> CreateAssetByType(
            shine::EAssetKind kind,
            const shine::SString& logicalPath)
        {
            if (registry_ == nullptr)
            {
                return nullptr;
            }

            shine::AssetCreateContext context;
            context.kind = kind;
            context.logicalPath = logicalPath;
            context.assetName = logicalPath;
            return registry_->CreateAsset(context);
        }

        [[nodiscard]] AssetRegistry* GetRegistry() noexcept
        {
            return registry_;
        }

        [[nodiscard]] const AssetRegistry* GetRegistry() const noexcept
        {
            return registry_;
        }

    private:
        [[nodiscard]] EditorAssetRecord ToLegacyRecord(const AssetRegistryRecord& record) const
        {
            EditorAssetRecord legacy;
            legacy.assetID = record.assetID;
            legacy.logicalPathId = record.logicalPathId;
            legacy.sourcePathId = record.sourcePathId;
            legacy.kind = record.kind;
            legacy.depOffset = record.dependencyOffset;
            legacy.depCount = record.dependencyCount;
            legacy.buildState = record.buildState;
            return legacy;
        }

    private:
        AssetRegistry* registry_ = nullptr;
    };
}