#include "ImporterAutoRegistry.h"

namespace shine::editor::asset
{
    ImporterAutoRegistry& ImporterAutoRegistry::Instance() noexcept
    {
        // Guaranteed to be initialised before any call from Registrar<T>
        // constructors because it is a function-local static.
        static ImporterAutoRegistry instance;
        return instance;
    }

    void ImporterAutoRegistry::Register(Factory factory)
    {
        factories_.push_back(std::move(factory));
    }

    std::vector<std::shared_ptr<IAssetImporter>> ImporterAutoRegistry::CreateAll() const
    {
        std::vector<std::shared_ptr<IAssetImporter>> result;
        result.reserve(factories_.size());
        for (const auto& factory : factories_)
            result.push_back(factory());
        return result;
    }

} // namespace shine::editor::asset
