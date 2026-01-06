#include "gltfLoader.h"

#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <sstream>
#include <charconv>
#include <cctype>
#include <functional>

#include "fmt/format.h"

#include "math/vector.ixx"
#include "math/vector2.h"
#include "math/rotator.h"
#include "math/quat.h"

#include "util/timer/function_timer.h"
#include "util/file_util.ixx"
#include "util/base64/base64.ixx"


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
            setState(EAssetLoadState::FAILD);
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

        unload();
        _loaded = false;

        // 检查文件扩展名，决定是 GLB 还是纯 JSON GLTF
        std::string filePathStr(filePath);
        size_t dotPos = filePathStr.rfind('.');
        bool isGLB = false;
        
        if (dotPos != std::string::npos) {
            std::string ext = filePathStr.substr(dotPos + 1);
            // 转换为小写进行比较
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            isGLB = (ext == "glb");
        }

        setState(EAssetLoadState::PARSING_DATA);
        bool result = false;

        if (isGLB) {
            // GLB 格式：二进制文件
            auto fileResult = util::read_full_file(filePath);
            if (!fileResult.has_value()) {
                setError(EAssetLoaderError::FILE_NOT_FOUND);
                return false;
            }

            auto fileMapView = std::move(fileResult.value());
            const void* data = fileMapView.view.data();
            size_t size = fileMapView.view.size();
            result = parseGLB(data, size);
        } else {
            // 纯 JSON GLTF 格式
            result = parseGLTF(filePath);
        }

        if (result) {
            _loaded = true;
            setState(EAssetLoadState::COMPLETE);
        } else {
            setState(EAssetLoadState::FAILD);
        }

        return result;
    }
    
    void gltfLoader::unload()
    {
        _model = GltfModel();
        _bufferData.clear();
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
            
        // 对于 GLB 格式，第一个缓冲区（索引 0）应该使用 BIN chunk 的数据
        // 如果 buffers 数组为空，创建一个
        if (_model.buffers.empty() && !_bufferData.empty()) {
            Buffer buffer;
            buffer.byteLength = _bufferData[0].size();
            buffer.uri.clear();  // GLB 格式中，第一个缓冲区没有 URI
            _model.buffers.push_back(std::move(buffer));
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
                        n.translation.reserve(trans.size());
                        for (const auto& val : trans) {
                            if (val.isNumber()) {
                                n.translation.push_back(static_cast<float>(val.getNumber()));
                            }
                        }
                    }
                    if (node.has("rotation") && node["rotation"].isArray()) {
                        const auto& rot = node["rotation"].getArray();
                        n.rotation.reserve(rot.size());
                        for (const auto& val : rot) {
                            if (val.isNumber()) {
                                n.rotation.push_back(static_cast<float>(val.getNumber()));
                            }
                        }
                    }
                    if (node.has("scale") && node["scale"].isArray()) {
                        const auto& scale = node["scale"].getArray();
                        n.scale.reserve(scale.size());
                        for (const auto& val : scale) {
                            if (val.isNumber()) {
                                n.scale.push_back(static_cast<float>(val.getNumber()));
                            }
                        }
                    }
                    if (node.has("children") && node["children"].isArray()) {
                        const auto& children = node["children"].getArray();
                        n.children.reserve(children.size());
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

        // 解析 scenes
        if (root.has("scenes") && root["scenes"].isArray()) {
            const auto& scenes = root["scenes"].getArray();
            _model.scenes.reserve(scenes.size());
            for (const auto& scene : scenes) {
                Scene s;
                if (scene.isObject()) {
                    if (scene.has("name") && scene["name"].isString()) {
                        s.name = scene["name"].getString();
                    }
                    if (scene.has("nodes") && scene["nodes"].isArray()) {
                        const auto& nodes = scene["nodes"].getArray();
                        s.nodes.reserve(nodes.size());
                        for (const auto& val : nodes) {
                            if (val.isNumber()) {
                                s.nodes.push_back(static_cast<int>(val.getNumber()));
                            }
                        }
                    }
                }
                _model.scenes.push_back(std::move(s));
            }
        }

        // 解析默认场景
        if (root.has("scene") && root["scene"].isNumber()) {
            _model.scene = static_cast<int>(root["scene"].getNumber());
        }

        fmt::println("glTF model loaded: version={}, buffers={}, bufferViews={}, accessors={}, meshes={}, nodes={}, scenes={}",
                     _model.version, _model.buffers.size(), _model.bufferViews.size(),
                     _model.accessors.size(), _model.meshes.size(), _model.nodes.size(), _model.scenes.size());

        return true;
    }

    bool gltfLoader::parseBinaryChunk(const std::byte* binData, size_t binSize)
    {
        // 将 BIN chunk 数据添加到第一个缓冲区
        std::vector<std::byte> buffer;
        buffer.assign(binData, binData + binSize);
        _bufferData.push_back(std::move(buffer));
        fmt::println("Binary chunk loaded: {} bytes", binSize);
        return true;
    }

    bool gltfLoader::parseGLTF(const char* filePath)
    {
        // 读取 JSON 文件
        auto fileResult = util::read_full_file(filePath);
        if (!fileResult.has_value()) {
            setError(EAssetLoaderError::FILE_NOT_FOUND);
            return false;
        }

        auto fileMapView = std::move(fileResult.value());
        const char* jsonData = reinterpret_cast<const char*>(fileMapView.view.data());
        size_t jsonSize = fileMapView.view.size();

        // 解析 JSON
        if (!parseJSONChunk(jsonData, jsonSize)) {
            return false;
        }

        // 加载所有缓冲区
        std::string basePath = util::get_file_directory(filePath);
        for (size_t i = 0; i < _model.buffers.size(); ++i) {
            const Buffer& buffer = _model.buffers[i];
            if (buffer.uri.empty()) {
                // GLB 格式的缓冲区，应该已经在 parseBinaryChunk 中加载
                // 如果 _bufferData 中还没有对应索引的数据，创建一个空缓冲区
                if (i >= _bufferData.size()) {
                    _bufferData.resize(i + 1);
                    _bufferData[i].resize(buffer.byteLength);
                }
            } else if (buffer.uri.find("data:") == 0) {
                // Data URI，需要解码
                if (!loadDataURIBuffer(buffer, i)) {
                    fmt::println("Warning: Failed to decode data URI buffer {}: {}", i, buffer.uri.substr(0, 50));
                    // 创建一个空缓冲区作为占位符
                    if (i >= _bufferData.size()) {
                        _bufferData.resize(i + 1);
                    }
                    _bufferData[i].resize(buffer.byteLength);
                }
            } else {
                // 外部文件，需要加载
                if (!loadExternalBuffer(buffer, i, basePath.c_str())) {
                    fmt::println("Warning: Failed to load external buffer {}: {}", i, buffer.uri);
                    // 创建一个空缓冲区作为占位符
                    if (i >= _bufferData.size()) {
                        _bufferData.resize(i + 1);
                    }
                    _bufferData[i].resize(buffer.byteLength);
                }
            }
        }

        return true;
    }

    bool gltfLoader::loadDataURIBuffer(const Buffer& buffer, size_t bufferIndex)
    {
        if (buffer.uri.empty() || buffer.uri.find("data:") != 0) {
            return false;
        }

        // 解析 data URI: data:[<mime_type>][;base64],<data>
        size_t commaPos = buffer.uri.find(',', 5);
        if (commaPos == std::string::npos) {
            return false;
        }

        std::string_view metaView = std::string_view(buffer.uri).substr(5, commaPos - 5);
        std::string_view dataView = std::string_view(buffer.uri).substr(commaPos + 1);

        // 检查是否使用 base64 编码
        bool isBase64 = metaView.find(";base64") != std::string_view::npos;

        std::vector<std::byte> decodedData;

        if (isBase64) {
            // Base64 解码
            try {
                auto decoded = shine::base64::base64_decode(dataView);
                decodedData.reserve(decoded.size());
                for (unsigned char byte : decoded) {
                    decodedData.push_back(static_cast<std::byte>(byte));
                }
            } catch (...) {
                return false;
            }
        } else {
            // URL 编码的数据（直接使用）
            decodedData.reserve(dataView.size());
            for (char c : dataView) {
                decodedData.push_back(static_cast<std::byte>(c));
            }
        }

        // 验证大小
        if (decodedData.size() != buffer.byteLength) {
            fmt::println("Warning: Data URI buffer size mismatch: expected {}, got {}", 
                        buffer.byteLength, decodedData.size());
            // 调整大小以匹配预期
            if (decodedData.size() < buffer.byteLength) {
                decodedData.resize(buffer.byteLength);
            } else {
                decodedData.resize(buffer.byteLength);
            }
        }

        // 确保缓冲区数组足够大
        if (bufferIndex >= _bufferData.size()) {
            _bufferData.resize(bufferIndex + 1);
        }
        _bufferData[bufferIndex] = std::move(decodedData);

        return true;
    }

    bool gltfLoader::loadExternalBuffer(const Buffer& buffer, size_t bufferIndex, const char* basePath)
    {
        if (buffer.uri.empty()) {
            return false;
        }

        // 构建完整路径
        std::string fullPath;
        if (basePath && basePath[0] != '\0') {
            fullPath = basePath;
            // 检查是否需要添加路径分隔符
            if (fullPath.back() != '/' && fullPath.back() != '\\') {
                fullPath += "/";
            }
            fullPath += buffer.uri;
        } else {
            fullPath = buffer.uri;
        }

        // 读取外部缓冲区文件
        auto fileResult = util::read_full_file(fullPath);
        if (!fileResult.has_value()) {
            return false;
        }

        auto fileMapView = std::move(fileResult.value());
        
        // 验证文件大小
        if (fileMapView.view.size() != buffer.byteLength) {
            fmt::println("Warning: External buffer size mismatch: expected {}, got {}", 
                        buffer.byteLength, fileMapView.view.size());
        }

        // 将数据添加到对应索引的缓冲区
        if (bufferIndex >= _bufferData.size()) {
            _bufferData.resize(bufferIndex + 1);
        }
        
        _bufferData[bufferIndex].assign(fileMapView.view.data(), 
                                       fileMapView.view.data() + fileMapView.view.size());

        return true;
    }

    const std::vector<std::byte>& gltfLoader::getBufferData(int bufferIndex) const
    {
        static const std::vector<std::byte> emptyBuffer;
        
        if (bufferIndex < 0 || static_cast<size_t>(bufferIndex) >= _bufferData.size()) {
            return emptyBuffer;
        }
        
        return _bufferData[bufferIndex];
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

        // 获取缓冲区数据
        const std::vector<std::byte>& bufferData = getBufferData(bufferView.buffer);
        if (bufferData.empty()) {
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
        size_t totalSize = (accessor.count - 1) * stride + elementSize;

        size_t dataOffset = bufferView.byteOffset + accessor.byteOffset;
        
        if (dataOffset + totalSize > bufferData.size()) {
            return result;
        }

        result.resize(totalSize);
        std::memcpy(result.data(), bufferData.data() + dataOffset, totalSize);

        return result;
    }

    std::vector<MeshData> gltfLoader::extractMeshData() const
    {
        std::vector<MeshData> result;

        if (!_loaded) {
            return result;
        }

        // 确定要处理的节点列表
        std::vector<int> nodesToProcess;
        
        if (_model.scene >= 0 && static_cast<size_t>(_model.scene) < _model.scenes.size()) {
            // 使用默认场景的节点
            const Scene& scene = _model.scenes[_model.scene];
            nodesToProcess = scene.nodes;
        } else if (!_model.scenes.empty()) {
            // 使用第一个场景的节点
            nodesToProcess = _model.scenes[0].nodes;
        } else {
            // 如果没有场景，处理所有有 mesh 的节点
            for (size_t i = 0; i < _model.nodes.size(); ++i) {
                if (_model.nodes[i].mesh >= 0) {
                    nodesToProcess.push_back(static_cast<int>(i));
                }
            }
        }

        // 递归处理节点（包括子节点）
        std::function<void(int, const math::FVector3f&, const math::FRotator3f&, const math::FVector3f&)> processNode;
        processNode = [&](int nodeIdx, const math::FVector3f& parentTranslation, 
                         const math::FRotator3f& parentRotation, const math::FVector3f& parentScale) {
            if (nodeIdx < 0 || static_cast<size_t>(nodeIdx) >= _model.nodes.size()) {
                return;
            }

            const Node& node = _model.nodes[nodeIdx];
            
            // 计算当前节点的变换
            math::FVector3f translation = parentTranslation;
            math::FRotator3f rotation = parentRotation;
            math::FVector3f scale = parentScale;

            if (node.translation.size() >= 3) {
                translation = parentTranslation + math::FVector3f(node.translation[0], node.translation[1], node.translation[2]);
            }

            if (node.rotation.size() >= 4) {
                // glTF 使用 xyzw 顺序的四元数
                math::FQuatf quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
                rotation = quat.eulerAngles();
            }

            if (node.scale.size() >= 3) {
                scale = parentScale * math::FVector3f(node.scale[0], node.scale[1], node.scale[2]);
            }

            // 处理当前节点的 mesh
            if (node.mesh >= 0 && static_cast<size_t>(node.mesh) < _model.meshes.size()) {
                const Mesh& mesh = _model.meshes[node.mesh];

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

                // 提取纹理坐标（支持多个纹理坐标集）
                for (int texCoordSet = 0; texCoordSet < 8; ++texCoordSet) {
                    std::string attrName = (texCoordSet == 0) ? "TEXCOORD_0" : 
                                           fmt::format("TEXCOORD_{}", texCoordSet);
                    
                    if (primitive.attributes.count(attrName) > 0) {
                        int texcoordAccessorIdx = primitive.attributes.at(attrName);
                        if (texcoordAccessorIdx >= 0 && static_cast<size_t>(texcoordAccessorIdx) < _model.accessors.size()) {
                            const Accessor& accessor = _model.accessors[texcoordAccessorIdx];
                            auto floatData = readAccessorAs<float>(accessor);
                            
                            if (accessor.type == "VEC2" && floatData.size() >= accessor.count * 2) {
                                // 只提取第一个纹理坐标集到 texcoords
                                if (texCoordSet == 0) {
                                    meshData.texcoords.reserve(accessor.count);
                                    for (size_t i = 0; i < accessor.count; ++i) {
                                        meshData.texcoords.emplace_back(
                                            floatData[i * 2],
                                            floatData[i * 2 + 1]
                                        );
                                    }
                                }
                                // TODO: 可以扩展 MeshData 结构以支持多个纹理坐标集
                            }
                        }
                    }
                }

                // 提取顶点颜色（COLOR_0）
                if (primitive.attributes.count("COLOR_0") > 0) {
                    int colorAccessorIdx = primitive.attributes.at("COLOR_0");
                    if (colorAccessorIdx >= 0 && static_cast<size_t>(colorAccessorIdx) < _model.accessors.size()) {
                        const Accessor& accessor = _model.accessors[colorAccessorIdx];
                        auto floatData = readAccessorAs<float>(accessor);
                        
                        if (accessor.type == "VEC3" && floatData.size() >= accessor.count * 3) {
                            meshData.colors.reserve(accessor.count);
                            for (size_t i = 0; i < accessor.count; ++i) {
                                meshData.colors.emplace_back(
                                    floatData[i * 3],
                                    floatData[i * 3 + 1],
                                    floatData[i * 3 + 2],
                                    1.0f  // Alpha 默认为 1.0
                                );
                            }
                        } else if (accessor.type == "VEC4" && floatData.size() >= accessor.count * 4) {
                            meshData.colors.reserve(accessor.count);
                            for (size_t i = 0; i < accessor.count; ++i) {
                                meshData.colors.emplace_back(
                                    floatData[i * 4],
                                    floatData[i * 4 + 1],
                                    floatData[i * 4 + 2],
                                    floatData[i * 4 + 3]
                                );
                            }
                        } else if (accessor.componentType == COMPONENT_TYPE_UNSIGNED_BYTE) {
                            // 处理归一化的 UNSIGNED_BYTE 颜色数据
                            if (accessor.type == "VEC3" || accessor.type == "VEC4") {
                                auto byteData = readAccessorAs<uint8_t>(accessor);
                                size_t components = (accessor.type == "VEC3") ? 3 : 4;
                                meshData.colors.reserve(accessor.count);
                                for (size_t i = 0; i < accessor.count; ++i) {
                                    float r = byteData[i * components] / 255.0f;
                                    float g = byteData[i * components + 1] / 255.0f;
                                    float b = byteData[i * components + 2] / 255.0f;
                                    float a = (components == 4) ? byteData[i * components + 3] / 255.0f : 1.0f;
                                    meshData.colors.emplace_back(r, g, b, a);
                                }
                            }
                        }
                    }
                }

                // 保存材质索引
                meshData.materialIndex = primitive.material;

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

            // 递归处理子节点
            for (int childIdx : node.children) {
                processNode(childIdx, translation, rotation, scale);
            }
        };

        // 处理所有根节点
        math::FVector3f rootTranslation(0.0f);
        math::FRotator3f rootRotation(0.0f, 0.0f, 0.0f);
        math::FVector3f rootScale(1.0f);

        for (int nodeIdx : nodesToProcess) {
            processNode(nodeIdx, rootTranslation, rootRotation, rootScale);
        }

        return result;
    }

    std::vector<int> gltfLoader::getSceneRootNodes(int sceneIndex) const
    {
        std::vector<int> result;
        
        if (sceneIndex < 0 || static_cast<size_t>(sceneIndex) >= _model.scenes.size()) {
            return result;
        }
        
        const Scene& scene = _model.scenes[sceneIndex];
        return scene.nodes;
    }

    size_t gltfLoader::getMeshCount() const noexcept
    {
        if (!_loaded)
        {
            return 0;
        }

        size_t count = 0;
        for (const auto& mesh : _model.meshes)
        {
            count += mesh.primitives.size();
        }
        return count;
    }

}
