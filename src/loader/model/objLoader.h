#pragma once

#include "loader/core/loader.h"
#include "loader/model/model_loader.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <cstring>
#include <sstream>

#include "math/vector.h"
#include "math/vector2.h"

namespace shine::loader
{

    class objLoader : public IModelLoader
    {

    public:
           
        objLoader() {
            addSupportedExtension("obj");
        }
    
        virtual ~objLoader() = default; 
        virtual bool loadFromMemory(const void* data, size_t size);
        virtual bool loadFromFile(const char* filePath);
        void unload();

        // 实现基类方法
        virtual const char* getName() const override { return "objLoader"; }
        virtual const char* getVersion() const override { return "1.0.0"; }

        // ========================================================================
        // IModelLoader 接口实现
        // ========================================================================

        bool isLoaded() const noexcept override { return _loaded; }
        std::vector<MeshData> extractMeshData() const override;
        size_t getMeshCount() const noexcept override;
        
        // 获取组/对象数量
        size_t getGroupCount() const { return _model.groups.size(); }
        
        // 获取材质数量
        size_t getMaterialCount() const { return _model.materials.size(); }

    private:
        // OBJ 文件格式结构
        struct ObjVertex {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            float w = 1.0f;  // 可选，默认1.0
        };

        struct ObjTexCoord {
            float u = 0.0f;
            float v = 0.0f;
            float w = 0.0f;  // 可选，默认0.0
        };

        struct ObjNormal {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
        };

        struct ObjFace {
            std::vector<int> vertexIndices;      // 顶点索引（从1开始，OBJ格式）
            std::vector<int> texCoordIndices;    // 纹理坐标索引（从1开始）
            std::vector<int> normalIndices;      // 法线索引（从1开始）
            int materialIndex = -1;              // 材质索引
        };

        struct ObjGroup {
            std::string name;
            std::vector<ObjFace> faces;
            int materialIndex = -1;
        };

        struct ObjMaterial {
            std::string name;
            float ambient[3] = {0.2f, 0.2f, 0.2f};
            float diffuse[3] = {0.8f, 0.8f, 0.8f};
            float specular[3] = {0.0f, 0.0f, 0.0f};
            float emissive[3] = {0.0f, 0.0f, 0.0f};
            float shininess = 0.0f;
            float transparency = 1.0f;
            float refraction = 1.0f;
            float dissolve = 1.0f;
            int illuminationModel = 0;
            
            // 纹理路径
            std::string ambientMap;
            std::string diffuseMap;
            std::string specularMap;
            std::string normalMap;
            std::string bumpMap;
        };

        struct ObjModel {
            std::vector<ObjVertex> vertices;
            std::vector<ObjTexCoord> texCoords;
            std::vector<ObjNormal> normals;
            std::vector<ObjGroup> groups;
            std::unordered_map<std::string, ObjMaterial> materials;
            std::unordered_map<std::string, int> materialNameToIndex;  // 材质名称到索引的稳定映射
            std::string mtlLibPath;  // 材质库文件路径
        };

        // 内部方法
        bool parseOBJ(const char* data, size_t size, const char* basePath = nullptr);
        bool parseMTL(const char* filePath);
        void parseLine(const std::string_view& line, const char* basePath = nullptr);
        void parseFace(const std::string_view& line);
        void parseVertex(const std::string_view& line);
        void parseTexCoord(const std::string_view& line);
        void parseNormal(const std::string_view& line);
        void parseGroup(const std::string_view& line);
        void parseUseMaterial(const std::string_view& line);
        void parseMaterialLib(const std::string_view& line, const char* basePath);
        
        // 辅助函数
        std::vector<std::string_view> splitString(const std::string_view& str, char delimiter);
        float parseFloat(const std::string_view& str);
        int parseInt(const std::string_view& str);
        void trimWhitespace(std::string_view& str);
        
        // 数据成员
        ObjModel _model;
        bool _loaded = false;
        std::string _basePath;  // OBJ 文件的基础路径（用于加载 MTL 文件）
        int _currentMaterialIndex = -1;  // 当前使用的材质索引
    };

}

