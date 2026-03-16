#include "gltfLoader.h"

#include <functional>
#include <cstring>
#include <span>
#include <cstddef>
#include <limits>

#include "fmt/format.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

#include "loader/model/gltf/ShineGltf.h"

#include "math/vector.ixx"
#include "math/quat.h"

#include "util/timer/FunctionTimer.h"
#include "util/image_util.h"
#include "util/path_util.h"

namespace shine::loader
{
    REGISTER_LOG_GROUP_END(GltfLoaderLog)

    using namespace shine::math;

    namespace
    {
        struct AccessorView
        {
            const unsigned char* data = nullptr;
            size_t stride = 0;
            int componentType = 0;
            int type = 0;
            bool normalized = false;
            size_t count = 0;
        };

        void ensure_gltf_loader_log_categories()
        {
            static const bool initialized = []()
            {
                ADD_LOG_CATEGORY_WITH_CONSOLE(GltfLoaderLog, "parse", true)
                ADD_LOG_CATEGORY_WITH_CONSOLE(GltfLoaderLog, "extract", true)
                ADD_LOG_CATEGORY_WITH_CONSOLE(GltfLoaderLog, "accessor", true)
                return true;
            }();
            (void)initialized;
        }

        bool get_accessor_view(const gltf::GltfModel& model, int accessorIndex, AccessorView& outView)
        {
            ensure_gltf_loader_log_categories();
            if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size()))
            {
                SHINE_LOG_WARN(GltfLoaderLog, "accessor", "accessor invalid index={}, accessorCount={}", accessorIndex, model.accessors.size());
                return false;
            }

            const auto& accessor = model.accessors[static_cast<size_t>(accessorIndex)];
            if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
            {
                SHINE_LOG_WARN(
                    GltfLoaderLog,
                    "accessor",
                    "accessor {} invalid bufferView={}, bufferViewCount={}, sparse={}",
                    accessorIndex,
                    accessor.bufferView,
                    model.bufferViews.size(),
                    accessor.sparse.isSparse);
                return false;
            }

            const auto& bufferView = model.bufferViews[static_cast<size_t>(accessor.bufferView)];
            if (bufferView.buffer < 0 || bufferView.buffer >= static_cast<int>(model.buffers.size()))
            {
                SHINE_LOG_WARN(
                    GltfLoaderLog,
                    "accessor",
                    "accessor {} invalid buffer index, bufferView={}, buffer={}, bufferCount={}",
                    accessorIndex,
                    accessor.bufferView,
                    bufferView.buffer,
                    model.buffers.size());
                return false;
            }

            const auto& buffer = model.buffers[static_cast<size_t>(bufferView.buffer)];
            const size_t byteOffset = bufferView.byteOffset + accessor.byteOffset;
            if (byteOffset >= buffer.data.size())
            {
                SHINE_LOG_WARN(
                    GltfLoaderLog,
                    "accessor",
                    "accessor {} byteOffset overflow, bufferView={}, byteOffset={}, bufferSize={}",
                    accessorIndex,
                    accessor.bufferView,
                    byteOffset,
                    buffer.data.size());
                return false;
            }

            const int componentSize = gltf::GetComponentSizeInBytes(static_cast<u32>(accessor.componentType));
            const int components    = gltf::GetNumComponentsInType(static_cast<u32>(accessor.type));
            if (componentSize <= 0 || components <= 0)
            {
                SHINE_LOG_WARN(
                    GltfLoaderLog,
                    "accessor",
                    "accessor {} invalid type/component, componentType={}, type={}",
                    accessorIndex,
                    accessor.componentType,
                    accessor.type);
                return false;
            }

            const int strideValue = (bufferView.byteStride == 0)
                ? componentSize * components
                : static_cast<int>(bufferView.byteStride);

