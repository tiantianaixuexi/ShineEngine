#pragma once
// ============================================================
//  ImporterAutoRegistry — self-registration registry for IAssetImporter.
//
//  EDITOR-ONLY.
//
//  Goal: adding a new importer requires ONLY creating the importer class
//  and placing REGISTER_IMPORTER(MyImporter) in its .cpp file.
//  No existing source file ever needs to be modified.
//
//  How it works:
//    Each REGISTER_IMPORTER macro defines a file-scope static Registrar<T>
//    object.  Its constructor runs before main() and enqueues a factory
//    lambda in the global ImporterAutoRegistry singleton.  When
//    ImportPipeline::Init() is called (after all static initialisers have
//    run) it calls ImporterAutoRegistry::CreateAll() to instantiate and
//    register every discovered importer.
//
//  Linker safety:
//    The ShineAsset module uses a directory-based scan ("dirs"), so every
//    .cpp in that directory is compiled into the same static library target.
//    All object files are therefore included by the linker and the
//    static-initialiser trick is safe.
// ============================================================

#include <functional>
#include <memory>
#include <vector>

namespace shine::editor::asset
{
    class IAssetImporter;

    // -----------------------------------------------------------------------
    //  Central registry — singleton, filled at static-init time.
    // -----------------------------------------------------------------------
    class ImporterAutoRegistry
    {
    public:
        using Factory = std::function<std::shared_ptr<IAssetImporter>()>;

        /// Returns the process-wide singleton.
        [[nodiscard]] static ImporterAutoRegistry& Instance() noexcept;

        /// Called by Registrar<T> at static-init time.  Thread-safe: all
        /// calls happen before main() on a single thread.
        void Register(Factory factory);

        /// Instantiates one of each registered importer and returns them.
        /// Called once from ImportPipeline::Init().
        [[nodiscard]] std::vector<std::shared_ptr<IAssetImporter>> CreateAll() const;

    private:
        std::vector<Factory> factories_;
    };

    // -----------------------------------------------------------------------
    //  CRTP helper — one static instance per importer type.
    // -----------------------------------------------------------------------
    template<typename T>
    struct ImporterRegistrar
    {
        ImporterRegistrar()
        {
            ImporterAutoRegistry::Instance().Register(
                [] { return std::make_shared<T>(); });
        }
    };

} // namespace shine::editor::asset

// -----------------------------------------------------------------------
//  Public macro — drop this in any importer's .cpp to self-register it.
//
//  Example (GltfAssetImporter.cpp):
//      REGISTER_IMPORTER(GltfAssetImporter)
// -----------------------------------------------------------------------
#define REGISTER_IMPORTER(ClassName)                                         \
    static ::shine::editor::asset::ImporterRegistrar<ClassName>              \
        g_importer_registrar_##ClassName {}; /* NOLINT(cert-err58-cpp) */
