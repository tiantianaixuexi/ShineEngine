#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "EngineCore/subsystem.h"
#include "EngineCore/asset/AssetRegistryFormat.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"
#include "util/file_util.ixx"

namespace shine
{
    /**
     * @brief 运行时 bundle 读取器。
     *
     * 该子系统负责：
     * - 打开并缓存 bundle 文件映射
     * - 按 RegistryEntry / bundleIndex / offset / size 读取资产字节
     * - 为 RuntimeRegistry / RuntimeAssetLoader 提供统一的底层字节访问接口
     *
     * 当前假设 bundle 文件是纯 payload 拼接格式：
     * - 文件内没有额外 header / toc
     * - RegistryEntry::offset / size 直接对应 bundle 文件中的字节区间
     *
     * 若后续引入 bundle header/table，只需要在该类内部扩展偏移解析逻辑，
     * 对上层调用方接口可保持不变。
     */
    class RuntimeBundleReader final : public Subsystem
    {
    public:
        struct BundleSource
        {
            std::uint32_t bundleIndex = 0;
            shine::SString bundlePath;

            [[nodiscard]] constexpr bool IsValid() const noexcept
            {
                return bundleIndex != 0 && !bundlePath.empty();
            }
        };

        struct BundleView
        {
            std::uint32_t bundleIndex = 0;
            std::span<const std::byte> bytes{};

            [[nodiscard]] bool IsValid() const noexcept
            {
                return bundleIndex != 0 && !bytes.empty();
            }

            [[nodiscard]] std::size_t Size() const noexcept
            {
                return bytes.size();
            }

            [[nodiscard]] const std::byte* Data() const noexcept
            {
                return bytes.data();
            }
        };

        struct BundleReadResult
        {
            std::uint32_t bundleIndex = 0;
            std::uint64_t offset = 0;
            std::uint64_t size = 0;
            std::span<const std::byte> bytes{};

            [[nodiscard]] bool IsValid() const noexcept
            {
                return bundleIndex != 0 && size > 0 && bytes.size() == size;
            }

            [[nodiscard]] const std::byte* Data() const noexcept
            {
                return bytes.data();
            }

            [[nodiscard]] std::size_t ByteCount() const noexcept
            {
                return bytes.size();
            }
        };

        RuntimeBundleReader() = default;
        ~RuntimeBundleReader() override = default;

        RuntimeBundleReader(const RuntimeBundleReader&) = delete;
        RuntimeBundleReader& operator=(const RuntimeBundleReader&) = delete;
        RuntimeBundleReader(RuntimeBundleReader&&) = delete;
        RuntimeBundleReader& operator=(RuntimeBundleReader&&) = delete;

        bool Init(EngineContext& ctx) override
        {
            (void)ctx;
            Reset();
            return true;
        }

        void Shutdown(EngineContext& ctx) override
        {
            (void)ctx;
            Reset();
        }

        /**
         * @brief 清空已注册 bundle 路径与已打开映射。
         */
        void Reset() noexcept
        {
            bundlePaths_.clear();
            openBundles_.clear();
        }

        /**
         * @brief 注册 bundle 索引到物理文件路径的映射。
         * @param bundleIndex bundle 索引
         * @param bundlePath bundle 文件路径
         * @return 注册成功返回 true
         */
        [[nodiscard]] bool RegisterBundlePath(
            std::uint32_t bundleIndex,
            shine::STextView bundlePath)
        {
            if (bundleIndex == 0 || bundlePath.empty())
            {
                return false;
            }

            bundlePaths_.insert_or_assign(bundleIndex, shine::SString::from_view(bundlePath));
            return true;
        }

        /**
         * @brief 批量注册 bundle 路径映射。
         * @param sources bundle 来源列表
         */
        void RegisterBundlePaths(std::span<const BundleSource> sources)
        {
            for (const auto& source : sources)
            {
                if (!source.IsValid())
                {
                    continue;
                }

                RegisterBundlePath(source.bundleIndex, source.bundlePath.view());
            }
        }

