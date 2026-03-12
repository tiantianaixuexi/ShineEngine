#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <vector>

#include "EngineCore/asset/BaseAsset.h"
#include "EngineCore/asset/RuntimeAssetLoader.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine
{
    /**
     * @brief 通用运行时二进制资产回退类型。
     *
     * 当某类资产暂未实现正式的 cooked 反序列化器时，可先使用该类型承载：
     * - 原始 cooked payload 字节
     * - 资产基础元数据
     * - 运行时 registry entry 信息
     *
     * 这样 RuntimeAssetLoader 仍可完成：
     * - Registry 查询
     * - Bundle 读取
     * - 基础对象构造
     * - 资产缓存
     *
     * 后续若某一资产类型拥有了专用运行时对象与反序列化器，
     * 再逐步替换掉该 fallback 即可。
     */
    class RuntimeBinaryAsset final : public shine::editor::asset::AssetBase
    {
    public:
        inline static constexpr std::uint32_t kCookedMagic = 0x4E494253; // "SBIN"
        inline static constexpr std::uint16_t kCookedVersion = 1;

#pragma pack(push, 1)
        struct CookedHeader
        {
            std::uint32_t magic = kCookedMagic;
            std::uint16_t version = kCookedVersion;
            std::uint16_t headerSize = static_cast<std::uint16_t>(sizeof(CookedHeader));

            std::uint32_t kind = static_cast<std::uint32_t>(shine::EAssetKind::Unknown);
            std::uint32_t flags = 0;

            std::uint32_t nameOffset = 0;
            std::uint32_t logicalPathOffset = 0;
            std::uint32_t sourcePathOffset = 0;

            std::uint64_t payloadOffset = 0;
            std::uint64_t payloadSize = 0;
        };
#pragma pack(pop)

    public:
        RuntimeBinaryAsset() = default;
        ~RuntimeBinaryAsset() override = default;

        RuntimeBinaryAsset(const RuntimeBinaryAsset&) = default;
        RuntimeBinaryAsset& operator=(const RuntimeBinaryAsset&) = default;
        RuntimeBinaryAsset(RuntimeBinaryAsset&&) noexcept = default;
        RuntimeBinaryAsset& operator=(RuntimeBinaryAsset&&) noexcept = default;

        [[nodiscard]] shine::STextView GetClassName() const noexcept override
        {
            return "RuntimeBinaryAsset";
        }

        void InitBinaryAsset(
            shine::AssetID assetID,
            shine::SString assetName,
            const shine::SString& logicalPath,
            shine::EAssetKind kind)
        {
            Init(assetID, std::move(assetName), logicalPath, kind);
            SetLifecycle(shine::EAssetLifecycle::Cooked);
            MarkClean();
        }

        void SetPayload(std::vector<std::byte> payload)
        {
            payload_ = std::move(payload);
        }

        void SetRawPayload(std::span<const std::byte> payload)
        {
            payload_.assign(payload.begin(), payload.end());
        }

        void SetRuntimeFlags(std::uint32_t flags) noexcept
        {
            runtimeFlags_ = flags;
        }

        void SetRegistryEntry(const shine::RegistryEntry& entry) noexcept
        {
            registryEntry_ = entry;
            hasRegistryEntry_ = true;
        }

        [[nodiscard]] std::uint32_t GetRuntimeFlags() const noexcept
        {
            return runtimeFlags_;
        }

        [[nodiscard]] const std::vector<std::byte>& GetPayload() const noexcept
        {
            return payload_;
        }

        [[nodiscard]] std::span<const std::byte> GetPayloadView() const noexcept
        {
            return std::span<const std::byte>(payload_.data(), payload_.size());
        }

        [[nodiscard]] std::size_t GetPayloadSize() const noexcept
        {
            return payload_.size();
        }

        [[nodiscard]] bool HasPayload() const noexcept
        {
            return !payload_.empty();
        }

        [[nodiscard]] bool HasRegistryEntry() const noexcept
        {
            return hasRegistryEntry_;
        }

        [[nodiscard]] const shine::RegistryEntry& GetRegistryEntry() const noexcept
        {
            return registryEntry_;
        }

        /**
         * @brief 将该资产序列化为通用 cooked binary 格式。
         *
         * 格式布局：
         * - CookedHeader
         * - name 字符串（\0 结尾）
         * - logicalPath 字符串（\0 结尾）
         * - sourcePath 字符串（\0 结尾）
         * - payload 原始字节
         */
        [[nodiscard]] std::vector<std::byte> SerializeCooked() const
        {
            std::vector<std::byte> bytes;
            bytes.resize(sizeof(CookedHeader), std::byte{0});

            auto append_bytes = [&bytes](const void* data, std::size_t size)
            {
                const auto* begin = static_cast<const std::byte*>(data);
                bytes.insert(bytes.end(), begin, begin + static_cast<std::ptrdiff_t>(size));
            };

            auto append_string = [&bytes](shine::STextView text) -> std::uint32_t
            {
                const auto offset = static_cast<std::uint32_t>(bytes.size());
                const auto* begin = reinterpret_cast<const std::byte*>(text.data());
                bytes.insert(
                    bytes.end(),
                    begin,
                    begin + static_cast<std::ptrdiff_t>(text.size()));
                bytes.push_back(std::byte{0});
                return offset;
            };

            CookedHeader header{};
            header.magic = kCookedMagic;
            header.version = kCookedVersion;
            header.headerSize = static_cast<std::uint16_t>(sizeof(CookedHeader));
            header.kind = static_cast<std::uint32_t>(GetKind());
            header.flags = runtimeFlags_;

            header.nameOffset = append_string(GetName());
            header.logicalPathOffset = append_string(GetLogicalPath());
            header.sourcePathOffset = append_string(GetSourcePath());

            header.payloadOffset = static_cast<std::uint64_t>(bytes.size());
            header.payloadSize = static_cast<std::uint64_t>(payload_.size());

            if (!payload_.empty())
            {
                append_bytes(payload_.data(), payload_.size());
            }

            std::memcpy(bytes.data(), &header, sizeof(CookedHeader));
            return bytes;
        }

        /**
         * @brief 从通用 cooked binary 格式反序列化运行时资产。
         */
        [[nodiscard]] static std::expected<std::shared_ptr<RuntimeBinaryAsset>, shine::SString>
        DeserializeCooked(
            shine::AssetID assetID,
            std::span<const std::byte> bytes)
        {
            if (bytes.size() < sizeof(CookedHeader))
            {
                return std::unexpected(MakeError("Cooked binary payload is too small."));
            }

            CookedHeader header{};
            std::memcpy(&header, bytes.data(), sizeof(CookedHeader));

            if (header.magic != kCookedMagic)
            {
                return std::unexpected(MakeError("Cooked binary magic is invalid."));
            }

            if (header.version != kCookedVersion)
            {
                return std::unexpected(MakeError("Cooked binary version is not supported."));
            }

            if (header.headerSize < sizeof(CookedHeader))
            {
                return std::unexpected(MakeError("Cooked binary header size is invalid."));
            }

            const auto read_c_string =
                [&bytes](std::uint32_t offset) -> std::expected<shine::SString, shine::SString>
            {
                if (offset >= bytes.size())
                {
                    return std::unexpected(MakeError("Cooked binary string offset is out of range."));
                }

                const char* begin =
                    reinterpret_cast<const char*>(bytes.data() + static_cast<std::ptrdiff_t>(offset));
                const char* end =
                    reinterpret_cast<const char*>(bytes.data() + static_cast<std::ptrdiff_t>(bytes.size()));

                const char* it = begin;
                while (it < end && *it != '\0')
                {
                    ++it;
                }

                if (it >= end)
                {
                    return std::unexpected(MakeError("Cooked binary string is not null terminated."));
                }

                return shine::SString(begin, static_cast<std::size_t>(it - begin));
            };

            auto nameResult = read_c_string(header.nameOffset);
            if (!nameResult.has_value())
            {
                return std::unexpected(nameResult.error());
            }

            auto logicalPathResult = read_c_string(header.logicalPathOffset);
            if (!logicalPathResult.has_value())
            {
                return std::unexpected(logicalPathResult.error());
            }

            auto sourcePathResult = read_c_string(header.sourcePathOffset);
            if (!sourcePathResult.has_value())
            {
                return std::unexpected(sourcePathResult.error());
            }

            if (header.payloadOffset > bytes.size())
            {
                return std::unexpected(MakeError("Cooked binary payload offset is out of range."));
            }

            if (header.payloadSize > (static_cast<std::uint64_t>(bytes.size()) - header.payloadOffset))
            {
                return std::unexpected(MakeError("Cooked binary payload size is out of range."));
            }

            auto asset = std::make_shared<RuntimeBinaryAsset>();
            const auto kind = static_cast<shine::EAssetKind>(header.kind);

            asset->InitBinaryAsset(
                assetID,
                std::move(nameResult.value()),
                logicalPathResult.value(),
                kind);

            asset->SetSourcePath(sourcePathResult.value());
            asset->SetRuntimeFlags(header.flags);

            const auto payloadBegin =
                bytes.begin() + static_cast<std::ptrdiff_t>(header.payloadOffset);
            const auto payloadEnd =
                payloadBegin + static_cast<std::ptrdiff_t>(header.payloadSize);

            asset->payload_.assign(payloadBegin, payloadEnd);
            asset->MarkClean();
            return asset;
        }

        /**
         * @brief RuntimeAssetLoader 的通用 fallback 反序列化入口。
         *
         * 用法：
         * - 对暂未实现专用反序列化器的资产类型，
         *   可统一注册到该函数。
         * - 它会优先尝试解析成标准 cooked binary 包装格式；
         * - 若格式不匹配，则退化为直接把原始 bytes 当成 payload 保存。
         */
        [[nodiscard]] static std::expected<std::shared_ptr<shine::editor::asset::AssetBase>, shine::SString>
        DeserializeForRuntimeLoader(const shine::RuntimeAssetLoader::DeserializerContext& context)
        {
            if (!context.IsValid())
            {
                return std::unexpected(MakeError("Runtime binary deserializer context is invalid."));
            }

            auto cookedResult = DeserializeCooked(context.assetID, context.bytes);
            if (cookedResult.has_value())
            {
                cookedResult.value()->SetRegistryEntry(context.registryEntry);
                return std::static_pointer_cast<shine::editor::asset::AssetBase>(cookedResult.value());
            }

            auto fallback = std::make_shared<RuntimeBinaryAsset>();
            fallback->InitBinaryAsset(
                context.assetID,
                MakeFallbackName(context.assetID),
                MakeFallbackLogicalPath(context.assetID),
                context.kind);
            fallback->SetRawPayload(context.bytes);
            fallback->SetRuntimeFlags(context.registryEntry.flags);
            fallback->SetRegistryEntry(context.registryEntry);
            fallback->MarkClean();

            return std::static_pointer_cast<shine::editor::asset::AssetBase>(fallback);
        }

    private:
        [[nodiscard]] static shine::SString MakeError(shine::STextView text)
        {
            return shine::SString::from_view(text);
        }

        [[nodiscard]] static shine::SString MakeFallbackName(shine::AssetID assetID)
        {
            shine::SString name = "runtime_binary_";
            name.append(std::to_string(assetID));
            return name;
        }

        [[nodiscard]] static shine::SString MakeFallbackLogicalPath(shine::AssetID assetID)
        {
            shine::SString path = "/runtime/binary/";
            path.append(std::to_string(assetID));
            return path;
        }

    private:
        std::uint32_t runtimeFlags_ = 0;
        bool hasRegistryEntry_ = false;
        shine::RegistryEntry registryEntry_{};
        std::vector<std::byte> payload_;
    };

    static_assert(sizeof(RuntimeBinaryAsset::CookedHeader) >= 32, "RuntimeBinaryAsset::CookedHeader layout changed unexpectedly.");
}