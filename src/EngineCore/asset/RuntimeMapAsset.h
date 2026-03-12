#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <vector>

#include "EngineCore/asset/BaseAsset.h"
#include "EngineCore/asset/RuntimeAssetLoader.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine
{
    /**
     * @brief 运行时 Map 资产。
     *
     * 这是 editor::asset::MapAsset 的运行时对应物，职责是：
     * - 持有运行时可消费的世界基础定义
     * - 支持 cooked 二进制序列化 / 反序列化
     * - 作为 RuntimeAssetLoader 的反序列化目标类型
     *
     * 当前设计只保留运行时真正需要的静态描述数据：
     * - 世界名称
     * - 世界逻辑路径
     * - 世界设置
     * - 持久关卡 Actor 列表
     * - Streaming Level 列表
     *
     * 不在此对象中直接承载：
     * - 运行中 Actor 实例
     * - 异步加载状态
     * - 世界激活/卸载过程状态
     *
     * 这些状态应继续放在 WorldService / Runtime world layer 中。
     */
    class RuntimeMapAsset final : public shine::editor::asset::AssetBase
    {
    public:
        inline static constexpr std::uint32_t kCookedMagic = 0x50414D53; // "SMAP"
        inline static constexpr std::uint16_t kCookedVersion = 1;

#pragma pack(push, 1)
        struct CookedHeader
        {
            std::uint32_t magic = kCookedMagic;
            std::uint16_t version = kCookedVersion;
            std::uint16_t headerSize = static_cast<std::uint16_t>(sizeof(CookedHeader));
            std::uint32_t flags = 0;

            std::uint32_t nameOffset = 0;
            std::uint32_t logicalPathOffset = 0;
            std::uint32_t worldNameOffset = 0;

            std::uint32_t persistentActorOffset = 0;
            std::uint32_t persistentActorCount = 0;

            std::uint32_t levelOffset = 0;
            std::uint32_t levelCount = 0;

            float gravityX = 0.0f;
            float gravityY = -980.0f;
            float gravityZ = 0.0f;
            std::uint8_t enableStreaming = 1;
            std::uint8_t reserved0 = 0;
            std::uint16_t reserved1 = 0;
        };

        struct CookedLevelHeader
        {
            std::uint32_t nameOffset = 0;
            std::uint32_t actorOffset = 0;
            std::uint32_t actorCount = 0;
            std::uint32_t flags = 0;
        };

        struct CookedActorHeader
        {
            std::uint32_t actorNameOffset = 0;
            std::uint32_t classPathOffset = 0;
            std::uint32_t flags = 0;

            float positionX = 0.0f;
            float positionY = 0.0f;
            float positionZ = 0.0f;

            float rotationPitch = 0.0f;
            float rotationYaw = 0.0f;
            float rotationRoll = 0.0f;

            float scaleX = 1.0f;
            float scaleY = 1.0f;
            float scaleZ = 1.0f;
        };
#pragma pack(pop)

        struct WorldSettings
        {
            float gravityX = 0.0f;
            float gravityY = -980.0f;
            float gravityZ = 0.0f;
            bool enableStreaming = true;
        };

        struct ActorSpawnDefinition
        {
            shine::SString actorName;
            shine::SString classPath;

            float positionX = 0.0f;
            float positionY = 0.0f;
            float positionZ = 0.0f;

            float rotationPitch = 0.0f;
            float rotationYaw = 0.0f;
            float rotationRoll = 0.0f;

            float scaleX = 1.0f;
            float scaleY = 1.0f;
            float scaleZ = 1.0f;

            std::uint32_t flags = 0;

            [[nodiscard]] bool IsValid() const noexcept
            {
                return !classPath.empty();
            }
        };

        struct LevelDefinition
        {
            shine::SString levelName;
            std::vector<ActorSpawnDefinition> actors;
            std::uint32_t flags = 0;
        };

    public:
        RuntimeMapAsset()
        {
            SetLifecycle(shine::EAssetLifecycle::Cooked);
        }

        ~RuntimeMapAsset() override = default;

        RuntimeMapAsset(const RuntimeMapAsset&) = default;
        RuntimeMapAsset& operator=(const RuntimeMapAsset&) = default;
        RuntimeMapAsset(RuntimeMapAsset&&) noexcept = default;
        RuntimeMapAsset& operator=(RuntimeMapAsset&&) noexcept = default;

        [[nodiscard]] shine::STextView GetClassName() const noexcept override
        {
            return "RuntimeMapAsset";
        }

        void InitRuntimeMap(
            shine::AssetID assetID,
            shine::SString assetName,
            const shine::SString& logicalPath)
        {
            Init(assetID, std::move(assetName), logicalPath, shine::EAssetKind::World);
            SetLifecycle(shine::EAssetLifecycle::Cooked);
            MarkClean();
        }

        void SetWorldName(shine::SString worldName)
        {
            worldName_ = std::move(worldName);
        }

        void SetWorldSettings(const WorldSettings& settings)
        {
            worldSettings_ = settings;
        }

        void SetPersistentActors(std::vector<ActorSpawnDefinition> actors)
        {
            persistentActors_ = std::move(actors);
        }

        void SetStreamingLevels(std::vector<LevelDefinition> levels)
        {
            streamingLevels_ = std::move(levels);
        }

        void AddPersistentActor(ActorSpawnDefinition actor)
        {
            persistentActors_.push_back(std::move(actor));
        }

        void AddStreamingLevel(LevelDefinition level)
        {
            streamingLevels_.push_back(std::move(level));
        }

        [[nodiscard]] shine::STextView GetWorldName() const noexcept
        {
            return worldName_.view();
        }

        [[nodiscard]] const WorldSettings& GetWorldSettings() const noexcept
        {
            return worldSettings_;
        }

        [[nodiscard]] const std::vector<ActorSpawnDefinition>& GetPersistentActors() const noexcept
        {
            return persistentActors_;
        }

        [[nodiscard]] const std::vector<LevelDefinition>& GetStreamingLevels() const noexcept
        {
            return streamingLevels_;
        }

        [[nodiscard]] bool HasStreamingLevels() const noexcept
        {
            return !streamingLevels_.empty();
        }

        [[nodiscard]] std::size_t GetPersistentActorCount() const noexcept
        {
            return persistentActors_.size();
        }

        [[nodiscard]] std::size_t GetStreamingLevelCount() const noexcept
        {
            return streamingLevels_.size();
        }

        /**
         * @brief 序列化为 cooked 二进制。
         *
         * 格式布局：
         * - CookedHeader
         * - 持久关卡 Actor Header 数组
         * - Level Header 数组
         * - 各 streaming level 的 Actor Header 数组
         * - 所有字符串池（以 '\0' 结尾）
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

            auto append_struct = [&append_bytes]<typename T>(const T& value)
            {
                append_bytes(&value, sizeof(T));
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
            header.flags = 0;
            header.gravityX = worldSettings_.gravityX;
            header.gravityY = worldSettings_.gravityY;
            header.gravityZ = worldSettings_.gravityZ;
            header.enableStreaming = worldSettings_.enableStreaming ? 1 : 0;

            std::vector<CookedActorHeader> persistentHeaders;
            persistentHeaders.reserve(persistentActors_.size());

            header.persistentActorOffset = static_cast<std::uint32_t>(bytes.size());
            header.persistentActorCount = static_cast<std::uint32_t>(persistentActors_.size());

            for (std::size_t i = 0; i < persistentActors_.size(); ++i)
            {
                persistentHeaders.emplace_back();
                append_struct(persistentHeaders.back());
            }

            std::vector<CookedLevelHeader> levelHeaders;
            levelHeaders.reserve(streamingLevels_.size());

            header.levelOffset = static_cast<std::uint32_t>(bytes.size());
            header.levelCount = static_cast<std::uint32_t>(streamingLevels_.size());

            for (std::size_t i = 0; i < streamingLevels_.size(); ++i)
            {
                levelHeaders.emplace_back();
                append_struct(levelHeaders.back());
            }

            std::vector<std::vector<CookedActorHeader>> levelActorHeaders;
            levelActorHeaders.resize(streamingLevels_.size());

            for (std::size_t levelIndex = 0; levelIndex < streamingLevels_.size(); ++levelIndex)
            {
                const auto& level = streamingLevels_[levelIndex];
                auto& cookedActors = levelActorHeaders[levelIndex];
                cookedActors.reserve(level.actors.size());

                levelHeaders[levelIndex].actorOffset = static_cast<std::uint32_t>(bytes.size());
                levelHeaders[levelIndex].actorCount = static_cast<std::uint32_t>(level.actors.size());
                levelHeaders[levelIndex].flags = level.flags;

                for (std::size_t actorIndex = 0; actorIndex < level.actors.size(); ++actorIndex)
                {
                    cookedActors.emplace_back();
                    append_struct(cookedActors.back());
                }
            }

            header.nameOffset = append_string(GetName());
            header.logicalPathOffset = append_string(GetLogicalPath());
            header.worldNameOffset = append_string(worldName_.view());

            auto fill_actor_header =
                [&append_string](CookedActorHeader& cooked, const ActorSpawnDefinition& actor)
            {
                cooked.actorNameOffset = append_string(actor.actorName.view());
                cooked.classPathOffset = append_string(actor.classPath.view());
                cooked.flags = actor.flags;
                cooked.positionX = actor.positionX;
                cooked.positionY = actor.positionY;
                cooked.positionZ = actor.positionZ;
                cooked.rotationPitch = actor.rotationPitch;
                cooked.rotationYaw = actor.rotationYaw;
                cooked.rotationRoll = actor.rotationRoll;
                cooked.scaleX = actor.scaleX;
                cooked.scaleY = actor.scaleY;
                cooked.scaleZ = actor.scaleZ;
            };

            for (std::size_t i = 0; i < persistentActors_.size(); ++i)
            {
                fill_actor_header(persistentHeaders[i], persistentActors_[i]);
            }

            for (std::size_t i = 0; i < streamingLevels_.size(); ++i)
            {
                levelHeaders[i].nameOffset = append_string(streamingLevels_[i].levelName.view());

                const auto& actors = streamingLevels_[i].actors;
                auto& cookedActors = levelActorHeaders[i];
                for (std::size_t j = 0; j < actors.size(); ++j)
                {
                    fill_actor_header(cookedActors[j], actors[j]);
                }
            }

            auto patch_struct = [&bytes]<typename T>(std::size_t offset, const T& value)
            {
                if (offset + sizeof(T) > bytes.size())
                {
                    return;
                }

                std::memcpy(bytes.data() + static_cast<std::ptrdiff_t>(offset), &value, sizeof(T));
            };

            patch_struct(0, header);

            if (!persistentHeaders.empty())
            {
                const auto offset = static_cast<std::size_t>(header.persistentActorOffset);
                std::memcpy(
                    bytes.data() + static_cast<std::ptrdiff_t>(offset),
                    persistentHeaders.data(),
                    persistentHeaders.size() * sizeof(CookedActorHeader));
            }

            if (!levelHeaders.empty())
            {
                const auto offset = static_cast<std::size_t>(header.levelOffset);
                std::memcpy(
                    bytes.data() + static_cast<std::ptrdiff_t>(offset),
                    levelHeaders.data(),
                    levelHeaders.size() * sizeof(CookedLevelHeader));
            }

            for (std::size_t i = 0; i < levelHeaders.size(); ++i)
            {
                const auto& actors = levelActorHeaders[i];
                if (actors.empty())
                {
                    continue;
                }

                const auto offset = static_cast<std::size_t>(levelHeaders[i].actorOffset);
                std::memcpy(
                    bytes.data() + static_cast<std::ptrdiff_t>(offset),
                    actors.data(),
                    actors.size() * sizeof(CookedActorHeader));
            }

            return bytes;
        }

        /**
         * @brief 从 cooked 二进制反序列化 RuntimeMapAsset。
         */
        [[nodiscard]] static std::expected<std::shared_ptr<RuntimeMapAsset>, shine::SString> DeserializeCooked(
            shine::AssetID assetID,
            std::span<const std::byte> bytes)
        {
            if (bytes.size() < sizeof(CookedHeader))
            {
                return std::unexpected(MakeError("Cooked map payload is too small."));
            }

            CookedHeader header{};
            std::memcpy(&header, bytes.data(), sizeof(CookedHeader));

            if (header.magic != kCookedMagic)
            {
                return std::unexpected(MakeError("Cooked map magic is invalid."));
            }

            if (header.version != kCookedVersion)
            {
                return std::unexpected(MakeError("Cooked map version is not supported."));
            }

            if (header.headerSize < sizeof(CookedHeader))
            {
                return std::unexpected(MakeError("Cooked map header size is invalid."));
            }

            const auto read_c_string =
                [&bytes](std::uint32_t offset) -> std::expected<shine::SString, shine::SString>
            {
                if (offset >= bytes.size())
                {
                    return std::unexpected(MakeError("Cooked map string offset is out of range."));
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
                    return std::unexpected(MakeError("Cooked map string is not null terminated."));
                }

                return shine::SString(begin, static_cast<std::size_t>(it - begin));
            };

            const auto read_actor_headers =
                [&bytes](std::uint32_t offset, std::uint32_t count)
                -> std::expected<std::span<const CookedActorHeader>, shine::SString>
            {
                const auto totalBytes =
                    static_cast<std::uint64_t>(count) * static_cast<std::uint64_t>(sizeof(CookedActorHeader));

                if (offset > bytes.size() || totalBytes > (bytes.size() - offset))
                {
                    return std::unexpected(MakeError("Cooked map actor table is out of range."));
                }

                const auto* ptr =
                    reinterpret_cast<const CookedActorHeader*>(
                        bytes.data() + static_cast<std::ptrdiff_t>(offset));

                return std::span<const CookedActorHeader>(ptr, count);
            };

            const auto read_level_headers =
                [&bytes](std::uint32_t offset, std::uint32_t count)
                -> std::expected<std::span<const CookedLevelHeader>, shine::SString>
            {
                const auto totalBytes =
                    static_cast<std::uint64_t>(count) * static_cast<std::uint64_t>(sizeof(CookedLevelHeader));

                if (offset > bytes.size() || totalBytes > (bytes.size() - offset))
                {
                    return std::unexpected(MakeError("Cooked map level table is out of range."));
                }

                const auto* ptr =
                    reinterpret_cast<const CookedLevelHeader*>(
                        bytes.data() + static_cast<std::ptrdiff_t>(offset));

                return std::span<const CookedLevelHeader>(ptr, count);
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

            auto worldNameResult = read_c_string(header.worldNameOffset);
            if (!worldNameResult.has_value())
            {
                return std::unexpected(worldNameResult.error());
            }

            auto asset = std::make_shared<RuntimeMapAsset>();
            asset->InitRuntimeMap(assetID, std::move(nameResult.value()), logicalPathResult.value());
            asset->SetWorldName(std::move(worldNameResult.value()));

            WorldSettings settings;
            settings.gravityX = header.gravityX;
            settings.gravityY = header.gravityY;
            settings.gravityZ = header.gravityZ;
            settings.enableStreaming = header.enableStreaming != 0;
            asset->SetWorldSettings(settings);

            auto persistentHeaderResult =
                read_actor_headers(header.persistentActorOffset, header.persistentActorCount);
            if (!persistentHeaderResult.has_value())
            {
                return std::unexpected(persistentHeaderResult.error());
            }

            std::vector<ActorSpawnDefinition> persistentActors;
            persistentActors.reserve(header.persistentActorCount);

            for (const auto& cookedActor : persistentHeaderResult.value())
            {
                auto actorNameResult = read_c_string(cookedActor.actorNameOffset);
                if (!actorNameResult.has_value())
                {
                    return std::unexpected(actorNameResult.error());
                }

                auto classPathResult = read_c_string(cookedActor.classPathOffset);
                if (!classPathResult.has_value())
                {
                    return std::unexpected(classPathResult.error());
                }

                ActorSpawnDefinition actor;
                actor.actorName = std::move(actorNameResult.value());
                actor.classPath = std::move(classPathResult.value());
                actor.flags = cookedActor.flags;

                actor.positionX = cookedActor.positionX;
                actor.positionY = cookedActor.positionY;
                actor.positionZ = cookedActor.positionZ;

                actor.rotationPitch = cookedActor.rotationPitch;
                actor.rotationYaw = cookedActor.rotationYaw;
                actor.rotationRoll = cookedActor.rotationRoll;

                actor.scaleX = cookedActor.scaleX;
                actor.scaleY = cookedActor.scaleY;
                actor.scaleZ = cookedActor.scaleZ;

                persistentActors.push_back(std::move(actor));
            }

            asset->SetPersistentActors(std::move(persistentActors));

            auto levelHeaderResult = read_level_headers(header.levelOffset, header.levelCount);
            if (!levelHeaderResult.has_value())
            {
                return std::unexpected(levelHeaderResult.error());
            }

            std::vector<LevelDefinition> levels;
            levels.reserve(header.levelCount);

            for (const auto& cookedLevel : levelHeaderResult.value())
            {
                auto levelNameResult = read_c_string(cookedLevel.nameOffset);
                if (!levelNameResult.has_value())
                {
                    return std::unexpected(levelNameResult.error());
                }

                auto levelActorsResult =
                    read_actor_headers(cookedLevel.actorOffset, cookedLevel.actorCount);
                if (!levelActorsResult.has_value())
                {
                    return std::unexpected(levelActorsResult.error());
                }

                LevelDefinition level;
                level.levelName = std::move(levelNameResult.value());
                level.flags = cookedLevel.flags;
                level.actors.reserve(cookedLevel.actorCount);

                for (const auto& cookedActor : levelActorsResult.value())
                {
                    auto actorNameResult = read_c_string(cookedActor.actorNameOffset);
                    if (!actorNameResult.has_value())
                    {
                        return std::unexpected(actorNameResult.error());
                    }

                    auto classPathResult = read_c_string(cookedActor.classPathOffset);
                    if (!classPathResult.has_value())
                    {
                        return std::unexpected(classPathResult.error());
                    }

                    ActorSpawnDefinition actor;
                    actor.actorName = std::move(actorNameResult.value());
                    actor.classPath = std::move(classPathResult.value());
                    actor.flags = cookedActor.flags;

                    actor.positionX = cookedActor.positionX;
                    actor.positionY = cookedActor.positionY;
                    actor.positionZ = cookedActor.positionZ;

                    actor.rotationPitch = cookedActor.rotationPitch;
                    actor.rotationYaw = cookedActor.rotationYaw;
                    actor.rotationRoll = cookedActor.rotationRoll;

                    actor.scaleX = cookedActor.scaleX;
                    actor.scaleY = cookedActor.scaleY;
                    actor.scaleZ = cookedActor.scaleZ;

                    level.actors.push_back(std::move(actor));
                }

                levels.push_back(std::move(level));
            }

            asset->SetStreamingLevels(std::move(levels));
            asset->MarkClean();
            return asset;
        }

        /**
         * @brief RuntimeAssetLoader 反序列化桥接入口。
         */
        [[nodiscard]] static std::expected<std::shared_ptr<shine::editor::asset::AssetBase>, shine::SString>
        DeserializeForRuntimeLoader(const shine::RuntimeAssetLoader::DeserializerContext& context)
        {
            if (!context.IsValid())
            {
                return std::unexpected(MakeError("Runtime map deserializer context is invalid."));
            }

            if (context.kind != shine::EAssetKind::World)
            {
                return std::unexpected(MakeError("Runtime map deserializer received non-world asset kind."));
            }

            auto result = DeserializeCooked(context.assetID, context.bytes);
            if (!result.has_value())
            {
                return std::unexpected(result.error());
            }

            return std::static_pointer_cast<shine::editor::asset::AssetBase>(result.value());
        }

    private:
        [[nodiscard]] static shine::SString MakeError(shine::STextView text)
        {
            return shine::SString::from_view(text);
        }

    private:
        shine::SString worldName_;
        WorldSettings worldSettings_{};
        std::vector<ActorSpawnDefinition> persistentActors_;
        std::vector<LevelDefinition> streamingLevels_;
    };

    static_assert(sizeof(RuntimeMapAsset::CookedHeader) >= 40, "RuntimeMapAsset::CookedHeader layout changed unexpectedly.");
    static_assert(sizeof(RuntimeMapAsset::CookedLevelHeader) >= 16, "RuntimeMapAsset::CookedLevelHeader layout changed unexpectedly.");
    static_assert(sizeof(RuntimeMapAsset::CookedActorHeader) >= 48, "RuntimeMapAsset::CookedActorHeader layout changed unexpectedly.");
}
