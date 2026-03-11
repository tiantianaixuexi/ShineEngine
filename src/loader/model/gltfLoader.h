#pragma once

#include "loader/core/loader.h"
#include "loader/model/model_loader.h"
#include "EngineCore/log/LogSystem.h"
#include <vector>
#include <expected>

#include "math/vector.ixx"
#include "string/shine_string.h"
#include "string/shine_text_view.h"
#include "math/vector2.h"

#include "third/tinygltf/tiny_gltf.h"

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

        virtual ~gltfLoader() = default;
        virtual bool loadFromMemory(const void* data, size_t size) override;
        virtual bool loadFromFile(const char* filePath) override;
        void unload() override;

        virtual const char* getName() const override { return "gltfLoader"; }
        virtual const char* getVersion() const override { return "1.0.0"; }


        std::vector<MeshData> extractMeshData() const override;
        std::expected<MeshData, SString> extractMeshDataByIndex(size_t meshIndex) const;
        size_t getMeshCount() const noexcept override;

        size_t getSceneCount() const { return _model.scenes.size(); }
        int getDefaultSceneIndex() const { return _model.defaultScene; }
        std::vector<int> getSceneRootNodes(int sceneIndex) const;

    private:
        bool parseFromMemory(const unsigned char* bytes, size_t size, bool preferBinary);
        bool loadImagesForModel();
        bool appendPrimitiveMeshData(std::vector<MeshData>& meshes, const tinygltf::Primitive& primitive, const tinygltf::Node& node, STextView meshName) const;
        static bool hasBinaryMagic(const unsigned char* bytes, size_t size);

    private:
        tinygltf::Model _model;
        SString _basePath;
    };
}
