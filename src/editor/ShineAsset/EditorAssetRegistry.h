#pragma once
// ============================================================
//  EditorAssetRegistry — editor-side UUID → disk path + metadata.
//
//  EDITOR-ONLY — never include from runtime code.
//
//  This registry is the single source of truth for where every
//  known asset lives on disk.  Because asset references always use
//  UUID, renaming or moving a .sasset file requires only updating
//  the entry's diskPath — no dependent asset needs to change.
//
//  Key operations:
//    Scan         — discover all .sasset files under a content root
//    OnFileMoved  — update the path for one asset (relocation)
//    OnFileDeleted— mark entry as Dangling; callers can find
//                   all affected dependents via the dependency graph
//    Find         — fast UUID lookup
//    FindByPath   — reverse lookup by disk path
//
//  The dependency graph (AssetDependencyGraph) is kept in sync
//  automatically whenever metadata is registered or updated.
// ============================================================

#include <filesystem>
#include <functional>
#include <optional>
#include <vector>

#include "AssetDependencyGraph.h"
#include "AssetMetadata.h"
#include "EngineCore/subsystem.h"
#include "util/function/EventHandle.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::util::watcher
{
    struct FileChangeEvent;
}

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  A single entry in the editor registry
    // -----------------------------------------------------------------------
    struct EditorAssetEntry
    {
        SString     uuid;
        SString     diskPath;       // absolute path to the .sasset file
        AssetRecord record;         // parsed from the .sasset file
        bool        isDangling = false; // true when file was deleted from disk
    };

    // -----------------------------------------------------------------------
    //  Delete policy used by TryDelete
    // -----------------------------------------------------------------------
    enum class EDeletePolicy : std::uint8_t
    {
        SafeOnly,   // refuse if any live asset depends on this UUID
        Force,      // delete even if dependents exist (they become dangling)
    };

    // -----------------------------------------------------------------------
    //  Result of TryDelete
    // -----------------------------------------------------------------------
    struct DeleteResult
    {
        bool              succeeded = false;
        std::vector<SString> affectedDependents; // UUIDs that now have dangling refs
    };

    // -----------------------------------------------------------------------
    //  EditorAssetRegistry
    // -----------------------------------------------------------------------
    class EditorAssetRegistry : public shine::Subsystem
    {
    public:
        EditorAssetRegistry();
        ~EditorAssetRegistry() override = default;

        EditorAssetRegistry(const EditorAssetRegistry&)            = delete;
        EditorAssetRegistry& operator=(const EditorAssetRegistry&) = delete;

        // -----------------------------------------------------------------------
        //  Subsystem lifecycle
        // -----------------------------------------------------------------------
        bool Init(EngineContext& ctx) override;
        void Shutdown(EngineContext& ctx) override;

        // -----------------------------------------------------------------------
        //  Discovery
        // -----------------------------------------------------------------------

        /// Recursively scan `contentRoot` for *.sasset files and register them.
        /// Already-known entries whose paths are still valid are updated in-place.
        /// Returns the number of assets discovered (new + updated).
        std::size_t Scan(const std::filesystem::path& contentRoot);

        // -----------------------------------------------------------------------
        //  Registration (used directly by the importer pipeline)
        // -----------------------------------------------------------------------

        /// Register or update an asset entry from an already-parsed AssetRecord
        /// and its on-disk path.  The dependency graph is updated automatically.
        void Register(const std::filesystem::path& diskPath, AssetRecord record);

        // -----------------------------------------------------------------------
        //  Relocation & deletion (called by the file-watcher or editor UI)
        // -----------------------------------------------------------------------

        /// Notify the registry that a .sasset file was moved or renamed.
        /// The UUID remains the same; only the diskPath is updated.
        void OnFileMoved(const std::filesystem::path& oldPath,
                         const std::filesystem::path& newPath);

        /// Notify the registry that a .sasset file was deleted from disk.
        /// Marks the entry as Dangling.  Does NOT remove it from the registry
        /// so existing AssetHandle holders can detect the broken reference.
        ///
        /// Returns the UUIDs of all assets that had a dependency on this UUID
        /// (direct dependents only).
        std::vector<SString> OnFileDeleted(const std::filesystem::path& path);

        /// Attempt to logically delete an asset:
        ///   SafeOnly  — refuses if any other asset depends on it.
        ///   Force     — deletes and marks all dependents as having dangling refs.
        /// Does NOT touch the file system; the caller is responsible for
        /// removing the .sasset file after a successful delete.
        [[nodiscard]] DeleteResult
        TryDelete(STextView uuid, EDeletePolicy policy = EDeletePolicy::SafeOnly);

        // -----------------------------------------------------------------------
        //  Query
        // -----------------------------------------------------------------------

        /// Find an entry by UUID.  Returns nullptr if not found.
        [[nodiscard]] const EditorAssetEntry* Find(STextView uuid) const;

        /// Find an entry by its current disk path.  Returns nullptr if not found.
        [[nodiscard]] const EditorAssetEntry*
        FindByPath(const std::filesystem::path& diskPath) const;

        /// Return true if a non-dangling entry with this UUID exists.
        [[nodiscard]] bool IsKnown(STextView uuid) const;

        /// Return true if the entry exists but its file is gone.
        [[nodiscard]] bool IsDangling(STextView uuid) const;

        /// Return all direct dependents of `uuid` (reverse dependency lookup).
        [[nodiscard]] const std::unordered_set<SString>&
        GetDependents(STextView uuid) const;

        // -----------------------------------------------------------------------
        //  Iteration — for registry serialization / UI listing
        // -----------------------------------------------------------------------
        using ForEachFn = std::function<void(const EditorAssetEntry&)>;
        void ForEach(const ForEachFn& fn) const;

        [[nodiscard]] std::size_t EntryCount() const;

        // -----------------------------------------------------------------------
        //  Bulk
        // -----------------------------------------------------------------------
        void Clear();

        // -----------------------------------------------------------------------
        //  Access to the underlying dependency graph (for external tools)
        // -----------------------------------------------------------------------
        [[nodiscard]] const AssetDependencyGraph& DependencyGraph() const
        {
            return m_deps;
        }

    private:
        // UUID → entry
        std::unordered_map<SString, EditorAssetEntry> m_entries;

        // path string → UUID (reverse index for path-based lookup)
        std::unordered_map<SString, SString> m_pathIndex;

        // dependency graph kept in sync with m_entries
        AssetDependencyGraph m_deps;

        // File watcher connection
        util::EventHandle<const util::watcher::FileChangeEvent&>::Handle m_watcherHandle;

        // Internal helpers
        void RebuildDepsForEntry(const EditorAssetEntry& entry);
        void RemoveDepsForEntry(STextView uuid);
        void UpdatePathIndex(STextView oldPath, STextView newPath, STextView uuid);
        void OnFileChangeEvent(const util::watcher::FileChangeEvent& event);
    };

} // namespace shine::editor::asset