        /**
         * @brief 移除某个 bundle 的路径映射及已打开缓存。
         * @param bundleIndex bundle 索引
         * @return 若存在并移除成功返回 true
         */
        [[nodiscard]] bool UnregisterBundle(std::uint32_t bundleIndex) noexcept
        {
            openBundles_.erase(bundleIndex);
            return bundlePaths_.erase(bundleIndex) > 0;
        }

        /**
         * @brief 检查是否已注册 bundle 路径。
         */
        [[nodiscard]] bool HasBundlePath(std::uint32_t bundleIndex) const noexcept
        {
            return bundlePaths_.find(bundleIndex) != bundlePaths_.end();
        }

        /**
         * @brief 获取已注册的 bundle 路径。
         */
        [[nodiscard]] std::expected<shine::STextView, std::string> GetBundlePath(
            std::uint32_t bundleIndex) const noexcept
        {
            const auto it = bundlePaths_.find(bundleIndex);
            if (it == bundlePaths_.end())
            {
                return std::unexpected("Bundle path is not registered.");
            }

            return it->second.view();
        }

        /**
         * @brief 打开并缓存某个 bundle。
         * @param bundleIndex bundle 索引
         * @return 成功返回 true
         */
        [[nodiscard]] bool OpenBundle(std::uint32_t bundleIndex)
        {
            if (bundleIndex == 0)
            {
                return false;
            }

            if (openBundles_.find(bundleIndex) != openBundles_.end())
            {
                return true;
            }

            const auto pathResult = GetBundlePath(bundleIndex);
            if (!pathResult.has_value())
            {
                return false;
            }

#ifndef SHINE_PLATFORM_WASM
            auto mapped = shine::util::read_full_file(pathResult.value());
            if (!mapped.has_value())
            {
                return false;
            }

            OpenBundleState state;
            state.bundleIndex = bundleIndex;
            state.path = shine::SString::from_view(pathResult.value());
            state.mappedFile = std::move(mapped.value());
            openBundles_.emplace(bundleIndex, std::move(state));
#else
            bool success = false;
            auto mapped = shine::util::read_full_file(pathResult.value(), &success);
            if (!success)
            {
                return false;
            }

            OpenBundleState state;
            state.bundleIndex = bundleIndex;
            state.path = shine::SString::from_view(pathResult.value());
            state.mappedFile = std::move(mapped);
            openBundles_.emplace(bundleIndex, std::move(state));
#endif

            return true;
        }

        /**
         * @brief 关闭某个 bundle 的已打开缓存。
         */
        [[nodiscard]] bool CloseBundle(std::uint32_t bundleIndex) noexcept
        {
            return openBundles_.erase(bundleIndex) > 0;
        }

        /**
         * @brief 检查 bundle 是否已打开。
         */
        [[nodiscard]] bool IsBundleOpen(std::uint32_t bundleIndex) const noexcept
        {
            return openBundles_.find(bundleIndex) != openBundles_.end();
        }

        /**
         * @brief 获取已打开 bundle 的总数。
         */
        [[nodiscard]] std::size_t GetOpenBundleCount() const noexcept
        {
            return openBundles_.size();
        }

        /**
         * @brief 获取已注册 bundle 的总数。
         */
        [[nodiscard]] std::size_t GetRegisteredBundleCount() const noexcept
        {
            return bundlePaths_.size();
        }

        /**
         * @brief 读取整个 bundle 视图。
         */
        [[nodiscard]] std::expected<BundleView, std::string> GetBundleView(
            std::uint32_t bundleIndex)
        {
            auto* state = EnsureBundleOpen(bundleIndex);
            if (state == nullptr)
            {
                return std::unexpected("Failed to open runtime bundle.");
            }

            const auto bytes = GetMappedBytes(*state);
            if (bytes.empty())
            {
                return std::unexpected("Runtime bundle is empty.");
            }

            return BundleView{
                .bundleIndex = bundleIndex,
                .bytes = bytes
            };
        }

