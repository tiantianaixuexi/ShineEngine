#pragma once
// ============================================================
//  RuntimeAssetRegistry — runtime UUID → loaded asset map.
//
//  Responsibilities:
//    • Own the lifetime of every loaded AssetBase via shared_ptr.
//    • Map creator functions (AssetCreatorFn) per type ID.
//    • Provide thread-safe Find / Register / Unregister.
//    • Trigger async load requests via RequestLoad.
//
//  This registry is the single source of truth for which assets
//  are currently live in memory.  AssetHandle<T>::Resolve() calls
//  Find() through this class.
//
//  Design note: this registry is a runtime concern and has no
//  knowledge of .sasset files or import settings — those are
//  editor-only (EditorAssetRegistry).
// ============================================================

#include <memory>
#include <mutex>
#include <unordered_map>

#include "AssetBase.h"
#include "AssetFactory.h"
#include "EngineCore/subsystem.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::asset
{
    // -----------------------------------------------------------------------
    //  Load request result — returned by RequestLoad.
    // -----------------------------------------------------------------------
    enum class ELoadResult : std::uint8_t
    {
        Queued,         // load request enqueued successfully
        AlreadyLoaded,  // asset is already in Loaded state
        AlreadyLoading, // a load is already in flight
        NoFactory,      // no creator registered for this type ID
        InvalidUUID,    // UUID string is empty or malformed
    };

    // -----------------------------------------------------------------------
    //  RuntimeAssetRegistry
    // -----------------------------------------------------------------------
    class RuntimeAssetRegistry : public shine::Subsystem
    {
    public:
        using AssetMap = std::unordered_map<SString,
                                            std::shared_ptr<AssetBase>,
                                            SStringTransparentHash,
                                            SStringTransparentEqual>;
        using FactoryMap = std::unordered_map<SString,
                                              AssetCreatorFn,
                                              SStringTransparentHash,
                                              SStringTransparentEqual>;

        RuntimeAssetRegistry()  = default;
        ~RuntimeAssetRegistry() override = default;

        // Non-copyable, non-movable singleton-friendly class.
        RuntimeAssetRegistry(const RuntimeAssetRegistry&)            = delete;
        RuntimeAssetRegistry& operator=(const RuntimeAssetRegistry&) = delete;

        // -----------------------------------------------------------------------
        //  Factory registration
        //  Call once per asset type at startup (before any load requests).
        // -----------------------------------------------------------------------

        /// Register a creator function for a given type ID string.
        /// Overwrites any previously registered creator for that type.
        void RegisterFactory(STextView typeId, AssetCreatorFn creator);

        /// Remove the creator for a type ID.
        void UnregisterFactory(STextView typeId);

        // -----------------------------------------------------------------------
        //  Asset registration (used by loaders once data is ready)
        // -----------------------------------------------------------------------

        /// Insert a fully-constructed asset into the registry.
        /// The asset's UUID must be unique — asserts in debug if a duplicate
        /// is registered.
        void Register(std::shared_ptr<AssetBase> asset);

        /// Remove an asset by UUID.  Remaining shared_ptr holders outside
        /// the registry will keep the object alive until they release.
        void Unregister(STextView uuid);

        // -----------------------------------------------------------------------
        //  Query
        // -----------------------------------------------------------------------

        /// Find a registered asset by UUID.  Returns nullptr if not found.
        [[nodiscard]] std::shared_ptr<AssetBase> Find(STextView uuid) const;

        /// Typed find — returns nullptr if not found or wrong type.
        template<std::derived_from<AssetBase> T>
        [[nodiscard]] std::shared_ptr<T> FindAs(STextView uuid) const
        {
            return std::dynamic_pointer_cast<T>(Find(uuid));
        }

        /// Return true if an asset with this UUID is currently registered.
        [[nodiscard]] bool Contains(STextView uuid) const;

        // -----------------------------------------------------------------------
        //  Async load request
        //  The actual loading work is done on a background thread by the loader
        //  subsystem; this method only enqueues the request and creates the
        //  placeholder asset object in Loading state.
        //
        //  Returns the placeholder asset (in Loading state) so callers can
        //  cache a shared_ptr before the load completes, if needed.
        // -----------------------------------------------------------------------

        /// Request an asset to be loaded.
        /// typeId — required so the correct factory is used to allocate the object.
        /// Returns {result, placeholder} where placeholder is valid only when
        ///         result == Queued or AlreadyLoading/AlreadyLoaded.
        struct LoadRequest
        {
            ELoadResult                result;
            std::shared_ptr<AssetBase> asset;   // may be nullptr on failure
        };

        [[nodiscard]] LoadRequest RequestLoad(STextView uuid, STextView typeId);

        // -----------------------------------------------------------------------
        //  Bulk operations
        // -----------------------------------------------------------------------

        /// Unregister all assets and clear all factories.
        void Clear();

        /// Return the number of currently registered (live) assets.
        [[nodiscard]] std::size_t AssetCount() const;

    private:
        mutable std::mutex m_mutex;

        // UUID string → live asset
        AssetMap m_assets;

        // typeId string → creator function
        FactoryMap m_factories;
    };

} // namespace shine::asset
