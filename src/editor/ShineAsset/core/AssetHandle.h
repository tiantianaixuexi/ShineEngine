#pragma once
// ============================================================
//  AssetHandle<T> — stable, type-safe UUID-based asset reference.
//
//  Compiled into BOTH runtime and editor builds.
//
//  Design goals:
//    • UUID is the stable identity — moving or renaming an asset
//      on disk never invalidates an AssetHandle.
//    • No raw pointer storage — resolution always goes through
//      RuntimeAssetRegistry so lifetime is managed centrally.
//    • Null state is explicit and cheap to test.
//    • Fully hashable/comparable for use in maps and sets.
//    • Glaze-serializable as a plain UUID string (see glz::meta below).
//
//  Usage:
//      AssetHandle<TextureAsset> handle("a1b2c3d4-...");
//      if (auto tex = handle.Resolve(*registry))
//          tex->Bind(slot);
//
//  Serializing a handle in a struct:
//      struct Material {
//          AssetHandle<TextureAsset> albedo;   // stored as UUID in JSON
//      };
// ============================================================

#include <concepts>
#include <functional>   // std::hash
#include <memory>
#include <string>

#include "AssetBase.h"
#include "RuntimeAssetRegistry.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::asset
{

    // -----------------------------------------------------------------------
    //  AssetHandle<T>
    // -----------------------------------------------------------------------
    template<std::derived_from<AssetBase> T>
    class AssetHandle
    {
    public:
        // ----- Construction -------------------------------------------------

        /// Null handle — IsNull() returns true.
        AssetHandle() = default;

        /// Construct from a UUID string.
        explicit AssetHandle(STextView uuid) : m_uuid(uuid) {}

        // Allow implicit construction from string literal for convenience.
        AssetHandle(const char* uuid) : m_uuid(uuid) {} // NOLINT(google-explicit-constructor)

        // ----- Null / validity ----------------------------------------------

        [[nodiscard]] bool IsNull() const noexcept { return m_uuid.empty(); }
        explicit operator bool() const noexcept    { return !IsNull(); }

        // ----- Identity -----------------------------------------------------

        [[nodiscard]] STextView GetUUID() const noexcept { return m_uuid; }

        // ----- Resolution ---------------------------------------------------

        /// Resolve this handle to a live asset via the given registry.
        /// Returns nullptr if the UUID is not registered or the asset hasn't
        /// been loaded yet.  Callers should check IsNull() before calling.
        [[nodiscard]] std::shared_ptr<T>
        Resolve(RuntimeAssetRegistry& registry) const
        {
            if (IsNull())
                return nullptr;
            return registry.FindAs<T>(m_uuid);
        }

        // ----- Comparison / hash -------------------------------------------

        [[nodiscard]] bool operator==(const AssetHandle& rhs) const noexcept
        {
            return m_uuid == rhs.m_uuid;
        }
        [[nodiscard]] bool operator!=(const AssetHandle& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        // ----- Glaze serialization support ----------------------------------
        // Glaze will read/write this handle as a plain UUID string.
        // The inner `glaze` struct tells Glaze to treat AssetHandle<T> as
        // a transparent wrapper around m_uuid.

        struct glaze
        {
            using T2 = AssetHandle;
            // Serialize as the uuid string directly (not as an object).
            static constexpr auto value = &T2::m_uuid;
        };

    private:
        SString m_uuid;
    };

    // -----------------------------------------------------------------------
    //  Null handle helper
    // -----------------------------------------------------------------------
    template<std::derived_from<AssetBase> T>
    [[nodiscard]] inline AssetHandle<T> NullHandle() noexcept
    {
        return AssetHandle<T>{};
    }

} // namespace shine::asset

// -----------------------------------------------------------------------
//  std::hash specialisation — enables use in unordered_map/set
// -----------------------------------------------------------------------
namespace std
{
    template<typename T>
    struct hash<shine::asset::AssetHandle<T>>
    {
        std::size_t operator()(const shine::asset::AssetHandle<T>& h) const noexcept
        {
            return std::hash<std::string_view>{}(std::string_view(h.GetUUID().sv()));
        }
    };
} // namespace std
