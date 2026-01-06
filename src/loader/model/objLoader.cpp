#include "objLoader.h"
#include <unordered_set>

#include <vector>
#include <string>
#include <algorithm>
#include <charconv>
#include <functional>

#include "fmt/format.h"

#include "math/vector.ixx"
#include "math/vector2.h"
#include "math/rotator.h"

#include "util/timer/function_timer.h"
#include "util/file_util.ixx"

namespace shine::loader
{
    using namespace shine::math;

    namespace {
        // 辅助函数：跳过空白字符
        void skipWhitespace(const char*& ptr, const char* end) {
            while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\r')) {
                ++ptr;
            }
        }

        // 辅助函数：读取浮点数
        bool parseFloatValue(const char*& ptr, const char* end, float& out) {
            skipWhitespace(ptr, end);
            const char* start = ptr;
            bool hasDot = false;
            bool hasExp = false;
            
            if (ptr < end && (*ptr == '-' || *ptr == '+')) ++ptr;
            if (ptr >= end || (*ptr < '0' || *ptr > '9')) return false;
            
            while (ptr < end) {
                if (*ptr >= '0' && *ptr <= '9') {
                    // Digit - always allowed
                    ++ptr;
                } else if (*ptr == '.') {
                    if (hasDot) break;
                    hasDot = true;
                    ++ptr;
                } else if (*ptr == 'e' || *ptr == 'E') {
                    if (hasExp) break;
                    hasExp = true;
                    ++ptr;
                    // Allow sign immediately after 'e' or 'E'
                    if (ptr < end && (*ptr == '-' || *ptr == '+')) {
                        ++ptr;
                    }
                } else {
                    // Invalid character - stop parsing
                    break;
                }
            }
            
            double value = 0.0;
            auto [p, ec] = std::from_chars(start, ptr, value);
            if (ec == std::errc()) {
                out = static_cast<float>(value);
                return true;
            }
            return false;
        }

