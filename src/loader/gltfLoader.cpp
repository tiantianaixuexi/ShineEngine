#include "gltfLoader.h"

#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <sstream>
#include <charconv>

#include "fmt/format.h"

#include "math/vector.h"
#include "math/vector2.h"
#include "math/rotator.h"
#include "math/quat.h"

#include "util/timer/function_timer.h"
#include "util/file_util.h"


namespace shine::loader
{
    using namespace shine::math;

    namespace {
        // GLB 文件格式常量
        constexpr uint32_t GLB_MAGIC = 0x46546C67;  // "glTF"
        constexpr uint32_t GLB_CHUNK_TYPE_JSON = 0x4E4F534A;  // "JSON"
        constexpr uint32_t GLB_CHUNK_TYPE_BIN = 0x004E4942;   // "BIN\0"

        // glTF 组件类型
        constexpr int COMPONENT_TYPE_BYTE = 5120;
        constexpr int COMPONENT_TYPE_UNSIGNED_BYTE = 5121;
        constexpr int COMPONENT_TYPE_SHORT = 5122;
        constexpr int COMPONENT_TYPE_UNSIGNED_SHORT = 5123;
        constexpr int COMPONENT_TYPE_UNSIGNED_INT = 5125;
        constexpr int COMPONENT_TYPE_FLOAT = 5126;

        // 字节序转换（小端）
        template<typename T>
        T readLittleEndian(const std::byte* data) {
            T value = 0;
            std::memcpy(&value, data, sizeof(T));
            return value;
        }

        // 简单的 JSON 解析辅助函数
        void skipWhitespace(const char*& ptr, const char* end) {
            while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) {
                ++ptr;
            }
        }

        bool parseString(const char*& ptr, const char* end, std::string& out) {
            skipWhitespace(ptr, end);
            if (ptr >= end || *ptr != '"') return false;
            ++ptr;
            
            out.clear();
            while (ptr < end && *ptr != '"') {
                if (*ptr == '\\') {
                    ++ptr;
                    if (ptr >= end) return false;
                    switch (*ptr) {
                        case '"': out += '"'; break;
                        case '\\': out += '\\'; break;
                        case '/': out += '/'; break;
                        case 'b': out += '\b'; break;
                        case 'f': out += '\f'; break;
                        case 'n': out += '\n'; break;
                        case 'r': out += '\r'; break;
                        case 't': out += '\t'; break;
                        default: out += *ptr; break;
                    }
                } else {
                    out += *ptr;
                }
                ++ptr;
            }
            if (ptr >= end || *ptr != '"') return false;
            ++ptr;
            return true;
        }

        bool parseNumber(const char*& ptr, const char* end, double& out) {
            skipWhitespace(ptr, end);
            const char* start = ptr;
            bool hasDot = false;
            bool hasExp = false;
            
            if (ptr < end && (*ptr == '-' || *ptr == '+')) ++ptr;
            if (ptr >= end || (*ptr < '0' || *ptr > '9')) return false;
            
            while (ptr < end && ((*ptr >= '0' && *ptr <= '9') || *ptr == '.' || *ptr == 'e' || *ptr == 'E' || *ptr == '-' || *ptr == '+')) {
                if (*ptr == '.') {
                    if (hasDot) break;
                    hasDot = true;
                } else if (*ptr == 'e' || *ptr == 'E') {
                    if (hasExp) break;
                    hasExp = true;
                }
                ++ptr;
            }
            
            auto [p, ec] = std::from_chars(start, ptr, out);
            return ec == std::errc();
        }

        bool parseInteger(const char*& ptr, const char* end, int64_t& out) {
            skipWhitespace(ptr, end);
            const char* start = ptr;
            bool negative = false;
            
            if (ptr < end && *ptr == '-') {
                negative = true;
                ++ptr;
            }
            if (ptr >= end || (*ptr < '0' || *ptr > '9')) return false;
            
            while (ptr < end && *ptr >= '0' && *ptr <= '9') {
                ++ptr;
            }
            
            int64_t value = 0;
            auto [p, ec] = std::from_chars(start + (negative ? 1 : 0), ptr, value);
            if (ec == std::errc()) {
                out = negative ? -value : value;
                return true;
            }
            return false;
        }

