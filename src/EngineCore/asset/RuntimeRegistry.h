#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "AssetRegistryFormat.h"
#include "EngineCore/asset/shared/AssetTypes.h"
#include "string/shine_text_view.h"
#include "util/file_util.ixx"

namespace shine
{
    class RuntimeRegistry
    {
    public:
        static constexpr std::uint16_t kSupportedVersion = kRuntimeRegistryVersion;

        RuntimeRegistry() = default;
        ~RuntimeRegistry() = default;

        RuntimeRegistry(const RuntimeRegistry&) = delete;
        RuntimeRegistry& operator=(const RuntimeRegistry&) = delete;

        RuntimeRegistry(RuntimeRegistry&&) noexcept = default;
        RuntimeRegistry& operator=(RuntimeRegistry&&) noexcept = default;

        [[nodiscard]] bool Initialize(STextView registryPath);
        void Reset() noexcept;

        [[nodiscard]] bool IsInitialized() const noexcept;
        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] const RegistryHeader* GetHeader() const noexcept;
        [[nodiscard]] std::span<const RegistryEntry> GetEntries() const noexcept;
        [[nodiscard]] std::size_t GetEntryCount() const noexcept;
        [[nodiscard]] std::size_t GetFileSize() const noexcept;

        [[nodiscard]] bool FindAsset(AssetID id, RegistryEntry& outEntry) const noexcept;
        [[nodiscard]] bool FindAsset(
            AssetID id,
            std::uint32_t& bundleIdx,
            std::uint64_t& offset,
            std::uint64_t& size) const noexcept;

    private:
        [[nodiscard]] bool ValidateMappedData() noexcept;

    private:
        util::FileMapView mappedFile_{};
        const RegistryHeader* header_ = nullptr;
        std::span<const RegistryEntry> entries_{};
        bool valid_ = false;
    };
}