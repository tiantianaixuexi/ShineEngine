#pragma once


#include "loader/model/model_loader.h"
#include "EngineCore/log/LogSystem.h"
#include <vector>
#include <expected>

#include "string/shine_string.h"
#include "string/shine_text_view.h"

#include "loader/model/gltf/ShineGltf.h"

namespace shine::loader
{
    REGISTER_LOG_GROUP(GltfLoaderLog)

    std::expected<MeshData, SString> LoadGltfMeshFromFile(STextView filePath, size_t meshIndex = 0, IModelLoader::ProgressCallback progressCallback = nullptr);
    std::expected<MeshData, SString> LoadGltfMeshFromMemory(const void* data, size_t size, size_t meshIndex = 0, IModelLoader::ProgressCallback progressCallback = nullptr);

    class gltfLoader : public IModelLoader
    {
    public:
        gltfLoader()
        {
            addSupportedExtension("gltf");
            addSupportedExtension("glb");
        }

        gltfLoader(const gltfLoader&) = default;
        gltfLoader(gltfLoader&&)  noexcept = default;
        gltfLoader& operator=(gltfLoader&&) = default;

        ~gltfLoader() override = default;
        bool loadFromMemory(const void* data, size_t size) override;
        bool loadFromFile(const char* filePath) override;
        void unload() override;

        [[nodiscard]] const char* getName() const override { return "gltfLoader"; }
        [[nodiscard]] const char* getVersion() const override { return "1.0.0"; }


        [[nodiscard]] std::vector<MeshData> extractMeshData() const override;
        [[nodiscard]] std::expected<MeshData, SString> extractMeshDataByIndex(size_t meshIndex) const;
        [[nodiscard]] std::size_t getMeshCount() const noexcept override;

        [[nodiscard]] std::size_t getSceneCount() const { return _model.scenes.size(); }
        [[nodiscard]] int getDefaultSceneIndex() const { return _model.defaultScene; }
        [[nodiscard]] std::vector<int> getSceneRootNodes(int sceneIndex) const;

    private:
        bool loadImagesForModel();
        bool appendPrimitiveMeshData(std::vector<MeshData>& meshes, const gltf::GltfPrimitive& primitive, const gltf::GltfNode& node, STextView meshName) const;

    private:
        gltf::GltfModel _model;
        SString _basePath;
    };
}