        bool parseBoolean(const char*& ptr, const char* end, bool& out) {
            skipWhitespace(ptr, end);
            if (ptr + 4 <= end && std::strncmp(ptr, "true", 4) == 0) {
                ptr += 4;
                out = true;
                return true;
            }
            if (ptr + 5 <= end && std::strncmp(ptr, "false", 5) == 0) {
                ptr += 5;
                out = false;
                return true;
            }
            return false;
        }

        bool parseNull(const char*& ptr, const char* end) {
            skipWhitespace(ptr, end);
            if (ptr + 4 <= end && std::strncmp(ptr, "null", 4) == 0) {
                ptr += 4;
                return true;
            }
            return false;
        }

        // 简单的 JSON 值类型
        struct JsonValue {
            enum Type { Null, Bool, Number, String, Array, Object };
            Type type = Null;
            bool boolValue = false;
            double numberValue = 0.0;
            std::string stringValue;
            std::vector<JsonValue> arrayValue;
            std::unordered_map<std::string, JsonValue> objectValue;

            bool isNull() const { return type == Null; }
            bool isBool() const { return type == Bool; }
            bool isNumber() const { return type == Number; }
            bool isString() const { return type == String; }
            bool isArray() const { return type == Array; }
            bool isObject() const { return type == Object; }

            bool getBool() const { return boolValue; }
            double getNumber() const { return numberValue; }
            const std::string& getString() const { return stringValue; }
            const std::vector<JsonValue>& getArray() const { return arrayValue; }
            const std::unordered_map<std::string, JsonValue>& getObject() const { return objectValue; }

            bool has(const std::string& key) const {
                return isObject() && objectValue.count(key) > 0;
            }

            const JsonValue& operator[](const std::string& key) const {
                static JsonValue nullValue;
                if (isObject()) {
                    auto it = objectValue.find(key);
                    if (it != objectValue.end()) return it->second;
                }
                return nullValue;
            }

            const JsonValue& operator[](size_t index) const {
                static JsonValue nullValue;
                if (isArray() && index < arrayValue.size()) {
                    return arrayValue[index];
                }
                return nullValue;
            }

            size_t size() const {
                if (isArray()) return arrayValue.size();
                if (isObject()) return objectValue.size();
                return 0;
            }
        };

        bool parseJsonValue(const char*& ptr, const char* end, JsonValue& value);

        bool parseJsonArray(const char*& ptr, const char* end, JsonValue& value) {
            skipWhitespace(ptr, end);
            if (ptr >= end || *ptr != '[') return false;
            ++ptr;

            value.type = JsonValue::Array;
            value.arrayValue.clear();

            skipWhitespace(ptr, end);
            if (ptr < end && *ptr == ']') {
                ++ptr;
                return true;
            }

            while (ptr < end) {
                JsonValue item;
                if (!parseJsonValue(ptr, end, item)) return false;
                value.arrayValue.push_back(std::move(item));

                skipWhitespace(ptr, end);
                if (ptr >= end) return false;
                if (*ptr == ']') {
                    ++ptr;
                    return true;
                }
                if (*ptr != ',') return false;
                ++ptr;
            }
            return false;
        }

        bool parseJsonObject(const char*& ptr, const char* end, JsonValue& value) {
            skipWhitespace(ptr, end);
            if (ptr >= end || *ptr != '{') return false;
            ++ptr;

            value.type = JsonValue::Object;
            value.objectValue.clear();

            skipWhitespace(ptr, end);
            if (ptr < end && *ptr == '}') {
                ++ptr;
                return true;
            }

            while (ptr < end) {
                std::string key;
                if (!parseString(ptr, end, key)) return false;

                skipWhitespace(ptr, end);
                if (ptr >= end || *ptr != ':') return false;
                ++ptr;

                JsonValue item;
                if (!parseJsonValue(ptr, end, item)) return false;
                value.objectValue[std::move(key)] = std::move(item);

                skipWhitespace(ptr, end);
                if (ptr >= end) return false;
                if (*ptr == '}') {
                    ++ptr;
                    return true;
                }
                if (*ptr != ',') return false;
                ++ptr;
            }
            return false;
        }

