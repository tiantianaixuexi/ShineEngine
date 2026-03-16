#pragma once
// ============================================================
//  AssetBase — base class for all runtime asset objects.
//
//  This header is compiled into BOTH shipping and editor builds.
//  Editor-only metadata (source file, import settings, sub-asset
//  tree) lives in AssetMetadata.h — never include that from runtime.
//
//  Derive from AssetBase to create a concrete runtime asset type.
//  The open type-ID string (matching AssetTypeId constants) means
//  new asset types need zero changes here.
// ============================================================

#include <atomic>
#include <cstdint>

#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::asset
{
    // -----------------------------------------------------------------------
    //  Load state — replaces the old bool IsLoaded().
    //  Ordered so that (state >= EAssetState::Loaded) means "usable".
    // -----------------------------------------------------------------------
    enum class EAssetState : std::uint8_t
    {
        Unloaded = 0,  // not yet requested
        Loading,       // async load in progress
        Loaded,        // GPU/CPU data ready, safe to use
        Failed,        // load attempted but encountered an error
    };

    // -----------------------------------------------------------------------
    //  AssetBase
    // -----------------------------------------------------------------------
    class AssetBase
    {
    public:
        AssetBase() = default;

        explicit AssetBase(STextView uuid, STextView typeId)
            : m_uuid(uuid), m_typeId(typeId)
        {
        }

        virtual ~AssetBase() = default;

        // Assets are non-copyable; move is intentionally disabled too because
        // RuntimeAssetRegistry holds shared_ptr — move would invalidate handles.
        AssetBase(const AssetBase&)            = delete;
        AssetBase& operator=(const AssetBase&) = delete;
        AssetBase(AssetBase&&)                 = delete;
        AssetBase& operator=(AssetBase&&)      = delete;

        // -----------------------------------------------------------------------
        //  Identity (immutable after construction)
        // -----------------------------------------------------------------------

        /// RFC 9562 UUID string ("8-4-4-4-12" lower-case hex).
        [[nodiscard]] STextView GetUUID()   const noexcept { return m_uuid;   }

        /// String type ID matching AssetTypeId constants (e.g. "model", "texture").
        [[nodiscard]] STextView GetTypeId() const noexcept { return m_typeId; }

        // -----------------------------------------------------------------------
        //  Load state (thread-safe reads)
        // -----------------------------------------------------------------------

        [[nodiscard]] EAssetState GetState() const noexcept
        {
            return m_state.load(std::memory_order_acquire);
        }

        [[nodiscard]] bool IsLoaded()  const noexcept { return GetState() == EAssetState::Loaded;  }
        [[nodiscard]] bool IsLoading() const noexcept { return GetState() == EAssetState::Loading; }
        [[nodiscard]] bool HasFailed() const noexcept { return GetState() == EAssetState::Failed;  }

        // Called by the loader/registry to transition state (loader infrastructure).
        void SetState(EAssetState s) noexcept
        {
            m_state.store(s, std::memory_order_release);
        }

    private:
        SString                  m_uuid;
        SString                  m_typeId;
        std::atomic<EAssetState> m_state{ EAssetState::Unloaded };
    };

} // namespace shine::asset
