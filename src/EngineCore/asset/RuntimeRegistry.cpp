#include "RuntimeRegistry.h"

#include <algorithm>
#include <cstring>

namespace shine
{
    namespace
    {
        [[nodiscard]] constexpr bool IsHeaderStructLayoutSupported(const RegistryHeader& header) noexcept
        {
            return header.headerSize >= sizeof(RegistryHeader) &&
                   header.entrySize == sizeof(RegistryEntry);
        }

        [[nodiscard]] constexpr bool IsKnownVersion(const RegistryHeader& header) noexcept
        {
            return header.version == RuntimeRegistry::kSupportedVersion;
        }

        [[nodiscard]] constexpr bool IsMagicValid(const RegistryHeader& header) noexcept
        {
            return header.magic == kRuntimeRegistryMagic;
        }

        [[nodiscard]] bool IsHeaderValid(const RegistryHeader& header, std::size_t fileSize) noexcept
        {
            return IsMagicValid(header) &&
                   IsKnownVersion(header) &&
                   IsHeaderStructLayoutSupported(header) &&
                   IsValidRegistryHeader(header, fileSize);
        }

        [[nodiscard]] bool AreEntriesValid(std::span<const RegistryEntry> entries) noexcept
        {
            for (const RegistryEntry& entry : entries)
            {
                if (!IsValidRegistryEntry(entry))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] bool AreEntriesStrictlySorted(std::span<const RegistryEntry> entries) noexcept
        {
            if (entries.empty())
            {
                return true;
            }

            for (std::size_t i = 1; i < entries.size(); ++i)
            {
                if (entries[i - 1].assetID >= entries[i].assetID)
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] bool BuildValidatedEntriesView(
            std::span<const std::byte> fileContent,
            const RegistryHeader*& outHeader,
            std::span<const RegistryEntry>& outEntries) noexcept
        {
            outHeader = nullptr;
            outEntries = {};

            if (fileContent.size() < sizeof(RegistryHeader))
            {
                return false;
            }

            RegistryHeader headerCopy{};
            std::memcpy(&headerCopy, fileContent.data(), sizeof(RegistryHeader));

            if (!IsHeaderValid(headerCopy, fileContent.size()))
            {
                return false;
            }

            const auto headerBytes = static_cast<std::size_t>(headerCopy.headerSize);
            const auto entrySize = static_cast<std::size_t>(headerCopy.entrySize);
            const auto entryCount = static_cast<std::size_t>(headerCopy.entryCount);
            const auto entriesBytes = entrySize * entryCount;

            if (headerBytes > fileContent.size() || entriesBytes > (fileContent.size() - headerBytes))
            {
                return false;
            }

            if (headerBytes % alignof(RegistryEntry) != 0)
            {
                return false;
            }

            const auto fileBytes = fileContent.subspan(0);
            const auto headerBytesView = fileBytes.first(sizeof(RegistryHeader));
            const auto entriesBytesView = fileBytes.subspan(headerBytes, entriesBytes);

            const auto* headerPtr =
                static_cast<const RegistryHeader*>(static_cast<const void*>(headerBytesView.data()));
            const auto* entriesBegin =
                static_cast<const RegistryEntry*>(static_cast<const void*>(entriesBytesView.data()));
            const std::span<const RegistryEntry> entries(entriesBegin, entryCount);

            if (!AreEntriesValid(entries))
            {
                return false;
            }

            if (!AreEntriesStrictlySorted(entries))
            {
                return false;
            }

            outHeader = headerPtr;
            outEntries = entries;
            return true;
        }
    }

    bool RuntimeRegistry::Initialize(STextView registryPath)
    {
        Reset();

        if (registryPath.empty())
        {
            return false;
        }

#ifndef SHINE_PLATFORM_WASM
        auto mappedResult = util::read_full_file(registryPath);
        if (!mappedResult.has_value())
        {
            return false;
        }

        mappedFile_ = std::move(mappedResult.value());
#else
        bool success = false;
        mappedFile_ = util::read_full_file(registryPath, &success);
        if (!success)
        {
            mappedFile_ = {};
            return false;
        }
#endif

        if (!ValidateMappedData())
        {
            Reset();
            return false;
        }

        return true;
    }

    void RuntimeRegistry::Reset() noexcept
    {
        valid_ = false;
        header_ = nullptr;
        entries_ = {};
        mappedFile_ = {};
    }

    bool RuntimeRegistry::IsInitialized() const noexcept
    {
#ifndef SHINE_PLATFORM_WASM
        return !mappedFile_.view.empty();
#else
        return mappedFile_.view.data() != nullptr && mappedFile_.view.size() > 0;
#endif
    }

    bool RuntimeRegistry::IsValid() const noexcept
    {
        return valid_;
    }

    const RegistryHeader* RuntimeRegistry::GetHeader() const noexcept
    {
        return header_;
    }

    std::span<const RegistryEntry> RuntimeRegistry::GetEntries() const noexcept
    {
        return entries_;
    }

    std::size_t RuntimeRegistry::GetEntryCount() const noexcept
    {
        return entries_.size();
    }

    std::size_t RuntimeRegistry::GetFileSize() const noexcept
    {
        return mappedFile_.view.size();
    }

    bool RuntimeRegistry::FindAsset(AssetID id, RegistryEntry& outEntry) const noexcept
    {
        if (!valid_ || id == InvalidAssetID || entries_.empty())
        {
            return false;
        }

        const auto it = std::ranges::lower_bound(
            entries_,
            id,
            {},
            &RegistryEntry::assetID);

        if (it == entries_.end() || it->assetID != id)
        {
            return false;
        }

        outEntry = *it;
        return true;
    }

    bool RuntimeRegistry::FindAsset(
        AssetID id,
        std::uint32_t& bundleIdx,
        std::uint64_t& offset,
        std::uint64_t& size) const noexcept
    {
        RegistryEntry entry{};
        if (!FindAsset(id, entry))
        {
            return false;
        }

        bundleIdx = entry.bundleIndex;
        offset = entry.offset;
        size = entry.size;
        return true;
    }

    bool RuntimeRegistry::ValidateMappedData() noexcept
    {
        valid_ = false;
        header_ = nullptr;
        entries_ = {};

#ifndef SHINE_PLATFORM_WASM
        const auto content = mappedFile_.view.content;
#else
        const auto content = std::span<const std::byte>(mappedFile_.view.data(), mappedFile_.view.size());
#endif

        if (!BuildValidatedEntriesView(content, header_, entries_))
        {
            return false;
        }

        valid_ = true;
        return true;
    }
}