        bool parseJsonValue(const char*& ptr, const char* end, JsonValue& value) {
            skipWhitespace(ptr, end);
            if (ptr >= end) return false;

            if (*ptr == '"') {
                value.type = JsonValue::String;
                return parseString(ptr, end, value.stringValue);
            } else if (*ptr == '[') {
                return parseJsonArray(ptr, end, value);
            } else if (*ptr == '{') {
                return parseJsonObject(ptr, end, value);
            } else if (*ptr == 't' || *ptr == 'f') {
                value.type = JsonValue::Bool;
                return parseBoolean(ptr, end, value.boolValue);
            } else if (*ptr == 'n') {
                value.type = JsonValue::Null;
                return parseNull(ptr, end);
            } else if ((*ptr >= '0' && *ptr <= '9') || *ptr == '-' || *ptr == '+') {
                value.type = JsonValue::Number;
                return parseNumber(ptr, end, value.numberValue);
            }
            return false;
        }
    }

    bool gltfLoader::loadFromMemory(const void *data, size_t size)
    {
        util::FunctionTimer timer("gltfLoader::loadFromMemory");
        setState(EAssetLoadState::READING_FILE);

        if (!data || size == 0) {
            setError(EAssetLoaderError::INVALID_PARAMETER);
        return false;
        }

        unload();
        _loaded = false;

        setState(EAssetLoadState::PARSING_DATA);
        bool result = parseGLB(data, size);

        if (result) {
            _loaded = true;
            setState(EAssetLoadState::COMPLETE);
        } else {
            setState(EAssetLoadState::ERROR);
        }

        return result;
    }

    bool gltfLoader::loadFromFile(const char *filePath)
    {
        util::FunctionTimer timer("gltfLoader::loadFromFile");
        setState(EAssetLoadState::READING_FILE);

        if (!filePath) {
            setError(EAssetLoaderError::INVALID_PARAMETER);
            return false;
        }

        // 使用文件映射读取文件
        auto fileResult = util::read_full_file(filePath);
        if (!fileResult.has_value()) {
            setError(EAssetLoaderError::FILE_NOT_FOUND);
        return false;
        }

        auto fileMapView = std::move(fileResult.value());
        const void* data = fileMapView.view.data();
        size_t size = fileMapView.view.size();

        unload();
        _loaded = false;

        setState(EAssetLoadState::PARSING_DATA);
        bool result = parseGLB(data, size);

        if (result) {
            _loaded = true;
            setState(EAssetLoadState::COMPLETE);
        } else {
            setState(EAssetLoadState::ERROR);
        }

        return result;
    }
    
    void gltfLoader::unload()
    {
        _model = GltfModel();
        _binaryData.clear();
        _loaded = false;
        setState(EAssetLoadState::NONE);
    }

    bool gltfLoader::parseGLB(const void* data, size_t size)
    {
        if (size < 12) {
            setError(EAssetLoaderError::INVALID_FORMAT, "File too small");
            return false;
        }

        const std::byte* bytes = static_cast<const std::byte*>(data);

        // 读取 GLB 头部
        GLBHeader header;
        header.magic = readLittleEndian<uint32_t>(bytes);
        header.version = readLittleEndian<uint32_t>(bytes + 4);
        header.length = readLittleEndian<uint32_t>(bytes + 8);

        // 验证 magic number
        if (header.magic != GLB_MAGIC) {
            setError(EAssetLoaderError::INVALID_FORMAT);
            return false;
        }

        // 验证版本（支持版本 2）
        if (header.version != 2) {
            setError(EAssetLoaderError::VERSION_MISMATCH);
            return false;
        }

        // 验证文件长度
        if (header.length != size) {
            setError(EAssetLoaderError::CORRUPTION_DETECTED);
            return false;
        }

        size_t offset = 12;

        // 读取第一个 chunk (应该是 JSON)
        if (offset + 8 > size) {
            setError(EAssetLoaderError::INVALID_FORMAT);
            return false;
        }

        GLBChunk jsonChunk;
        jsonChunk.length = readLittleEndian<uint32_t>(bytes + offset);
        jsonChunk.type = readLittleEndian<uint32_t>(bytes + offset + 4);

        if (jsonChunk.type != GLB_CHUNK_TYPE_JSON) {
            setError(EAssetLoaderError::INVALID_FORMAT);
            return false;
        }

        offset += 8;
        if (offset + jsonChunk.length > size) {
            setError(EAssetLoaderError::CORRUPTION_DETECTED);
            return false;
        }

        jsonChunk.data.assign(bytes + offset, bytes + offset + jsonChunk.length);
        offset += jsonChunk.length;

        // 解析 JSON chunk
        const char* jsonData = reinterpret_cast<const char*>(jsonChunk.data.data());
        if (!parseJSONChunk(jsonData, jsonChunk.length)) {
            return false;
        }

        // 读取第二个 chunk (应该是 BIN)
        if (offset + 8 <= size) {
            GLBChunk binChunk;
            binChunk.length = readLittleEndian<uint32_t>(bytes + offset);
            binChunk.type = readLittleEndian<uint32_t>(bytes + offset + 4);

            if (binChunk.type == GLB_CHUNK_TYPE_BIN) {
                offset += 8;
                if (offset + binChunk.length <= size) {
                    binChunk.data.assign(bytes + offset, bytes + offset + binChunk.length);
                    if (!parseBinaryChunk(binChunk.data.data(), binChunk.length)) {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool gltfLoader::parseJSONChunk(const char* jsonData, size_t jsonSize)
    {
        const char* ptr = jsonData;
        const char* end = jsonData + jsonSize;

        JsonValue root;
        if (!parseJsonObject(ptr, end, root)) {
            setError(EAssetLoaderError::PARSE_ERROR);
            return false;
        }

        // 解析 asset.version
        if (root.has("asset") && root["asset"].isObject()) {
            const auto& asset = root["asset"];
            if (asset.has("version") && asset["version"].isString()) {
                _model.version = asset["version"].getString();
            }
        }

        // 解析 buffers
        if (root.has("buffers") && root["buffers"].isArray()) {
            const auto& buffers = root["buffers"].getArray();
            _model.buffers.reserve(buffers.size());
            for (const auto& buf : buffers) {
                Buffer buffer;
                if (buf.isObject()) {
                    if (buf.has("byteLength") && buf["byteLength"].isNumber()) {
                        buffer.byteLength = static_cast<size_t>(buf["byteLength"].getNumber());
                    }
                    if (buf.has("uri") && buf["uri"].isString()) {
                        buffer.uri = buf["uri"].getString();
                    }
                }
                _model.buffers.push_back(std::move(buffer));
            }
        }

        // 解析 bufferViews
        if (root.has("bufferViews") && root["bufferViews"].isArray()) {
            const auto& bufferViews = root["bufferViews"].getArray();
            _model.bufferViews.reserve(bufferViews.size());
            for (const auto& bv : bufferViews) {
                BufferView bufferView;
                if (bv.isObject()) {
                    if (bv.has("buffer") && bv["buffer"].isNumber()) {
                        bufferView.buffer = static_cast<int>(bv["buffer"].getNumber());
                    }
                    if (bv.has("byteOffset") && bv["byteOffset"].isNumber()) {
                        bufferView.byteOffset = static_cast<size_t>(bv["byteOffset"].getNumber());
                    }
                    if (bv.has("byteLength") && bv["byteLength"].isNumber()) {
                        bufferView.byteLength = static_cast<size_t>(bv["byteLength"].getNumber());
                    }
                    if (bv.has("byteStride") && bv["byteStride"].isNumber()) {
                        bufferView.byteStride = static_cast<int>(bv["byteStride"].getNumber());
                    }
                    if (bv.has("target") && bv["target"].isNumber()) {
                        bufferView.target = static_cast<int>(bv["target"].getNumber());
                    }
                }
                _model.bufferViews.push_back(std::move(bufferView));
            }
        }

        // 解析 accessors
        if (root.has("accessors") && root["accessors"].isArray()) {
            const auto& accessors = root["accessors"].getArray();
            _model.accessors.reserve(accessors.size());
            for (const auto& acc : accessors) {
                Accessor accessor;
                if (acc.isObject()) {
                    if (acc.has("bufferView") && acc["bufferView"].isNumber()) {
                        accessor.bufferView = static_cast<int>(acc["bufferView"].getNumber());
                    }
                    if (acc.has("byteOffset") && acc["byteOffset"].isNumber()) {
                        accessor.byteOffset = static_cast<size_t>(acc["byteOffset"].getNumber());
                    }
                    if (acc.has("componentType") && acc["componentType"].isNumber()) {
                        accessor.componentType = static_cast<int>(acc["componentType"].getNumber());
                    }
                    if (acc.has("normalized") && acc["normalized"].isBool()) {
                        accessor.normalized = acc["normalized"].getBool();
                    }
                    if (acc.has("count") && acc["count"].isNumber()) {
                        accessor.count = static_cast<size_t>(acc["count"].getNumber());
                    }
                    if (acc.has("type") && acc["type"].isString()) {
                        accessor.type = acc["type"].getString();
                    }
                }
                _model.accessors.push_back(std::move(accessor));
            }
        }

        // 解析 meshes
        if (root.has("meshes") && root["meshes"].isArray()) {
            const auto& meshes = root["meshes"].getArray();
            _model.meshes.reserve(meshes.size());
            for (const auto& mesh : meshes) {
                Mesh m;
                if (mesh.isObject()) {
                    if (mesh.has("name") && mesh["name"].isString()) {
                        m.name = mesh["name"].getString();
                    }
                    if (mesh.has("primitives") && mesh["primitives"].isArray()) {
                        const auto& primitives = mesh["primitives"].getArray();
                        m.primitives.reserve(primitives.size());
                        for (const auto& prim : primitives) {
                            Primitive primitive;
                            if (prim.isObject()) {
                                if (prim.has("attributes") && prim["attributes"].isObject()) {
                                    const auto& attrs = prim["attributes"].getObject();
                                    for (const auto& [key, value] : attrs) {
                                        if (value.isNumber()) {
                                            primitive.attributes[key] = static_cast<int>(value.getNumber());
                                        }
                                    }
                                }
                                if (prim.has("indices") && prim["indices"].isNumber()) {
                                    primitive.indices = static_cast<int>(prim["indices"].getNumber());
                                }
                                if (prim.has("material") && prim["material"].isNumber()) {
                                    primitive.material = static_cast<int>(prim["material"].getNumber());
                                }
                                if (prim.has("mode") && prim["mode"].isNumber()) {
                                    primitive.mode = static_cast<int>(prim["mode"].getNumber());
                                }
                            }
                            m.primitives.push_back(std::move(primitive));
                        }
                    }
                }
                _model.meshes.push_back(std::move(m));
            }
        }

        // 解析 nodes
        if (root.has("nodes") && root["nodes"].isArray()) {
            const auto& nodes = root["nodes"].getArray();
            _model.nodes.reserve(nodes.size());
            for (const auto& node : nodes) {
                Node n;
                if (node.isObject()) {
                    if (node.has("name") && node["name"].isString()) {
                        n.name = node["name"].getString();
                    }
                    if (node.has("mesh") && node["mesh"].isNumber()) {
                        n.mesh = static_cast<int>(node["mesh"].getNumber());
                    }
                    if (node.has("translation") && node["translation"].isArray()) {
                        const auto& trans = node["translation"].getArray();
                        for (const auto& val : trans) {
                            if (val.isNumber()) {
                                n.translation.push_back(static_cast<float>(val.getNumber()));
                            }
                        }
                    }
                    if (node.has("rotation") && node["rotation"].isArray()) {
                        const auto& rot = node["rotation"].getArray();
                        for (const auto& val : rot) {
                            if (val.isNumber()) {
                                n.rotation.push_back(static_cast<float>(val.getNumber()));
                            }
                        }
                    }
                    if (node.has("scale") && node["scale"].isArray()) {
                        const auto& scale = node["scale"].getArray();
                        for (const auto& val : scale) {
                            if (val.isNumber()) {
                                n.scale.push_back(static_cast<float>(val.getNumber()));
                            }
                        }
                    }
                    if (node.has("children") && node["children"].isArray()) {
                        const auto& children = node["children"].getArray();
                        for (const auto& val : children) {
                            if (val.isNumber()) {
                                n.children.push_back(static_cast<int>(val.getNumber()));
                            }
                        }
                    }
                }
                _model.nodes.push_back(std::move(n));
            }
        }

        fmt::println("glTF model loaded: version={}, buffers={}, bufferViews={}, accessors={}, meshes={}, nodes={}",
                     _model.version, _model.buffers.size(), _model.bufferViews.size(),
                     _model.accessors.size(), _model.meshes.size(), _model.nodes.size());

        return true;
    }

    bool gltfLoader::parseBinaryChunk(const std::byte* binData, size_t binSize)
    {
        _binaryData.assign(binData, binData + binSize);
        fmt::println("Binary chunk loaded: {} bytes", binSize);
        return true;
    }

    std::vector<std::byte> gltfLoader::getAccessorData(const Accessor& accessor) const
    {
        std::vector<std::byte> result;

        if (accessor.bufferView < 0 || 
            static_cast<size_t>(accessor.bufferView) >= _model.bufferViews.size()) {
            return result;
        }

        const BufferView& bufferView = _model.bufferViews[accessor.bufferView];
        
        if (bufferView.buffer < 0 || 
            static_cast<size_t>(bufferView.buffer) >= _model.buffers.size()) {
            return result;
        }

        // 计算数据大小
        size_t componentSize = 0;
        switch (accessor.componentType) {
            case COMPONENT_TYPE_BYTE:
            case COMPONENT_TYPE_UNSIGNED_BYTE:
                componentSize = 1;
                break;
            case COMPONENT_TYPE_SHORT:
            case COMPONENT_TYPE_UNSIGNED_SHORT:
                componentSize = 2;
                break;
            case COMPONENT_TYPE_UNSIGNED_INT:
                componentSize = 4;
                break;
            case COMPONENT_TYPE_FLOAT:
                componentSize = 4;
                break;
            default:
                return result;
        }

        size_t componentsPerElement = 1;
        if (accessor.type == "VEC2") componentsPerElement = 2;
        else if (accessor.type == "VEC3") componentsPerElement = 3;
        else if (accessor.type == "VEC4") componentsPerElement = 4;
        else if (accessor.type == "MAT2") componentsPerElement = 4;
        else if (accessor.type == "MAT3") componentsPerElement = 9;
        else if (accessor.type == "MAT4") componentsPerElement = 16;

        size_t elementSize = componentSize * componentsPerElement;
        size_t stride = (bufferView.byteStride > 0) ? bufferView.byteStride : elementSize;
        size_t totalSize = accessor.count * stride;

        size_t dataOffset = bufferView.byteOffset + accessor.byteOffset;
        
        if (dataOffset + totalSize > _binaryData.size()) {
            return result;
        }

        result.resize(totalSize);
        std::memcpy(result.data(), _binaryData.data() + dataOffset, totalSize);

        return result;
    }

    std::vector<gltfLoader::MeshData> gltfLoader::extractMeshData() const
    {
        std::vector<MeshData> result;

        if (!_loaded) {
            return result;
        }

        for (size_t nodeIdx = 0; nodeIdx < _model.nodes.size(); ++nodeIdx) {
            const Node& node = _model.nodes[nodeIdx];
            
            if (node.mesh < 0 || static_cast<size_t>(node.mesh) >= _model.meshes.size()) {
                continue;
            }

            const Mesh& mesh = _model.meshes[node.mesh];
            
            // 提取变换信息
            FVector3f translation(0.0f);
            FRotator3f rotation(0.0f, 0.0f, 0.0f);
            FVector3f scale(1.0f);

            if (node.translation.size() >= 3) {
                translation = FVector3f(node.translation[0], node.translation[1], node.translation[2]);
            }

            if (node.rotation.size() >= 4) {
                // glTF 使用 xyzw 顺序的四元数
                FQuatf quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
                rotation = quat.eulerAngles();
            }

            if (node.scale.size() >= 3) {
                scale = FVector3f(node.scale[0], node.scale[1], node.scale[2]);
            }

            // 处理每个 primitive
            for (const Primitive& primitive : mesh.primitives) {
                MeshData meshData;
                meshData.name = mesh.name;
                meshData.translation = translation;
                meshData.rotation = rotation;
                meshData.scale = scale;

                // 提取顶点位置
                if (primitive.attributes.count("POSITION") > 0) {
                    int posAccessorIdx = primitive.attributes.at("POSITION");
                    if (posAccessorIdx >= 0 && static_cast<size_t>(posAccessorIdx) < _model.accessors.size()) {
                        const Accessor& accessor = _model.accessors[posAccessorIdx];
                        auto floatData = readAccessorAs<float>(accessor);
                        
                        if (accessor.type == "VEC3" && floatData.size() >= accessor.count * 3) {
                            meshData.vertices.reserve(accessor.count);
                            for (size_t i = 0; i < accessor.count; ++i) {
                                meshData.vertices.emplace_back(
                                    floatData[i * 3],
                                    floatData[i * 3 + 1],
                                    floatData[i * 3 + 2]
                                );
                            }
                        }
                    }
                }

                // 提取法线
                if (primitive.attributes.count("NORMAL") > 0) {
                    int normalAccessorIdx = primitive.attributes.at("NORMAL");
                    if (normalAccessorIdx >= 0 && static_cast<size_t>(normalAccessorIdx) < _model.accessors.size()) {
                        const Accessor& accessor = _model.accessors[normalAccessorIdx];
                        auto floatData = readAccessorAs<float>(accessor);
                        
                        if (accessor.type == "VEC3" && floatData.size() >= accessor.count * 3) {
                            meshData.normals.reserve(accessor.count);
                            for (size_t i = 0; i < accessor.count; ++i) {
                                meshData.normals.emplace_back(
                                    floatData[i * 3],
                                    floatData[i * 3 + 1],
                                    floatData[i * 3 + 2]
                                );
                            }
                        }
                    }
                }

                // 提取纹理坐标
                if (primitive.attributes.count("TEXCOORD_0") > 0) {
                    int texcoordAccessorIdx = primitive.attributes.at("TEXCOORD_0");
                    if (texcoordAccessorIdx >= 0 && static_cast<size_t>(texcoordAccessorIdx) < _model.accessors.size()) {
                        const Accessor& accessor = _model.accessors[texcoordAccessorIdx];
                        auto floatData = readAccessorAs<float>(accessor);
                        
                        if (accessor.type == "VEC2" && floatData.size() >= accessor.count * 2) {
                            meshData.texcoords.reserve(accessor.count);
                            for (size_t i = 0; i < accessor.count; ++i) {
                                meshData.texcoords.emplace_back(
                                    floatData[i * 2],
                                    floatData[i * 2 + 1]
                                );
                            }
                        }
                    }
                }

                // 提取索引
                if (primitive.indices >= 0 && static_cast<size_t>(primitive.indices) < _model.accessors.size()) {
                    const Accessor& accessor = _model.accessors[primitive.indices];
                    meshData.indices = readAccessorAs<uint32_t>(accessor);
                } else {
                    // 如果没有索引，生成顺序索引
                    if (!meshData.vertices.empty()) {
                        meshData.indices.reserve(meshData.vertices.size());
                        for (size_t i = 0; i < meshData.vertices.size(); ++i) {
                            meshData.indices.push_back(static_cast<uint32_t>(i));
                        }
                    }
                }

                result.push_back(std::move(meshData));
            }
        }

        return result;
    }

}
