#include "EditorAssetRegistry.h"

#include "EditorAssetRegistryIndex.h"
#include "EngineCore/engine_context.h"
#include "util/EngineDirectoryService.h"
#include "util/watcher/FileWatchService.h"

namespace shine::editor::asset
{
    namespace
    {
        class PathLookupKey
        {
        public:
            explicit PathLookupKey(const std::filesystem::path& path)
                : m_storage(path.string())
                , m_view(STextView::from_string(m_storage))
            {
            }

            [[nodiscard]] STextView View() const noexcept
            {
                return m_view;
            }

        private:
            std::string m_storage;
            STextView   m_view;
        };

        [[nodiscard]] SString MakePathKey(const std::filesystem::path& path)
        {
            return SString(path.string());
        }
    }

    EditorAssetRegistry::EditorAssetRegistry() = default;

    // -----------------------------------------------------------------------
    //  Subsystem lifecycle
    // -----------------------------------------------------------------------

    bool EditorAssetRegistry::Init(EngineContext& ctx)
    {
        auto* dirService = ctx.GetSystem<util::EngineDirectoryService>();
        if (!dirService)
            return true; // no directory service — nothing to scan

        const auto& contentRoot = dirService->GetContentDirectory();
        if (contentRoot.empty())
            return true;

        // Try fast-startup from index
        auto indexPath = contentRoot / ".assetindex";
        std::vector<std::filesystem::path> staleFiles;
        if (LoadRegistryIndex(*this, indexPath, staleFiles))
        {
            // Re-scan stale files individually
            for (const auto& stalePath : staleFiles)
            {
                auto result = ReadAssetMetadataFile(stalePath.string());
                if (result)
                    Register(stalePath, std::move(result->asset));
            }
        }
        else
        {
            // No index or corrupted — full scan
            Scan(contentRoot);
        }

        // Connect to FileWatchService for hot-reload
        auto* fileWatchService = ctx.GetSystem<util::watcher::FileWatchService>();
        if (fileWatchService)
        {
            m_watcherHandle = fileWatchService->OnFileChanged.bind(
                [this](const util::watcher::FileChangeEvent& event)
                {
                    OnFileChangeEvent(event);
                });
        }

        return true;
    }

    void EditorAssetRegistry::Shutdown(EngineContext& ctx)
    {
        // Disconnect file watcher
        auto* fileWatchService = ctx.GetSystem<util::watcher::FileWatchService>();
        if (fileWatchService && m_watcherHandle)
        {
            fileWatchService->OnFileChanged.unbind(m_watcherHandle);
            m_watcherHandle = {};
        }

        auto* dirService = ctx.GetSystem<util::EngineDirectoryService>();
        if (dirService)
        {
            const auto& contentRoot = dirService->GetContentDirectory();
            if (!contentRoot.empty())
            {
                auto indexPath = contentRoot / ".assetindex";
                (void)SaveRegistryIndex(*this, indexPath);
            }
        }
    }

    // -----------------------------------------------------------------------
    //  Discovery
    // -----------------------------------------------------------------------

    std::size_t EditorAssetRegistry::Scan(const std::filesystem::path& contentRoot)
    {
        std::size_t count = 0;
        for (auto& entry : std::filesystem::recursive_directory_iterator(
                 contentRoot,
                 std::filesystem::directory_options::skip_permission_denied))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".sasset")
                continue;

            std::string pathStr = entry.path().string();
            auto result = ReadAssetMetadataFile(pathStr);
            if (!result)
                continue;   // malformed file — skip silently

