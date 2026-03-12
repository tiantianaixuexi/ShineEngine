#pragma once

#include <cstddef>
#include <cstdint>

#include "EngineCore/asset/shared/AssetTypes.h"

namespace shine
{
#pragma pack(push, 1)

    inline constexpr std::uint32_t kRuntimeRegistryMagic = 0x4E494853; // "SHIN"
    inline constexpr std::uint16_t kRuntimeRegistryVersion = 1;
    inline constexpr std::uint16_t kRuntimeRegistryFlagsNone = 0;

    enum class ERegistryEntryFlags : std::uint32_t
    {
        None = 0,
        Compressed = 1u << 0,
        Streamable = 1u << 1
    };

    constexpr ERegistryEntryFlags operator|(ERegistryEntryFlags lhs, ERegistryEntryFlags rhs) noexcept
    {
        return static_cast<ERegistryEntryFlags>(
            static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
    }

    constexpr ERegistryEntryFlags operator&(ERegistryEntryFlags lhs, ERegistryEntryFlags rhs) noexcept
    {
        return static_cast<ERegistryEntryFlags>(
            static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
    }

    constexpr ERegistryEntryFlags& operator|=(ERegistryEntryFlags& lhs, ERegistryEntryFlags rhs) noexcept
    {
        lhs = lhs | rhs;
        return lhs;
    }

    struct RegistryEntry
    {
        AssetID assetID = 0;
        std::uint32_t bundleIndex = 0;
        std::uint32_t flags = static_cast<std::uint32_t>(ERegistryEntryFlags::None);
        std::uint64_t offset = 0;
        std::uint64_t size = 0;
    };

    struct RegistryHeader
    {
        std::uint32_t magic = kRuntimeRegistryMagic;
        std::uint16_t version = kRuntimeRegistryVersion;
        std::uint16_t headerSize = static_cast<std::uint16_t>(sizeof(RegistryHeader));
        std::uint32_t entryCount = 0;
        std::uint32_t entrySize = static_cast<std::uint32_t>(sizeof(RegistryEntry));
        std::uint32_t flags = kRuntimeRegistryFlagsNone;
        std::uint64_t buildTimestamp = 0;
        std::uint64_t reserved0 = 0;
    };

#pragma pack(pop)

    static_assert(sizeof(RegistryHeader) >= 32, "RegistryHeader layout changed unexpectedly.");
    static_assert(sizeof(RegistryEntry) >= 24, "RegistryEntry layout changed unexpectedly.");
    static_assert(alignof(RegistryHeader) == 1, "RegistryHeader must remain packed.");
    static_assert(alignof(RegistryEntry) == 1, "RegistryEntry must remain packed.");

    [[nodiscard]] constexpr bool IsValidRegistryHeader(const RegistryHeader& header, std::size_t fileSize) noexcept
    {
        if (header.magic != kRuntimeRegistryMagic)
        {
            return false;
        }

        if (header.version != kRuntimeRegistryVersion)
        {
            return false;
        }

        if (header.headerSize < sizeof(RegistryHeader))
        {
            return false;
        }

        if (header.entrySize != sizeof(RegistryEntry))
        {
            return false;
        }

        if (fileSize < header.headerSize)
        {
            return false;
        }

        const std::uint64_t entriesBytes =
            static_cast<std::uint64_t>(header.entryCount) * static_cast<std::uint64_t>(header.entrySize);
        const std::uint64_t requiredSize =
            static_cast<std::uint64_t>(header.headerSize) + entriesBytes;

        if (requiredSize > static_cast<std::uint64_t>(fileSize))
        {
            return false;
        }

        return true;
    }

    [[nodiscard]] constexpr bool IsValidRegistryEntry(const RegistryEntry& entry) noexcept
    {
        if (entry.assetID == 0)
        {
            return false;
        }

        return true;
    }
}