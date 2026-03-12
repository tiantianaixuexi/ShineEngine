#pragma once

#include <cstdint>
#include <memory>

#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine
{
    using AssetID = std::uint64_t;
    constexpr AssetID InvalidAssetID = 0;

    using AssetHandle = std::uint32_t;
    constexpr AssetHandle InvalidHandle = 0xFFFFFFFFu;

    using StringId = std::uint32_t;
    constexpr StringId InvalidStringId = 0xFFFFFFFFu;

    enum class EAssetKind : std::uint8_t
    {
        Unknown = 0,
        Texture,
        Mesh,
        Material,
        Shader,
        Audio,
        Script,
        World,
        Blueprint,
        GameplayData,
        Count
    };

    enum class EAssetLifecycle : std::uint8_t
    {
        Imported = 0,
        Dirty,
        Cooking,
        Cooked,
        Failed
    };

    struct SoftAssetRef
    {
        AssetID id = InvalidAssetID;
        SString logicalPath;

        [[nodiscard]] bool IsValid() const noexcept
        {
            return id != InvalidAssetID || !logicalPath.empty();
        }

        void Reset()
        {
            id = InvalidAssetID;
            logicalPath.clear();
        }
    };

    struct AssetLocation
    {
        std::uint32_t bundleIndex = InvalidHandle;
        std::uint64_t byteOffset = 0;
        std::uint64_t byteSize = 0;

        [[nodiscard]] bool IsValid() const noexcept
        {
            return bundleIndex != InvalidHandle;
        }
    };

    class AssetBase;

    struct AssetCreateContext
    {
        SString assetName;
        SString logicalPath;
        SString sourcePath;
        EAssetKind kind = EAssetKind::Unknown;
    };

    struct AssetCreateResult
    {
        std::shared_ptr<AssetBase> asset;
        SString errorMessage;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return static_cast<bool>(asset);
        }
    };

    class IAssetFactory
    {
    public:
        virtual ~IAssetFactory() = default;

        [[nodiscard]] virtual AssetCreateResult CreateAsset(const AssetCreateContext& context) = 0;
        [[nodiscard]] virtual EAssetKind GetSupportedKind() const noexcept = 0;
        [[nodiscard]] virtual STextView GetDisplayName() const noexcept = 0;
        [[nodiscard]] virtual bool CanCreateNew() const noexcept { return true; }
    };
}