        // 辅助函数：读取整数
        bool parseIntValue(const char*& ptr, const char* end, int& out) {
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
                out = negative ? -static_cast<int>(value) : static_cast<int>(value);
                return true;
            }
            return false;
        }
    }

    bool objLoader::loadFromMemory(const void* data, size_t size)
    {
        util::FunctionTimer timer("objLoader::loadFromMemory");
        setState(EAssetLoadState::READING_FILE);

        if (!data || size == 0) {
            setError(EAssetLoaderError::INVALID_PARAMETER);
            return false;
        }

        unload();
        _loaded = false;

        setState(EAssetLoadState::PARSING_DATA);
        const char* dataPtr = static_cast<const char*>(data);
        bool result = parseOBJ(dataPtr, size);

        if (result) {
            _loaded = true;
            setState(EAssetLoadState::COMPLETE);
        } else {
            setState(EAssetLoadState::FAILD);
        }

        return result;
    }

    bool objLoader::loadFromFile(const char* filePath)
    {
        util::FunctionTimer timer("objLoader::loadFromFile");
        setState(EAssetLoadState::READING_FILE);

        if (!filePath) {
            setError(EAssetLoaderError::INVALID_PARAMETER);
            return false;
        }

        unload();
        _loaded = false;

        // 获取文件基础路径
        _basePath = util::get_file_directory(filePath);

        // 读取文件
        auto fileResult = util::read_full_file(filePath);
        if (!fileResult.has_value()) {
            setError(EAssetLoaderError::FILE_NOT_FOUND);
            return false;
        }

        auto fileMapView = std::move(fileResult.value());
        const char* data = reinterpret_cast<const char*>(fileMapView.view.data());
        size_t size = fileMapView.view.size();

        setState(EAssetLoadState::PARSING_DATA);
        bool result = parseOBJ(data, size, _basePath.c_str());

        if (result) {
            _loaded = true;
            setState(EAssetLoadState::COMPLETE);
        } else {
            setState(EAssetLoadState::FAILD);
        }

        return result;
    }
    
    void objLoader::unload()
    {
        _model = ObjModel();
        _loaded = false;
        _basePath.clear();
        _currentMaterialIndex = -1;
        setState(EAssetLoadState::NONE);
    }

    bool objLoader::parseOBJ(const char* data, size_t size, const char* basePath)
    {
        if (!data || size == 0) {
            setError(EAssetLoaderError::INVALID_FORMAT);
            return false;
        }

        const char* ptr = data;
        const char* end = data + size;
        std::string_view line;

        // 创建默认组
        ObjGroup defaultGroup;
        defaultGroup.name = "default";
        _model.groups.push_back(defaultGroup);
        int currentGroupIndex = 0;

        while (ptr < end) {
            const char* lineStart = ptr;
            
            // 找到行尾
            while (ptr < end && *ptr != '\n' && *ptr != '\r') {
                ++ptr;
            }
            
            // 创建行视图
            line = std::string_view(lineStart, ptr - lineStart);
            
            // 跳过换行符
            while (ptr < end && (*ptr == '\n' || *ptr == '\r')) {
                ++ptr;
            }

            // 跳过空行
            if (line.empty()) {
                continue;
            }

            // 移除行尾的空白字符
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
                line = line.substr(0, line.size() - 1);
            }

            // 跳过注释
            if (!line.empty() && line[0] == '#') {
                continue;
            }

            // 解析行
            parseLine(line, basePath);

            // 更新当前组索引（如果组发生了变化）
            if (!_model.groups.empty() && currentGroupIndex < static_cast<int>(_model.groups.size())) {
                currentGroupIndex = static_cast<int>(_model.groups.size()) - 1;
            }
        }

        fmt::println("OBJ model loaded: vertices={}, texCoords={}, normals={}, groups={}, materials={}",
                     _model.vertices.size(), _model.texCoords.size(), 
                     _model.normals.size(), _model.groups.size(), _model.materials.size());

        return true;
    }

    void objLoader::parseLine(const std::string_view& line, const char* basePath)
    {
        if (line.empty()) {
            return;
        }

        // 跳过前导空白
        size_t start = 0;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
            ++start;
        }
        if (start >= line.size()) {
            return;
        }

        std::string_view trimmed = line.substr(start);
        
        // 根据第一个字符判断命令类型
        if (trimmed.empty()) {
            return;
        }

        char cmd = trimmed[0];
        
        switch (cmd) {
            case 'v':
                if (trimmed.size() > 1) {
                    if (trimmed[1] == 't') {
                        // vt - 纹理坐标
                        parseTexCoord(trimmed.substr(2));
                    } else if (trimmed[1] == 'n') {
                        // vn - 法线
                        parseNormal(trimmed.substr(2));
                    } else if (trimmed[1] == ' ') {
                        // v - 顶点
                        parseVertex(trimmed.substr(1));
                    }
                } else {
                    // v - 顶点
                    parseVertex(trimmed.substr(1));
                }
                break;
                
            case 'f':
                // f - 面
                parseFace(trimmed.substr(1));
                break;
                
            case 'g':
            case 'o':
                // g/o - 组/对象
                parseGroup(trimmed.substr(1));
                break;
                
            case 'u':
                // usemtl - 使用材质
                if (trimmed.size() > 6 && trimmed.substr(0, 7) == "usemtl ") {
                    parseUseMaterial(trimmed.substr(7));
                }
                break;
                
            case 'm':
                // mtllib - 材质库
                if (trimmed.size() > 6 && trimmed.substr(0, 7) == "mtllib ") {
                    parseMaterialLib(trimmed.substr(7), basePath);
                }
                break;
                
            case 's':
                // s - 平滑组（暂时忽略）
                break;
                
            default:
                // 未知命令，忽略
                break;
        }
    }

    void objLoader::parseVertex(const std::string_view& line)
    {
        const char* ptr = line.data();
        const char* end = line.data() + line.size();
        
        ObjVertex vertex;
        
        if (!parseFloatValue(ptr, end, vertex.x)) return;
        if (!parseFloatValue(ptr, end, vertex.y)) return;
        if (ptr < end) {
            parseFloatValue(ptr, end, vertex.z);
        }
        if (ptr < end) {
            parseFloatValue(ptr, end, vertex.w);
        }
        
        _model.vertices.push_back(vertex);
    }

    void objLoader::parseTexCoord(const std::string_view& line)
    {
        const char* ptr = line.data();
        const char* end = line.data() + line.size();
        
        ObjTexCoord texCoord;
        
        if (!parseFloatValue(ptr, end, texCoord.u)) return;
        if (ptr < end) {
            parseFloatValue(ptr, end, texCoord.v);
        }
        if (ptr < end) {
            parseFloatValue(ptr, end, texCoord.w);
        }
        
        _model.texCoords.push_back(texCoord);
    }

    void objLoader::parseNormal(const std::string_view& line)
    {
        const char* ptr = line.data();
        const char* end = line.data() + line.size();
        
        ObjNormal normal;
        
        if (!parseFloatValue(ptr, end, normal.x)) return;
        if (!parseFloatValue(ptr, end, normal.y)) return;
        if (!parseFloatValue(ptr, end, normal.z)) return;
        
        _model.normals.push_back(normal);
    }

    void objLoader::parseFace(const std::string_view& line)
    {
        if (_model.groups.empty()) {
            ObjGroup defaultGroup;
            defaultGroup.name = "default";
            _model.groups.push_back(defaultGroup);
        }

        ObjGroup& currentGroup = _model.groups.back();
        ObjFace face;
        face.materialIndex = _currentMaterialIndex;

        // 解析面的顶点索引
        // 格式可能是: f v1 v2 v3 或 f v1/vt1 v2/vt2 v3/vt3 或 f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
        const char* ptr = line.data();
        const char* end = line.data() + line.size();
        
        skipWhitespace(ptr, end);
        
        while (ptr < end) {
            skipWhitespace(ptr, end);
            if (ptr >= end) break;
            
            // 读取顶点索引（可能包含 / 分隔符）
            const char* indexStart = ptr;
            while (ptr < end && *ptr != ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n') {
                ++ptr;
            }
            
            if (ptr > indexStart) {
                std::string_view indexStr(indexStart, ptr - indexStart);
                
                // 解析索引：可能是 v, v/vt, v/vt/vn, v//vn 等格式
                size_t slash1 = indexStr.find('/');
                
                if (slash1 == std::string_view::npos) {
                    // 只有顶点索引: v
                    int vIdx = parseInt(indexStr);
                    if (vIdx != 0) {
                        face.vertexIndices.push_back(vIdx);
                        face.texCoordIndices.push_back(0);  // 无纹理坐标
                        face.normalIndices.push_back(0);     // 无法线
                    }
                } else {
                    // 有分隔符
                    std::string_view vPart = indexStr.substr(0, slash1);
                    int vIdx = parseInt(vPart);
                    
                    size_t slash2 = indexStr.find('/', slash1 + 1);
                    
                    if (slash2 == std::string_view::npos) {
                        // v/vt 格式
                        std::string_view vtPart = indexStr.substr(slash1 + 1);
                        int vtIdx = parseInt(vtPart);
                        
                        face.vertexIndices.push_back(vIdx);
                        face.texCoordIndices.push_back(vtIdx);
                        face.normalIndices.push_back(0);
                    } else {
                        // v/vt/vn 或 v//vn 格式
                        std::string_view vtPart = indexStr.substr(slash1 + 1, slash2 - slash1 - 1);
                        std::string_view vnPart = indexStr.substr(slash2 + 1);
                        
                        int vtIdx = 0;
                        int vnIdx = parseInt(vnPart);
                        
                        if (!vtPart.empty()) {
                            vtIdx = parseInt(vtPart);
                        }
                        
                        face.vertexIndices.push_back(vIdx);
                        face.texCoordIndices.push_back(vtIdx);
                        face.normalIndices.push_back(vnIdx);
                    }
                }
            }
        }
        
        // OBJ 格式使用三角形扇或三角形带，需要转换为三角形
        // 这里简单处理：将多边形分解为三角形
        if (face.vertexIndices.size() >= 3) {
            // 使用三角形扇分解
            for (size_t i = 1; i < face.vertexIndices.size() - 1; ++i) {
                ObjFace triangle;
                triangle.materialIndex = face.materialIndex;
                
                // 第一个顶点
                triangle.vertexIndices.push_back(face.vertexIndices[0]);
                triangle.texCoordIndices.push_back(face.texCoordIndices[0]);
                triangle.normalIndices.push_back(face.normalIndices[0]);
                
                // 第二个顶点
                triangle.vertexIndices.push_back(face.vertexIndices[i]);
                triangle.texCoordIndices.push_back(face.texCoordIndices[i]);
                triangle.normalIndices.push_back(face.normalIndices[i]);
                
                // 第三个顶点
                triangle.vertexIndices.push_back(face.vertexIndices[i + 1]);
                triangle.texCoordIndices.push_back(face.texCoordIndices[i + 1]);
                triangle.normalIndices.push_back(face.normalIndices[i + 1]);
                
                currentGroup.faces.push_back(triangle);
            }
        }
    }

    void objLoader::parseGroup(const std::string_view& line)
    {
        // 跳过前导空白
        size_t start = 0;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
            ++start;
        }
        
        std::string_view name = line.substr(start);
        
        // 移除尾随空白
        while (!name.empty() && (name.back() == ' ' || name.back() == '\t' || name.back() == '\r')) {
            name = name.substr(0, name.size() - 1);
        }
        
        if (name.empty()) {
            name = "default";
        }
        
        ObjGroup group;
        group.name = std::string(name);
        group.materialIndex = _currentMaterialIndex;
        _model.groups.push_back(group);
    }

    void objLoader::parseUseMaterial(const std::string_view& line)
    {
        // 跳过前导空白
        size_t start = 0;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
            ++start;
        }
        
        std::string_view name = line.substr(start);
        
        // 移除尾随空白
        while (!name.empty() && (name.back() == ' ' || name.back() == '\t' || name.back() == '\r')) {
            name = name.substr(0, name.size() - 1);
        }
        
        if (!name.empty()) {
            std::string materialName(name);
            // 使用稳定的材质名称到索引映射进行查找
            auto it = _model.materialNameToIndex.find(materialName);
            if (it != _model.materialNameToIndex.end()) {
                _currentMaterialIndex = it->second;
            } else {
                // 如果没找到，设置为 -1
                _currentMaterialIndex = -1;
            }
        }
    }

    void objLoader::parseMaterialLib(const std::string_view& line, const char* basePath)
    {
        // 跳过前导空白
        size_t start = 0;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
            ++start;
        }
        
        std::string_view mtlPath = line.substr(start);
        
        // 移除尾随空白
        while (!mtlPath.empty() && (mtlPath.back() == ' ' || mtlPath.back() == '\t' || mtlPath.back() == '\r')) {
            mtlPath = mtlPath.substr(0, mtlPath.size() - 1);
        }
        
        if (mtlPath.empty()) {
            return;
        }
        
        // 构建完整路径
        std::string fullPath;
        if (basePath && basePath[0] != '\0') {
            fullPath = basePath;
            if (fullPath.back() != '/' && fullPath.back() != '\\') {
                fullPath += "/";
            }
            fullPath += std::string(mtlPath);
        } else {
            fullPath = std::string(mtlPath);
        }
        
        _model.mtlLibPath = fullPath;
        
        // 解析 MTL 文件
        parseMTL(fullPath.c_str());
    }

    bool objLoader::parseMTL(const char* filePath)
    {
        if (!filePath) {
            return false;
        }

        auto fileResult = util::read_full_file(filePath);
        if (!fileResult.has_value()) {
            fmt::println("Warning: Failed to load MTL file: {}", filePath);
            return false;
        }

        auto fileMapView = std::move(fileResult.value());
        const char* data = reinterpret_cast<const char*>(fileMapView.view.data());
        size_t size = fileMapView.view.size();

        const char* ptr = data;
        const char* end = data + size;
        std::string_view line;
        
        ObjMaterial* currentMaterial = nullptr;
        std::string mtlBasePath = util::get_file_directory(filePath);

        while (ptr < end) {
            const char* lineStart = ptr;
            
            // 找到行尾
            while (ptr < end && *ptr != '\n' && *ptr != '\r') {
                ++ptr;
            }
            
            line = std::string_view(lineStart, ptr - lineStart);
            
            // 跳过换行符
            while (ptr < end && (*ptr == '\n' || *ptr == '\r')) {
                ++ptr;
            }

            if (line.empty()) {
                continue;
            }

            // 移除行尾空白
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
                line = line.substr(0, line.size() - 1);
            }

            // 跳过注释
            if (!line.empty() && line[0] == '#') {
                continue;
            }

            // 跳过前导空白
            size_t start = 0;
            while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
                ++start;
            }
            if (start >= line.size()) {
                continue;
            }

            std::string_view trimmed = line.substr(start);

            if (trimmed.empty()) {
                continue;
            }

            // 解析 MTL 命令
            if (trimmed.size() > 6 && trimmed.substr(0, 7) == "newmtl ") {
                // 新材质
                std::string_view name = trimmed.substr(7);
                while (!name.empty() && (name.back() == ' ' || name.back() == '\t')) {
                    name = name.substr(0, name.size() - 1);
                }
                
                ObjMaterial material;
                material.name = std::string(name);
                
                // 如果材质已存在，使用现有索引；否则分配新索引
                int materialIndex;
                if (_model.materialNameToIndex.count(material.name) > 0) {
                    materialIndex = _model.materialNameToIndex[material.name];
                } else {
                    materialIndex = static_cast<int>(_model.materials.size());
                    _model.materialNameToIndex[material.name] = materialIndex;
                }
                
                _model.materials[material.name] = material;
                currentMaterial = &_model.materials[material.name];
            } else if (currentMaterial) {
                const char* cmdPtr = trimmed.data();
                const char* cmdEnd = trimmed.data() + trimmed.size();
                
                if (trimmed[0] == 'K') {
                    // Ka, Kd, Ks, Ke
                    if (trimmed.size() > 1) {
                        float values[3] = {0.0f, 0.0f, 0.0f};
                        const char* valPtr = cmdPtr + 2;
                        parseFloatValue(valPtr, cmdEnd, values[0]);
                        parseFloatValue(valPtr, cmdEnd, values[1]);
                        parseFloatValue(valPtr, cmdEnd, values[2]);
                        
                        switch (trimmed[1]) {
                            case 'a':  // Ka - Ambient
                                currentMaterial->ambient[0] = values[0];
                                currentMaterial->ambient[1] = values[1];
                                currentMaterial->ambient[2] = values[2];
                                break;
                            case 'd':  // Kd - Diffuse
                                currentMaterial->diffuse[0] = values[0];
                                currentMaterial->diffuse[1] = values[1];
                                currentMaterial->diffuse[2] = values[2];
                                break;
                            case 's':  // Ks - Specular
                                currentMaterial->specular[0] = values[0];
                                currentMaterial->specular[1] = values[1];
                                currentMaterial->specular[2] = values[2];
                                break;
                            case 'e':  // Ke - Emissive
                                currentMaterial->emissive[0] = values[0];
                                currentMaterial->emissive[1] = values[1];
                                currentMaterial->emissive[2] = values[2];
                                break;
                        }
                    }
                } else if (trimmed.size() > 1 && trimmed[0] == 'N') {
                    // Ns, Ni
                    float value = 0.0f;
                    const char* valPtr = cmdPtr + 2;
                    parseFloatValue(valPtr, cmdEnd, value);
                    
                    if (trimmed[1] == 's') {  // Ns - Shininess
                        currentMaterial->shininess = value;
                    } else if (trimmed[1] == 'i') {  // Ni - Refraction
                        currentMaterial->refraction = value;
                    }
                } else if (trimmed.size() > 1 && trimmed[0] == 'd') {
                    // d - Dissolve (transparency)
                    float value = 1.0f;
                    const char* valPtr = cmdPtr + 1;
                    parseFloatValue(valPtr, cmdEnd, value);
                    currentMaterial->dissolve = value;
                    currentMaterial->transparency = value;
                } else if (trimmed.size() > 1 && trimmed[0] == 'T') {
                    // Tr - Transparency (inverse of dissolve)
                    float value = 0.0f;
                    const char* valPtr = cmdPtr + 2;
                    parseFloatValue(valPtr, cmdEnd, value);
                    currentMaterial->transparency = value;
                    currentMaterial->dissolve = 1.0f - value;
                } else if (trimmed.size() > 5 && trimmed.substr(0, 5) == "illum") {
                    // illum - Illumination model
                    int value = 0;
                    const char* valPtr = cmdPtr + 5;
                    skipWhitespace(valPtr, cmdEnd);
                    parseIntValue(valPtr, cmdEnd, value);
                    currentMaterial->illuminationModel = value;
                } else if (trimmed.size() > 6 && trimmed.substr(0, 7) == "map_Ka ") {
                    // map_Ka - Ambient map
                    std::string_view path = trimmed.substr(7);
                    currentMaterial->ambientMap = std::string(path);
                } else if (trimmed.size() > 6 && trimmed.substr(0, 7) == "map_Kd ") {
                    // map_Kd - Diffuse map
                    std::string_view path = trimmed.substr(7);
                    currentMaterial->diffuseMap = std::string(path);
                } else if (trimmed.size() > 6 && trimmed.substr(0, 7) == "map_Ks ") {
                    // map_Ks - Specular map
                    std::string_view path = trimmed.substr(7);
                    currentMaterial->specularMap = std::string(path);
                } else if (trimmed.size() > 4 && trimmed.substr(0, 5) == "map_B") {
                    // map_Bump or bump - Bump map
                    size_t spacePos = trimmed.find(' ');
                    if (spacePos != std::string_view::npos && spacePos + 1 < trimmed.size()) {
                        std::string_view path = trimmed.substr(spacePos + 1);
                        currentMaterial->bumpMap = std::string(path);
                    }
                } else if (trimmed.size() > 8 && trimmed.substr(0, 9) == "map_Normal") {
                    // map_Normal - Normal map
                    size_t spacePos = trimmed.find(' ');
                    if (spacePos != std::string_view::npos && spacePos + 1 < trimmed.size()) {
                        std::string_view path = trimmed.substr(spacePos + 1);
                        currentMaterial->normalMap = std::string(path);
                    }
                }
            }
        }

        fmt::println("MTL file loaded: {} materials", _model.materials.size());
        return true;
    }

    std::vector<MeshData> objLoader::extractMeshData() const
    {
        std::vector<MeshData> result;

        if (!_loaded) {
            return result;
        }

        // 为每个组创建一个 MeshData
        for (const ObjGroup& group : _model.groups) {
            if (group.faces.empty()) {
                continue;
            }

            // 按材质分组面，为每个材质创建一个 MeshData
            std::unordered_map<int, std::vector<const ObjFace*>> facesByMaterial;
            for (const ObjFace& face : group.faces) {
                int matIdx = (face.materialIndex >= 0) ? face.materialIndex : group.materialIndex;
                facesByMaterial[matIdx].push_back(&face);
            }

            // 为每个材质创建一个 MeshData
            for (const auto& [matIdx, faces] : facesByMaterial) {
                MeshData meshData;
                meshData.name = group.name;
                if (!faces.empty() && faces[0]->materialIndex >= 0) {
                    meshData.name += "_mat_" + std::to_string(matIdx);
                }
                meshData.materialIndex = matIdx;
                meshData.translation = math::FVector3f(0.0f);
                meshData.rotation = math::FRotator3f(0.0f, 0.0f, 0.0f);
                meshData.scale = math::FVector3f(1.0f);

                // 用于索引映射（OBJ 索引从 1 开始，需要转换为从 0 开始）
                std::unordered_map<std::string, uint32_t> indexMap;
                uint32_t nextIndex = 0;

                // 处理每个面
                for (const ObjFace* facePtr : faces) {
                    const ObjFace& face = *facePtr;
                    if (face.vertexIndices.size() != 3) {
                        continue;  // 只处理三角形
                    }

                    // 为每个顶点创建唯一索引
                    for (size_t i = 0; i < 3; ++i) {
                        int vIdx = face.vertexIndices[i];
                        int vtIdx = (i < face.texCoordIndices.size()) ? face.texCoordIndices[i] : 0;
                        int vnIdx = (i < face.normalIndices.size()) ? face.normalIndices[i] : 0;

                        // 处理负索引（OBJ 格式支持相对索引，负索引表示从末尾开始）
                        size_t vAbsIdx = 0;
                        if (vIdx != 0) {
                            if (vIdx > 0) {
                                vAbsIdx = static_cast<size_t>(vIdx - 1);
                            } else {
                                // 负索引：-1 表示最后一个，-2 表示倒数第二个，以此类推
                                vAbsIdx = _model.vertices.size() + static_cast<size_t>(vIdx);
                            }
                        }

                        size_t vtAbsIdx = 0;
                        if (vtIdx != 0) {
                            if (vtIdx > 0) {
                                vtAbsIdx = static_cast<size_t>(vtIdx - 1);
                            } else {
                                vtAbsIdx = _model.texCoords.size() + static_cast<size_t>(vtIdx);
                            }
                        }

                        size_t vnAbsIdx = 0;
                        if (vnIdx != 0) {
                            if (vnIdx > 0) {
                                vnAbsIdx = static_cast<size_t>(vnIdx - 1);
                            } else {
                                vnAbsIdx = _model.normals.size() + static_cast<size_t>(vnIdx);
                            }
                        }

                        // 创建唯一键
                        std::string key = fmt::format("{}_{}_{}", vIdx, vtIdx, vnIdx);

                        uint32_t index;
                        if (indexMap.count(key) > 0) {
                            index = indexMap[key];
                        } else {
                            index = nextIndex++;
                            indexMap[key] = index;

                            // 添加顶点位置
                            if (vIdx != 0 && vAbsIdx < _model.vertices.size()) {
                                const ObjVertex& v = _model.vertices[vAbsIdx];
                                meshData.vertices.emplace_back(v.x, v.y, v.z);
                            } else {
                                meshData.vertices.emplace_back(0.0f, 0.0f, 0.0f);
                            }

                            // 添加纹理坐标
                            if (vtIdx != 0 && vtAbsIdx < _model.texCoords.size()) {
                                const ObjTexCoord& vt = _model.texCoords[vtAbsIdx];
                                meshData.texcoords.emplace_back(vt.u, vt.v);
                            } else {
                                meshData.texcoords.emplace_back(0.0f, 0.0f);
                            }

                            // 添加法线
                            if (vnIdx != 0 && vnAbsIdx < _model.normals.size()) {
                                const ObjNormal& vn = _model.normals[vnAbsIdx];
                                meshData.normals.emplace_back(vn.x, vn.y, vn.z);
                            } else {
                                meshData.normals.emplace_back(0.0f, 0.0f, 1.0f);
                            }

                            // 默认颜色
                            meshData.colors.emplace_back(1.0f, 1.0f, 1.0f, 1.0f);
                        }

                        meshData.indices.push_back(index);
                    }
                }

                if (!meshData.vertices.empty()) {
                    result.push_back(std::move(meshData));
                }
            }
        }

        return result;
    }

    std::vector<std::string_view> objLoader::splitString(const std::string_view& str, char delimiter)
    {
        std::vector<std::string_view> result;
        size_t start = 0;
        
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == delimiter) {
                if (i > start) {
                    result.push_back(str.substr(start, i - start));
                }
                start = i + 1;
            }
        }
        
        if (start < str.size()) {
            result.push_back(str.substr(start));
        }
        
        return result;
    }

    float objLoader::parseFloat(const std::string_view& str)
    {
        double value = 0.0;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
        if (ec == std::errc()) {
            return static_cast<float>(value);
        }
        return 0.0f;
    }

    int objLoader::parseInt(const std::string_view& str)
    {
        int64_t value = 0;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
        if (ec == std::errc()) {
            return static_cast<int>(value);
        }
        return 0;
    }

    void objLoader::trimWhitespace(std::string_view& str)
    {
        // 移除前导空白
        while (!str.empty() && (str[0] == ' ' || str[0] == '\t' || str[0] == '\r')) {
            str = str.substr(1);
        }
        
        // 移除尾随空白
        while (!str.empty() && (str.back() == ' ' || str.back() == '\t' || str.back() == '\r')) {
            str = str.substr(0, str.size() - 1);
        }
    }

    size_t objLoader::getMeshCount() const noexcept
    {
        if (!_loaded)
        {
            return 0;
        }

        size_t count = 0;
        for (const auto& group : _model.groups)
        {
            if (!group.faces.empty())
            {
                // 按材质分组，每个材质算一个网格
                std::unordered_set<int> materials;
                for (const auto& face : group.faces)
                {
                    int matIdx = (face.materialIndex >= 0) ? face.materialIndex : group.materialIndex;
                    materials.insert(matIdx);
                }
                count += materials.empty() ? 1 : materials.size();
            }
        }
        return count;
    }

}