            outView.data = buffer.data.data() + byteOffset;
            outView.stride = (strideValue > 0) ? static_cast<size_t>(strideValue) : static_cast<size_t>(componentSize * components);
            outView.componentType = accessor.componentType;
            outView.type = accessor.type;
            outView.normalized = accessor.normalized;
            outView.count = accessor.count;
            return true;
        }

        float read_normalized_integer(int componentType, const unsigned char* data)
        {
            switch (componentType)
            {
                case gltf::COMPONENT_TYPE_BYTE:
                {
                    int8_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return std::max(-1.0f, static_cast<float>(value) / 127.0f);
                }
                case gltf::COMPONENT_TYPE_UNSIGNED_BYTE:
                {
                    uint8_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<float>(value) / 255.0f;
                }
                case gltf::COMPONENT_TYPE_SHORT:
                {
                    int16_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return std::max(-1.0f, static_cast<float>(value) / 32767.0f);
                }
                case gltf::COMPONENT_TYPE_UNSIGNED_SHORT:
                {
                    uint16_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<float>(value) / 65535.0f;
                }
                case gltf::COMPONENT_TYPE_UNSIGNED_INT:
                {
                    uint32_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<float>(value) / 4294967295.0f;
                }
                default:
                    return 0.0f;
            }
        }

        float read_float_component(const unsigned char* data, int componentType, bool normalized)
        {
            if (normalized)
            {
                return read_normalized_integer(componentType, data);
            }

            switch (componentType)
            {
                case gltf::COMPONENT_TYPE_FLOAT:
                {
                    float value = 0.0f;
                    std::memcpy(&value, data, sizeof(value));
                    return value;
                }
                case gltf::COMPONENT_TYPE_DOUBLE:
                {
                    double value = 0.0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<float>(value);
                }
                case gltf::COMPONENT_TYPE_BYTE:
                {
                    int8_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<float>(value);
                }
                case gltf::COMPONENT_TYPE_UNSIGNED_BYTE:
                {
                    uint8_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<float>(value);
                }
                case gltf::COMPONENT_TYPE_SHORT:
                {
                    int16_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<float>(value);
                }
                case gltf::COMPONENT_TYPE_UNSIGNED_SHORT:
                {
                    uint16_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<float>(value);
                }
                case gltf::COMPONENT_TYPE_INT:
                {
                    int32_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<float>(value);
                }
                case gltf::COMPONENT_TYPE_UNSIGNED_INT:
                {
                    uint32_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<float>(value);
                }
                default:
                    return 0.0f;
            }
        }

        uint32_t read_index_component(const unsigned char* data, int componentType)
        {
            switch (componentType)
            {
                case gltf::COMPONENT_TYPE_UNSIGNED_BYTE:
                {
                    uint8_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<uint32_t>(value);
                }
                case gltf::COMPONENT_TYPE_UNSIGNED_SHORT:
                {
                    uint16_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return static_cast<uint32_t>(value);
                }
                case gltf::COMPONENT_TYPE_UNSIGNED_INT:
                {
                    uint32_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    return value;
                }
                default:
                    return 0;
            }
        }
    }

    std::expected<MeshData, SString> LoadGltfMeshFromFile(STextView filePath, size_t meshIndex, IModelLoader::ProgressCallback progressCallback)
    {
        gltfLoader loader;
        if (progressCallback)
        {
            loader.setProgressCallback(std::move(progressCallback));
        }

        if (filePath.empty())
        {
            return std::unexpected(SString::from_utf8("文件路径为空"));
        }
        if (!loader.loadFromFile(filePath.data()))
        {
            return std::unexpected(SString::from_utf8(fmt::format("glTF 文件加载失败: {}", filePath.sv())));
        }
        return loader.extractMeshDataByIndex(meshIndex);
    }

    std::expected<MeshData, SString> LoadGltfMeshFromMemory(const void* data, size_t size, size_t meshIndex, IModelLoader::ProgressCallback progressCallback)
    {
        if (!data || size == 0)
        {
            return std::unexpected(SString::from_utf8("内存数据为空"));
        }

        gltfLoader loader;
        if (progressCallback)
        {
            loader.setProgressCallback(std::move(progressCallback));
        }
        if (!loader.loadFromMemory(data, size))
        {
            return std::unexpected(SString::from_utf8("glTF 内存数据加载失败"));
        }
        return loader.extractMeshDataByIndex(meshIndex);
    }

    bool gltfLoader::loadImagesForModel()
    {
        for (auto& image : _model.images)
        {
            // Already decoded (has dimensions and pixel data)
            if (!image.image.empty() && image.width > 0 && image.height > 0)
            {
                continue;
            }

            if (!image.uri.empty())
            {
                // URI image: load and decode from file path
                SString imagePath = image.uri;
                if (!_basePath.empty() && !util::is_absolute_path(imagePath.view()))
                {
                    imagePath = util::join_path(_basePath.view(), image.uri.view());
                }

                auto imageResult = util::load_image(imagePath.sv(), 4);
                if (!imageResult.has_value())
                {
                    setError(EAssetLoaderError::PARSE_ERROR, imageResult.error());
                    return false;
                }

                auto& decoded = imageResult.value();
                image.width     = decoded.width;
                image.height    = decoded.height;
                image.component = decoded.channels;
                image.bits      = 8;
                image.pixel_type = static_cast<int>(gltf::COMPONENT_TYPE_UNSIGNED_BYTE);
                image.image.resize(decoded.data.size());
                if (!decoded.data.empty())
                {
                    std::memcpy(image.image.data(), decoded.data.data(), decoded.data.size());
                }
            }
            else if (image.bufferView >= 0 && image.bufferView < static_cast<int>(_model.bufferViews.size()))
            {
                // bufferView image: decode from buffer data already loaded by ShineGltf
                const auto& bv = _model.bufferViews[static_cast<size_t>(image.bufferView)];
                if (bv.buffer < 0 || bv.buffer >= static_cast<int>(_model.buffers.size()))
                {
                    continue;
                }

                const auto& bufData = _model.buffers[static_cast<size_t>(bv.buffer)].data;
                if (bv.byteOffset + bv.byteLength > bufData.size())
                {
                    continue;
                }

                const auto rawSpan = std::as_bytes(
                    std::span<const unsigned char>(bufData).subspan(bv.byteOffset, bv.byteLength));
                auto decoded = util::load_image_from_memory(rawSpan, 4);
                if (!decoded.has_value())
                {
                    setError(EAssetLoaderError::PARSE_ERROR, decoded.error());
                    return false;
                }

                image.width     = decoded->width;
                image.height    = decoded->height;
                image.component = decoded->channels;
                image.bits      = 8;
                image.pixel_type = static_cast<int>(gltf::COMPONENT_TYPE_UNSIGNED_BYTE);
                image.image.resize(decoded->data.size());
                if (!decoded->data.empty())
                {
                    std::memcpy(image.image.data(), decoded->data.data(), decoded->data.size());
                }
            }
        }

        return true;
    }

    bool gltfLoader::loadFromMemory(const void* data, size_t size)
    {
        ensure_gltf_loader_log_categories();
        util::FunctionTimer timer("gltfLoader::loadFromMemory");
        setState(EAssetLoadState::READING_FILE);
        notifyProgress(0.0f, "开始加载");

        if (!data || size == 0)
        {
            setError(EAssetLoaderError::INVALID_PARAMETER);
            return false;
        }

        if (size > static_cast<size_t>(std::numeric_limits<unsigned int>::max()))
        {
            setError(EAssetLoaderError::INVALID_PARAMETER);
            return false;
        }

        unload();
        isLoader = false;
        _basePath.clear();

        setState(EAssetLoadState::PARSING_DATA);
        notifyProgress(0.25f, "解析 glTF");

        gltf::ShineGltf gltfLoader;
        const auto dataSpan = std::span<const std::byte>{static_cast<const std::byte*>(data), size};
        auto parseResult = gltfLoader.LoadFromMemory(dataSpan, STextView{_basePath});
        if (!parseResult)
        {
            setError(EAssetLoaderError::PARSE_ERROR, parseResult.error().c_str());
            setState(EAssetLoadState::FAILD);
            return false;
        }
        _model = gltfLoader.TakeModel();

        if (!_model.extensionsUsed.empty())
        {
            SString extensionList;
            for (size_t i = 0; i < _model.extensionsUsed.size(); ++i)
            {
                if (i > 0) extensionList += ",";
                extensionList += STextView(_model.extensionsUsed[i]);
            }
            SHINE_LOG_INFO(GltfLoaderLog, "parse", "extensionsUsed={}", extensionList.sv());
        }

        notifyProgress(0.65f, "加载图片资源");
        if (!loadImagesForModel())
        {
            setState(EAssetLoadState::FAILD);
            return false;
        }

        setState(EAssetLoadState::COMPLETE);
        isLoader = true;
        notifyProgress(1.0f, "加载完成");
        return true;
    }

    bool gltfLoader::loadFromFile(const char* filePath)
    {
        ensure_gltf_loader_log_categories();
        setState(EAssetLoadState::READING_FILE);
        notifyProgress(0.0f, "读取文件");

        if (!filePath)
        {
            setError(EAssetLoaderError::INVALID_PARAMETER);
            return false;
        }

        unload();
        isLoader = false;
        _basePath = util::get_directory(STextView(filePath));

        notifyProgress(0.2f, "解析文件数据");

        gltf::ShineGltf gltfLoader;
        auto parseResult = gltfLoader.LoadFromFile(STextView(filePath));
        if (!parseResult)
        {
            setError(EAssetLoaderError::FILE_NOT_FOUND, parseResult.error().c_str());
            setState(EAssetLoadState::FAILD);
            return false;
        }
        _model = gltfLoader.TakeModel();

        if (!_model.extensionsUsed.empty())
        {
            SString extensionList;
            for (size_t i = 0; i < _model.extensionsUsed.size(); ++i)
            {
                if (i > 0) extensionList += ",";
                extensionList += STextView(_model.extensionsUsed[i]);
            }
            SHINE_LOG_INFO(GltfLoaderLog, "parse", "extensionsUsed={}", extensionList.sv());
        }

        notifyProgress(0.65f, "加载图片资源");
        if (!loadImagesForModel())
        {
            setState(EAssetLoadState::FAILD);
            return false;
        }

        setState(EAssetLoadState::COMPLETE);
        isLoader = true;
        notifyProgress(1.0f, "加载完成");
        return true;
    }

    void gltfLoader::unload()
    {
        _model = gltf::GltfModel{};
        isLoader = false;
        _basePath.clear();
        setState(EAssetLoadState::NONE);
    }

    bool gltfLoader::appendPrimitiveMeshData(std::vector<MeshData>& meshes, const gltf::GltfPrimitive& primitive, const gltf::GltfNode& node, STextView meshName) const
    {
        const STextView meshNameView(meshName);
        auto positionIt = primitive.attributes.find("POSITION");
        if (positionIt == primitive.attributes.end())
        {
            SHINE_LOG_WARN(GltfLoaderLog, "extract", "skip primitive, missing POSITION, node='{}' mesh='{}'", node.name, meshNameView.sv());
            return false;
        }

        AccessorView positionView;
        if (!get_accessor_view(_model, positionIt->second, positionView) || positionView.type != gltf::TYPE_VEC3)
        {
            SHINE_LOG_WARN(GltfLoaderLog, "extract", "skip primitive, invalid POSITION accessor, node='{}' mesh='{}' accessor={}", node.name, meshNameView.sv(), positionIt->second);
            return false;
        }

        MeshData meshData;
        meshData.name = meshNameView;
        meshData.materialIndex = primitive.material;
        meshData.vertices.reserve(positionView.count);

        if (node.translation.size() == 3)
        {
            meshData.translation = FVector3f(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]), static_cast<float>(node.translation[2]));
        }
        if (node.scale.size() == 3)
        {
            meshData.scale = FVector3f(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2]));
        }
        if (node.rotation.size() == 4)
        {
            const FQuatf q(
                static_cast<float>(node.rotation[3]),
                static_cast<float>(node.rotation[0]),
                static_cast<float>(node.rotation[1]),
                static_cast<float>(node.rotation[2]));
            meshData.rotation = q.toRotatorDegrees();
        }

        const int positionComponentSize = gltf::GetComponentSizeInBytes(static_cast<u32>(positionView.componentType));
        for (size_t i = 0; i < positionView.count; ++i)
        {
            const unsigned char* p = positionView.data + i * positionView.stride;
            const float x = read_float_component(p + 0 * positionComponentSize, positionView.componentType, positionView.normalized);
            const float y = read_float_component(p + 1 * positionComponentSize, positionView.componentType, positionView.normalized);
            const float z = read_float_component(p + 2 * positionComponentSize, positionView.componentType, positionView.normalized);
            meshData.vertices.emplace_back(x, y, z);
        }

        auto normalIt = primitive.attributes.find("NORMAL");
        if (normalIt != primitive.attributes.end())
        {
            AccessorView normalView;
            if (get_accessor_view(_model, normalIt->second, normalView) && normalView.type == gltf::TYPE_VEC3 && normalView.count == positionView.count)
            {
                meshData.normals.reserve(normalView.count);
                const int normalComponentSize = gltf::GetComponentSizeInBytes(static_cast<u32>(normalView.componentType));
                for (size_t i = 0; i < normalView.count; ++i)
                {
                    const unsigned char* p = normalView.data + i * normalView.stride;
                    const float x = read_float_component(p + 0 * normalComponentSize, normalView.componentType, normalView.normalized);
                    const float y = read_float_component(p + 1 * normalComponentSize, normalView.componentType, normalView.normalized);
                    const float z = read_float_component(p + 2 * normalComponentSize, normalView.componentType, normalView.normalized);
                    meshData.normals.emplace_back(x, y, z);
                }
            }
        }
        if (meshData.normals.size() != meshData.vertices.size())
        {
            meshData.normals.assign(meshData.vertices.size(), FVector3f(0.0f, 0.0f, 1.0f));
        }

        auto texcoordIt = primitive.attributes.find("TEXCOORD_0");
        if (texcoordIt != primitive.attributes.end())
        {
            AccessorView texcoordView;
            if (get_accessor_view(_model, texcoordIt->second, texcoordView) && texcoordView.type == gltf::TYPE_VEC2 && texcoordView.count == positionView.count)
            {
                meshData.texcoords.reserve(texcoordView.count);
                const int texComponentSize = gltf::GetComponentSizeInBytes(static_cast<u32>(texcoordView.componentType));
                for (size_t i = 0; i < texcoordView.count; ++i)
                {
                    const unsigned char* p = texcoordView.data + i * texcoordView.stride;
                    const float u = read_float_component(p + 0 * texComponentSize, texcoordView.componentType, texcoordView.normalized);
                    const float v = read_float_component(p + 1 * texComponentSize, texcoordView.componentType, texcoordView.normalized);
                    meshData.texcoords.emplace_back(u, v);
                }
            }
        }
        if (meshData.texcoords.size() != meshData.vertices.size())
        {
            meshData.texcoords.assign(meshData.vertices.size(), FVector2f(0.0f, 0.0f));
        }

        auto colorIt = primitive.attributes.find("COLOR_0");
        if (colorIt != primitive.attributes.end())
        {
            AccessorView colorView;
            if (get_accessor_view(_model, colorIt->second, colorView) &&
                (colorView.type == gltf::TYPE_VEC3 || colorView.type == gltf::TYPE_VEC4) &&
                colorView.count == positionView.count)
            {
                meshData.colors.reserve(colorView.count);
                const int colorComponentSize = gltf::GetComponentSizeInBytes(static_cast<u32>(colorView.componentType));
                const int colorComponents    = gltf::GetNumComponentsInType(static_cast<u32>(colorView.type));
                for (size_t i = 0; i < colorView.count; ++i)
                {
                    const unsigned char* p = colorView.data + i * colorView.stride;
                    const float r = read_float_component(p + 0 * colorComponentSize, colorView.componentType, colorView.normalized);
                    const float g = read_float_component(p + 1 * colorComponentSize, colorView.componentType, colorView.normalized);
                    const float b = read_float_component(p + 2 * colorComponentSize, colorView.componentType, colorView.normalized);
                    float a = 1.0f;
                    if (colorComponents == 4)
                    {
                        a = read_float_component(p + 3 * colorComponentSize, colorView.componentType, colorView.normalized);
                    }
                    meshData.colors.emplace_back(r, g, b, a);
                }
            }
        }
        if (meshData.colors.size() != meshData.vertices.size())
        {
            meshData.colors.assign(meshData.vertices.size(), VertexColor(1.0f, 1.0f, 1.0f, 1.0f));
        }

        if (primitive.indices >= 0)
        {
            AccessorView indexView;
            if (!get_accessor_view(_model, primitive.indices, indexView) || indexView.type != gltf::TYPE_SCALAR)
            {
                SHINE_LOG_WARN(GltfLoaderLog, "extract", "skip primitive, invalid index accessor, node='{}' mesh='{}' accessor={}", node.name, meshNameView.sv(), primitive.indices);
                return false;
            }

            meshData.indices.reserve(indexView.count);
            for (size_t i = 0; i < indexView.count; ++i)
            {
                const unsigned char* p = indexView.data + i * indexView.stride;
                meshData.indices.push_back(read_index_component(p, indexView.componentType));
            }
        }
        else
        {
            meshData.indices.reserve(meshData.vertices.size());
            for (uint32_t i = 0; i < static_cast<uint32_t>(meshData.vertices.size()); ++i)
            {
                meshData.indices.push_back(i);
            }
        }

        meshes.push_back(std::move(meshData));
        return true;
    }

    std::vector<MeshData> gltfLoader::extractMeshData() const
    {
        ensure_gltf_loader_log_categories();
        std::vector<MeshData> result;
        if (!isLoader || _model.scenes.empty() || _model.nodes.empty() || _model.meshes.empty())
        {
            SHINE_LOG_WARN(
                GltfLoaderLog,
                "extract",
                "extract empty, loaded={} scenes={} nodes={} meshes={}",
                isLoader,
                _model.scenes.size(),
                _model.nodes.size(),
                _model.meshes.size());
            return result;
        }

        int sceneIndex = _model.defaultScene;
        if (sceneIndex < 0 || sceneIndex >= static_cast<int>(_model.scenes.size()))
        {
            sceneIndex = 0;
        }
        SHINE_LOG_DEBUG(
            GltfLoaderLog,
            "extract",
            "extract begin, sceneIndex={} sceneRoots={} nodes={} meshes={}",
            sceneIndex,
            _model.scenes[static_cast<size_t>(sceneIndex)].nodes.size(),
            _model.nodes.size(),
            _model.meshes.size());

        const auto& scene = _model.scenes[static_cast<size_t>(sceneIndex)];
        std::function<void(int)> processNode = [&](int nodeIndex)
        {
            if (nodeIndex < 0 || nodeIndex >= static_cast<int>(_model.nodes.size()))
            {
                return;
            }

            const auto& node = _model.nodes[static_cast<size_t>(nodeIndex)];
            if (node.mesh >= 0 && node.mesh < static_cast<int>(_model.meshes.size()))
            {
                const auto& mesh = _model.meshes[static_cast<size_t>(node.mesh)];
                std::string meshName = mesh.name.empty() ? node.name : mesh.name;
                if (meshName.empty())
                {
                    meshName = fmt::format("mesh_{}", node.mesh);
                }

                for (size_t primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex)
                {
                    const auto& primitive = mesh.primitives[primitiveIndex];
                    if (primitive.mode != -1 && primitive.mode != 4) // 4 = TRIANGLES
                    {
                        SHINE_LOG_WARN(
                            GltfLoaderLog,
                            "extract",
                            "skip primitive, non-triangle mode={}, node='{}' mesh='{}' primitiveIndex={}",
                            primitive.mode,
                            node.name,
                            meshName,
                            primitiveIndex);
                        continue;
                    }

                    std::string primitiveName = meshName;
                    if (mesh.primitives.size() > 1)
                    {
                        primitiveName = fmt::format("{}_{}", meshName, primitiveIndex);
                    }
                    appendPrimitiveMeshData(result, primitive, node, STextView(primitiveName));
                }
            }

            for (int child : node.children)
            {
                processNode(child);
            }
        };

        for (int rootNode : scene.nodes)
        {
            processNode(rootNode);
        }
        SHINE_LOG_DEBUG(GltfLoaderLog, "extract", "extract end, renderableMeshes={}", result.size());

        return result;
    }

    std::expected<MeshData, SString> gltfLoader::extractMeshDataByIndex(size_t meshIndex) const
    {
        auto meshes = extractMeshData();
        if (meshes.empty())
        {
            return std::unexpected(SString::from_utf8("模型未包含可提取网格"));
        }
        if (meshIndex >= meshes.size())
        {
            return std::unexpected(SString::from_utf8(fmt::format("网格索引越界: {} / {}", meshIndex, meshes.size())));
        }
        return meshes[meshIndex];
    }

    size_t gltfLoader::getMeshCount() const noexcept
    {
        if (!isLoader)
        {
            return 0;
        }
        return extractMeshData().size();
    }

    std::vector<int> gltfLoader::getSceneRootNodes(int sceneIndex) const
    {
        if (!isLoader || sceneIndex < 0 || sceneIndex >= static_cast<int>(_model.scenes.size()))
        {
            return {};
        }
        return _model.scenes[static_cast<size_t>(sceneIndex)].nodes;
    }
}
