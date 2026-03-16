#include "AssetDependencyGraph.h"

#include <stack>

namespace shine::editor::asset
{
    // Static sentinels
    const std::vector<SString>        AssetDependencyGraph::s_emptyVec;
    const std::unordered_set<SString> AssetDependencyGraph::s_emptySet;

    void AssetDependencyGraph::SetDependencies(
        STextView ownerUuid, std::span<const std::string> deps)
    {
        SString owner(ownerUuid);

        // Remove old reverse edges for this owner
        if (auto it = m_forward.find(owner); it != m_forward.end())
        {
            for (const SString& oldDep : it->second)
                m_reverse[oldDep].erase(owner);
        }

        // Build new forward edge list
        auto& fwd = m_forward[owner];
        fwd.clear();
        fwd.reserve(deps.size());
        for (const std::string& dep : deps)
        {
            SString depKey(dep);
            fwd.push_back(depKey);
            m_reverse[depKey].insert(owner);
        }
    }

    void AssetDependencyGraph::RemoveAsset(STextView ownerUuid)
    {
        SString owner(ownerUuid);

        // Remove forward edges and clean up reverse index
        if (auto it = m_forward.find(owner); it != m_forward.end())
        {
            for (const SString& dep : it->second)
            {
                if (auto rit = m_reverse.find(dep); rit != m_reverse.end())
                    rit->second.erase(owner);
            }
            m_forward.erase(it);
        }

        // Also remove reverse entries pointing TO this owner (it no longer exists)
        m_reverse.erase(owner);
    }

    const std::vector<SString>&
    AssetDependencyGraph::GetDependencies(STextView ownerUuid) const
    {
        auto it = m_forward.find(SString(ownerUuid));
        return (it != m_forward.end()) ? it->second : s_emptyVec;
    }

    const std::unordered_set<SString>&
    AssetDependencyGraph::GetDependents(STextView targetUuid) const
    {
        auto it = m_reverse.find(SString(targetUuid));
        return (it != m_reverse.end()) ? it->second : s_emptySet;
    }

    bool AssetDependencyGraph::HasDependents(STextView targetUuid) const
    {
        auto it = m_reverse.find(SString(targetUuid));
        return (it != m_reverse.end()) && !it->second.empty();
    }

    bool AssetDependencyGraph::WouldCreateCycle(
        STextView ownerUuid, STextView newDep) const
    {
        // A cycle forms if newDep can already reach ownerUuid.
        return IsReachable(newDep, ownerUuid);
    }

    bool AssetDependencyGraph::IsReachable(STextView from, STextView target) const
    {
        if (from == target)
            return true;

        // Iterative DFS to avoid stack overflow on deep trees
        std::stack<SString> stack;
        std::unordered_set<SString> visited;
        stack.push(SString(from));

        while (!stack.empty())
        {
            SString current = std::move(stack.top());
            stack.pop();

            if (!visited.insert(current).second)
                continue;

            auto it = m_forward.find(current);
            if (it == m_forward.end())
                continue;

            for (const SString& dep : it->second)
            {
                if (dep == SString(target))
                    return true;
                if (!visited.contains(dep))
                    stack.push(dep);
            }
        }
        return false;
    }

    void AssetDependencyGraph::Clear()
    {
        m_forward.clear();
        m_reverse.clear();
    }

} // namespace shine::editor::asset
