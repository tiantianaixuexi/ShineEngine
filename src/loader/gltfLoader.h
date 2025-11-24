#pragma once

#include "loader.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <cstring>
#include <type_traits>

#include "math/vector.h"
#include "math/vector2.h"

namespace shine::loader
{

    class  gltfLoader : public IAssetLoader
    {

        public:
           
            gltfLoader() {
                    addSupportedExtension("gltf");
                    addSupportedExtension("glb");
            }
        
            virtual ~gltfLoader() = default; 
            virtual  bool loadFromMemory(const void* data, size_t size) ;
            virtual  bool loadFromFile(const char* filePath) ;
            void unload() ;

            // 实现基类方法
            virtual const char* getName() const override { return "gltfLoader"; }
            virtual const char* getVersion() const override { return "1.0.0"; }

            // 数据访问方法
            bool isLoaded() const { return _loaded; }

            
            // 提取网格数据（转换为项目内部格式）
            struct MeshData {
                std::string name;
                std::vector<math::FVector3f> vertices;
                std::vector<math::FVector3f> normals;
                std::vector<math::FVector2f> texcoords;
                std::vector<uint32_t> indices;
                math::FVector3f translation{0.0f};
                math::FRotator3f rotation{0.0f, 0.0f, 0.0f};
                math::FVector3f scale{1.0f};
            };
            
            std::vector<MeshData> extractMeshData() const;

        private:
            // GLB 文件格式结构
            struct GLBHeader {
                uint32_t magic;      // 0x46546C67 ("glTF")
                uint32_t version;     // 版本号
                uint32_t length;      // 文件总长度
            };

            struct GLBChunk {
                uint32_t length;     // chunk 长度
                uint32_t type;       // chunk 类型 (0x4E4F534A = "JSON", 0x004E4942 = "BIN\0")
                std::vector<std::byte> data;
            };

            // glTF 数据结构
            struct BufferView {
                int buffer = -1;
                size_t byteOffset = 0;
                size_t byteLength = 0;
                int byteStride = -1;
                int target = -1;  // ARRAY_BUFFER = 34962, ELEMENT_ARRAY_BUFFER = 34963
            };

            struct Accessor {
                int bufferView = -1;
                size_t byteOffset = 0;
                int componentType = -1;  // 5120=BYTE, 5121=UNSIGNED_BYTE, 5122=SHORT, 5123=UNSIGNED_SHORT, 5125=UNSIGNED_INT, 5126=FLOAT
                bool normalized = false;
                size_t count = 0;
                std::string type;  // "SCALAR", "VEC2", "VEC3", "VEC4", "MAT2", "MAT3", "MAT4"
            };

            struct Primitive {
                std::unordered_map<std::string, int> attributes;  // "POSITION", "NORMAL", "TEXCOORD_0" -> accessor index
                int indices = -1;
                int material = -1;
                int mode = 4;  // 4 = TRIANGLES
            };

            struct Mesh {
                std::string name;
                std::vector<Primitive> primitives;
            };

            struct Node {
                std::string name;
                int mesh = -1;
                std::vector<float> translation;  // [x, y, z]
                std::vector<float> rotation;      // [x, y, z, w] quaternion
                std::vector<float> scale;         // [x, y, z]
                std::vector<int> children;
            };

            struct Buffer {
                size_t byteLength = 0;
                std::string uri;  // 对于 GLB，为空字符串
            };

            struct GltfModel {
                std::string version;
                std::vector<Buffer> buffers;
                std::vector<BufferView> bufferViews;
                std::vector<Accessor> accessors;
                std::vector<Mesh> meshes;
                std::vector<Node> nodes;
            };

            // 内部方法
            bool parseGLB(const void* data, size_t size);
            bool parseJSONChunk(const char* jsonData, size_t jsonSize);
            bool parseBinaryChunk(const std::byte* binData, size_t binSize);
            std::vector<std::byte> getAccessorData(const Accessor& accessor) const;
            
            // 模板函数：从 accessor 读取特定类型的数据
            template<typename T> 
            std::vector<T> readAccessorAs(const Accessor& accessor) const {
                std::vector<T> result;
                auto data = getAccessorData(accessor);
                if (data.empty()) return result;

                size_t componentsPerElement = 1;
                if (accessor.type == "VEC2") componentsPerElement = 2;
                else if (accessor.type == "VEC3") componentsPerElement = 3;
                else if (accessor.type == "VEC4") componentsPerElement = 4;

                const BufferView& bufferView = _model.bufferViews[accessor.bufferView];
                size_t stride = (bufferView.byteStride > 0) 
                    ? bufferView.byteStride 
                    : sizeof(T) * componentsPerElement;

                const std::byte* ptr = data.data();
                for (size_t i = 0; i < accessor.count; ++i) {
                    if constexpr (std::is_same_v<T, float>) {
                        if (accessor.componentType == 5126) { // FLOAT
                            for (size_t j = 0; j < componentsPerElement; ++j) {
                                float value = 0;
                                std::memcpy(&value, ptr + j * sizeof(float), sizeof(float));
                                result.push_back(value);
                            }
                        }
                    } else if constexpr (std::is_same_v<T, uint32_t>) {
                        if (accessor.componentType == 5125) { // UNSIGNED_INT
                            uint32_t value = 0;
                            std::memcpy(&value, ptr, sizeof(uint32_t));
                            result.push_back(value);
                        } else if (accessor.componentType == 5123) { // UNSIGNED_SHORT
                            uint16_t value = 0;
                            std::memcpy(&value, ptr, sizeof(uint16_t));
                            result.push_back(static_cast<uint32_t>(value));
                        }
                    }
                    ptr += stride;
                }

                return result;
            }

            // 数据成员
            GltfModel _model;
            std::vector<std::byte> _binaryData;  // BIN chunk 数据
            bool _loaded = false;



    public:

        const GltfModel& getModel() const { return _model; }
    };

 
}