        /**
         * @brief 按 bundleIndex / offset / size 读取资产字节区间。
         */
        [[nodiscard]] std::expected<BundleReadResult, std::string> Read(
            std::uint32_t bundleIndex,
            std::uint64_t offset,
            std::uint64_t size)
        {
            if (bundleIndex == 0)
            {
                return std::unexpected("Bundle index is invalid.");
            }

            if (size == 0)
            {
                return std::unexpected("Read size must be greater than zero.");
            }

            auto* state = EnsureBundleOpen(bundleIndex);
            if (state == nullptr)
            {
                return std::unexpected("Failed to open runtime bundle.");
            }

            const auto bytes = GetMappedBytes(*state);
            if (!ValidateRange(bytes, offset, size))
            {
                return std::unexpected("Requested bundle read range is out of bounds.");
            }

            const auto resultBytes = bytes.subspan(
                static_cast<std::size_t>(offset),
                static_cast<std::size_t>(size));

            return BundleReadResult{
                .bundleIndex = bundleIndex,
                .offset = offset,
                .size = size,
                .bytes = resultBytes
            };
        }

        /**
         * @brief 按 RegistryEntry 读取资产字节区间。
         */
        [[nodiscard]] std::expected<BundleReadResult, std::string> Read(
            const shine::RegistryEntry& entry)
        {
            if (!shine::IsValidRegistryEntry(entry))
            {
                return std::unexpected("Registry entry is invalid.");
            }

            return Read(entry.bundleIndex, entry.offset, entry.size);
        }

        /**
         * @brief 按 RegistryEntry 拷贝资产字节到输出缓冲区。
         */
        [[nodiscard]] std::expected<std::vector<std::byte>, std::string> ReadBytes(
            const shine::RegistryEntry& entry)
        {
            auto readResult = Read(entry);
            if (!readResult.has_value())
            {
                return std::unexpected(readResult.error());
            }

            const auto view = readResult.value().bytes;
            return std::vector<std::byte>(view.begin(), view.end());
        }

        /**
         * @brief 按 bundleIndex / offset / size 拷贝资产字节到输出缓冲区。
         */
        [[nodiscard]] std::expected<std::vector<std::byte>, std::string> ReadBytes(
            std::uint32_t bundleIndex,
            std::uint64_t offset,
            std::uint64_t size)
        {
            auto readResult = Read(bundleIndex, offset, size);
            if (!readResult.has_value())
            {
                return std::unexpected(readResult.error());
            }

            const auto view = readResult.value().bytes;
            return std::vector<std::byte>(view.begin(), view.end());
        }

    private:
        struct OpenBundleState
        {
            std::uint32_t bundleIndex = 0;
            shine::SString path;
            shine::util::FileMapView mappedFile{};
        };

    private:
        [[nodiscard]] static std::span<const std::byte> GetMappedBytes(
            const OpenBundleState& state) noexcept
        {
#ifndef SHINE_PLATFORM_WASM
            return state.mappedFile.view.content;
#else
            return std::span<const std::byte>(
                state.mappedFile.view.data(),
                state.mappedFile.view.size());
#endif
        }

        [[nodiscard]] static bool ValidateRange(
            std::span<const std::byte> bytes,
            std::uint64_t offset,
            std::uint64_t size) noexcept
        {
            if (size == 0)
            {
                return false;
            }

            const auto total = static_cast<std::uint64_t>(bytes.size());
            if (offset > total)
            {
                return false;
            }

            if (size > (total - offset))
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] OpenBundleState* EnsureBundleOpen(std::uint32_t bundleIndex)
        {
            if (auto it = openBundles_.find(bundleIndex); it != openBundles_.end())
            {
                return &it->second;
            }

            if (!OpenBundle(bundleIndex))
            {
                return nullptr;
            }

            auto it = openBundles_.find(bundleIndex);
            if (it == openBundles_.end())
            {
                return nullptr;
            }

            return &it->second;
        }

    private:
        std::unordered_map<std::uint32_t, shine::SString> bundlePaths_;
        std::unordered_map<std::uint32_t, OpenBundleState> openBundles_;
    };
}