            Register(entry.path(), std::move(result->asset));
            ++count;
        }
        return count;
    }

    // -----------------------------------------------------------------------
    //  Registration
    // -----------------------------------------------------------------------

    void EditorAssetRegistry::Register(const std::filesystem::path& diskPath,
                                       AssetRecord record)
    {
        SString uuid(record.uuid);
        SString pathKey = MakePathKey(diskPath);

        // Update path index: remove old mapping if path changed
        if (auto existing = m_entries.find(uuid); existing != m_entries.end())
        {
            UpdatePathIndex(existing->second.diskPath, pathKey, uuid);
            existing->second.diskPath  = pathKey;
            existing->second.record    = std::move(record);
            existing->second.isDangling = false;
        }
        else
        {
            m_pathIndex[pathKey] = uuid;
            EditorAssetEntry e;
            e.uuid     = uuid;
            e.diskPath = pathKey;
            e.record   = std::move(record);
            m_entries.emplace(uuid, std::move(e));
        }

        RebuildDepsForEntry(m_entries.at(uuid));
    }

    // -----------------------------------------------------------------------
    //  Relocation
    // -----------------------------------------------------------------------

    void EditorAssetRegistry::OnFileMoved(const std::filesystem::path& oldPath,
                                          const std::filesystem::path& newPath)
    {
        PathLookupKey oldPathKey(oldPath);
        auto pit = m_pathIndex.find(oldPathKey.View());
        if (pit == m_pathIndex.end())
            return;

        SString uuid = pit->second;
        SString newPathKey = MakePathKey(newPath);

        UpdatePathIndex(oldPathKey.View(), newPathKey, uuid);

        if (auto it = m_entries.find(uuid); it != m_entries.end())
        {
            it->second.diskPath  = newPathKey;
            it->second.isDangling = false;
        }
    }

    // -----------------------------------------------------------------------
    //  Deletion
    // -----------------------------------------------------------------------

    std::vector<SString>
    EditorAssetRegistry::OnFileDeleted(const std::filesystem::path& path)
    {
        PathLookupKey pathKey(path);
        auto pit = m_pathIndex.find(pathKey.View());
        if (pit == m_pathIndex.end())
            return {};

        SString uuid = pit->second;
        m_pathIndex.erase(pit);

        std::vector<SString> affected;
        const auto& deps = m_deps.GetDependents(uuid);
        affected.assign(deps.begin(), deps.end());

        if (auto it = m_entries.find(uuid); it != m_entries.end())
            it->second.isDangling = true;

        return affected;
    }

    DeleteResult EditorAssetRegistry::TryDelete(STextView uuid, EDeletePolicy policy)
    {
        DeleteResult result;
        auto it = m_entries.find(uuid);
        if (it == m_entries.end())
            return result;   // unknown — nothing to do

        const auto& dependents = m_deps.GetDependents(uuid);

        if (!dependents.empty() && policy == EDeletePolicy::SafeOnly)
        {
            // Refuse — callers should inform the user which assets would break
            result.succeeded = false;
            result.affectedDependents.assign(dependents.begin(), dependents.end());
            return result;
        }

        // Collect affected before modification
        result.affectedDependents.assign(dependents.begin(), dependents.end());

        // Remove from path index
        m_pathIndex.erase(it->second.diskPath);

        // Remove from dependency graph
        RemoveDepsForEntry(uuid);

        // Remove entry
        m_entries.erase(it);

        result.succeeded = true;
        return result;
    }

    // -----------------------------------------------------------------------
    //  Query
    // -----------------------------------------------------------------------

    const EditorAssetEntry* EditorAssetRegistry::Find(STextView uuid) const
    {
        auto it = m_entries.find(uuid);
        return (it != m_entries.end()) ? &it->second : nullptr;
    }

    const EditorAssetEntry*
    EditorAssetRegistry::FindByPath(const std::filesystem::path& diskPath) const
    {
        PathLookupKey pathKey(diskPath);
        auto pit = m_pathIndex.find(pathKey.View());
        if (pit == m_pathIndex.end())
            return nullptr;
        auto it = m_entries.find(pit->second);
        return (it != m_entries.end()) ? &it->second : nullptr;
    }

    bool EditorAssetRegistry::IsKnown(STextView uuid) const
    {
        auto it = m_entries.find(uuid);
        return (it != m_entries.end()) && !it->second.isDangling;
    }

    bool EditorAssetRegistry::IsDangling(STextView uuid) const
    {
        auto it = m_entries.find(uuid);
        return (it != m_entries.end()) && it->second.isDangling;
    }

    const EditorAssetRegistry::DependencySet&
    EditorAssetRegistry::GetDependents(STextView uuid) const
    {
        return m_deps.GetDependents(uuid);
    }

    void EditorAssetRegistry::ForEach(const ForEachFn& fn) const
    {
        for (const auto& [uuid, entry] : m_entries)
            fn(entry);
    }

    std::size_t EditorAssetRegistry::EntryCount() const
    {
        return m_entries.size();
    }

    void EditorAssetRegistry::Clear()
    {
        m_entries.clear();
        m_pathIndex.clear();
        m_deps.Clear();
    }

    // -----------------------------------------------------------------------
    //  Internal helpers
    // -----------------------------------------------------------------------

    void EditorAssetRegistry::RebuildDepsForEntry(const EditorAssetEntry& entry)
    {
        m_deps.SetDependencies(entry.uuid, entry.record.dependencies);
    }

    void EditorAssetRegistry::RemoveDepsForEntry(STextView uuid)
    {
        m_deps.RemoveAsset(uuid);
    }

    void EditorAssetRegistry::UpdatePathIndex(STextView oldPath,
                                              const SString& newPath,
                                              STextView uuid)
    {
        m_pathIndex.erase(oldPath);
        m_pathIndex[newPath] = SString(uuid);
    }

    void EditorAssetRegistry::OnFileChangeEvent(const util::watcher::FileChangeEvent& event)
    {
        // Only handle .sasset files
        std::filesystem::path filePath = std::filesystem::path(event.directory) / event.filename;
        if (filePath.extension() != ".sasset")
            return;

        // FILE_ACTION_REMOVED
        if (event.action == 2)
        {
            OnFileDeleted(filePath);
            return;
        }

        // FILE_ACTION_MODIFIED or FILE_ACTION_ADDED
        if (event.action == 1 || event.action == 3)
        {
            // Re-read and re-register the asset
            auto result = ReadAssetMetadataFile(filePath.string());
            if (result)
                Register(filePath, std::move(result->asset));
            return;
        }

        // FILE_ACTION_RENAMED_OLD_NAME / FILE_ACTION_RENAMED_NEW_NAME
        // These come in pairs; we handle them via the browser's move logic
    }

} // namespace shine::editor::asset
