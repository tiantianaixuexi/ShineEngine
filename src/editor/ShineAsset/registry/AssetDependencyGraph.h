#pragma once
// ============================================================
//  AssetDependencyGraph — directed dependency graph for editor assets.
//
//  EDITOR-ONLY — never include from runtime code.
//
//  Tracks which assets depend on which other assets so that the editor
//  can answer:
//    • "What does asset A depend on?"  (forward deps)
//    • "Which assets will break if I delete asset B?"  (reverse deps)
//    • "Does adding this dependency create a cycle?"
//
//  All edges reference assets by UUID string.
//  The graph is rebuilt / updated incrementally by EditorAssetRegistry.
// ============================================================

#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::editor::asset
{
    class AssetDependencyGraph
    {
    public:
        using DependencySet = std::unordered_set<SString,
                                                 SStringTransparentHash,
                                                 SStringTransparentEqual>;
        using ForwardMap = std::unordered_map<SString,
                                              std::vector<SString>,
                                              SStringTransparentHash,
                                              SStringTransparentEqual>;
        using ReverseMap = std::unordered_map<SString,
                                              DependencySet,
                                              SStringTransparentHash,
                                              SStringTransparentEqual>;

        AssetDependencyGraph()  = default;
        ~AssetDependencyGraph() = default;

        // -----------------------------------------------------------------------
        //  Graph mutation
        // -----------------------------------------------------------------------

        /// Replace all forward dependencies of `ownerUuid` with `deps`.
        /// Automatically updates the reverse-dependency index.
        void SetDependencies(STextView ownerUuid, std::span<const std::string> deps);

        /// Remove all edges originating from `ownerUuid` (call on asset deletion).
        void RemoveAsset(STextView ownerUuid);

        // -----------------------------------------------------------------------
        //  Query — forward
        // -----------------------------------------------------------------------

        /// Return the UUIDs that `ownerUuid` directly depends on.
        /// Returns an empty span if the asset has no registered dependencies.
        [[nodiscard]] const std::vector<SString>&
        GetDependencies(STextView ownerUuid) const;

        // -----------------------------------------------------------------------
        //  Query — reverse (callers of ownerUuid)
        // -----------------------------------------------------------------------

        /// Return the UUIDs of all assets that directly depend on `targetUuid`.
        /// This is the answer to "who will break if I delete this asset?"
        [[nodiscard]] const DependencySet&
        GetDependents(STextView targetUuid) const;

        /// Return true if any live asset directly depends on `targetUuid`.
        [[nodiscard]] bool HasDependents(STextView targetUuid) const;

        // -----------------------------------------------------------------------
        //  Cycle detection
        // -----------------------------------------------------------------------

        /// Return true if adding the edge owner→newDep would form a cycle.
        /// Use before calling SetDependencies to guard against circular refs.
        [[nodiscard]] bool WouldCreateCycle(STextView ownerUuid, STextView newDep) const;

        // -----------------------------------------------------------------------
        //  Bulk
        // -----------------------------------------------------------------------

        void Clear();

        [[nodiscard]] std::size_t NodeCount() const { return m_forward.size(); }

    private:
        // UUID → list of UUIDs this asset depends on (forward edges)
        ForwardMap m_forward;

        // UUID → set of UUIDs that depend on this asset (reverse edges)
        ReverseMap m_reverse;

        // Sentinel empty containers returned for unknown UUIDs
        static const std::vector<SString>            s_emptyVec;
        static const DependencySet                   s_emptySet;

        // DFS reachability check for cycle detection
        [[nodiscard]] bool IsReachable(STextView from, STextView target) const;
    };

} // namespace shine::editor::asset
