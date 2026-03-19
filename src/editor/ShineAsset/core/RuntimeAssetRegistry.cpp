#include "RuntimeAssetRegistry.h"

#include <cassert>

namespace shine::asset
{
    void RuntimeAssetRegistry::RegisterFactory(STextView typeId, AssetCreatorFn creator)
    {
        std::scoped_lock lock(m_mutex);
        m_factories[SString(typeId)] = std::move(creator);
    }

    void RuntimeAssetRegistry::UnregisterFactory(STextView typeId)
    {
        std::scoped_lock lock(m_mutex);
        m_factories.erase(typeId);
    }

    void RuntimeAssetRegistry::Register(std::shared_ptr<AssetBase> asset)
    {
        assert(asset && "RuntimeAssetRegistry::Register — null asset");
        std::scoped_lock lock(m_mutex);
        SString key(asset->GetUUID());
        assert(!m_assets.contains(key) && "RuntimeAssetRegistry::Register — duplicate UUID");
        m_assets.emplace(std::move(key), std::move(asset));
    }

    void RuntimeAssetRegistry::Unregister(STextView uuid)
    {
        std::scoped_lock lock(m_mutex);
        m_assets.erase(uuid);
    }

    std::shared_ptr<AssetBase> RuntimeAssetRegistry::Find(STextView uuid) const
    {
        std::scoped_lock lock(m_mutex);
        auto it = m_assets.find(uuid);
        return (it != m_assets.end()) ? it->second : nullptr;
    }

    bool RuntimeAssetRegistry::Contains(STextView uuid) const
    {
        std::scoped_lock lock(m_mutex);
        return m_assets.contains(uuid);
    }

    RuntimeAssetRegistry::LoadRequest
    RuntimeAssetRegistry::RequestLoad(STextView uuid, STextView typeId)
    {
        if (uuid.empty())
            return { ELoadResult::InvalidUUID, nullptr };

        std::scoped_lock lock(m_mutex);

        // Already registered?
        if (auto it = m_assets.find(uuid); it != m_assets.end())
        {
            auto& existing = it->second;
            if (existing->GetState() == EAssetState::Loaded)
                return { ELoadResult::AlreadyLoaded, existing };
            return { ELoadResult::AlreadyLoading, existing };
        }

        // Find factory
        auto fit = m_factories.find(typeId);
        if (fit == m_factories.end())
            return { ELoadResult::NoFactory, nullptr };

        // Create placeholder in Loading state
        auto placeholder = fit->second(uuid);
        if (!placeholder)
            return { ELoadResult::NoFactory, nullptr };

        placeholder->SetState(EAssetState::Loading);
        SString key(uuid);
        m_assets.emplace(key, placeholder);

        // NOTE: The actual async load is dispatched by the caller (loader
        // subsystem) using the returned placeholder.  When load completes,
        // the loader calls placeholder->SetState(EAssetState::Loaded) and
        // the asset is immediately visible to all existing shared_ptr holders.

        return { ELoadResult::Queued, std::move(placeholder) };
    }

    void RuntimeAssetRegistry::Clear()
    {
        std::scoped_lock lock(m_mutex);
        m_assets.clear();
        m_factories.clear();
    }

    std::size_t RuntimeAssetRegistry::AssetCount() const
    {
        std::scoped_lock lock(m_mutex);
        return m_assets.size();
    }

} // namespace shine::asset
