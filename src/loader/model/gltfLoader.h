#pragma once

#include "loader/core/loader.h"
#include "loader/model/model_loader.h"
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

    class  gltfLoader : public IModelLoader
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

            // ========================================================================
            // IModelLoader 接口实现
            // ========================================================================

            bool isLoaded() const noexcept override { return _loaded; }
            std::vector<MeshData> extractMeshData() const override;
            size_t getMeshCount() const noexcept override;
            
            // 获取场景数量
            size_t getSceneCount() const { return _model.scenes.size(); }
            
            // 获取默认场景索引
            int getDefaultSceneIndex() const { return _model.scene; }
            
            // 获取场景的根节点索引列表
            std::vector<int> getSceneRootNodes(int sceneIndex) const;

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

            struct Scene {
                std::string name;
                std::vector<int> nodes;  // node indices
            };

            struct GltfModel {
                std::string version;
                std::vector<Buffer> buffers;
                std::vector<BufferView> bufferViews;
                std::vector<Accessor> accessors;
                std::vector<Mesh> meshes;
                std::vector<Node> nodes;
                std::vector<Scene> scenes;
                int scene = -1;  // default scene index
            };

            // 内部方法
            bool parseGLB(const void* data, size_t size);
            bool parseGLTF(const char* filePath);  // 解析纯 JSON GLTF 文件
            bool parseJSONChunk(const char* jsonData, size_t jsonSize);
            bool parseBinaryChunk(const std::byte* binData, size_t binSize);
            bool loadDataURIBuffer(const Buffer& buffer, size_t bufferIndex);
            bool loadExternalBuffer(const Buffer& buffer, size_t bufferIndex, const char* basePath);
            std::vector<std::byte> getAccessorData(const Accessor& accessor) const;
            
            // 模板函数：从 accessor 读取特定类型的数据（优化版本，避免不必要的拷贝）
            template<typename T> 
            std::vector<T> readAccessorAs(const Accessor& accessor) const {
                std::vector<T> result;
                
                if (accessor.bufferView < 0 || 
                    static_cast<size_t>(accessor.bufferView) >= _model.bufferViews.size()) {
                    return result;
                }

                const BufferView& bufferView = _model.bufferViews[accessor.bufferView];
                
                if (bufferView.buffer < 0 || 
                    static_cast<size_t>(bufferView.buffer) >= _model.buffers.size()) {
                    return result;
                }

                // 计算组件大小
                size_t componentSize = 0;
                switch (accessor.componentType) {
                    case 5120: case 5121: componentSize = 1; break;  // BYTE, UNSIGNED_BYTE
                    case 5122: case 5123: componentSize = 2; break;  // SHORT, UNSIGNED_SHORT
                    case 5125: componentSize = 4; break;  // UNSIGNED_INT
                    case 5126: componentSize = 4; break;  // FLOAT
                    default: return result;
                }

                // 计算每个元素的组件数量
                size_t componentsPerElement = 1;
                if (accessor.type == "VEC2") componentsPerElement = 2;
                else if (accessor.type == "VEC3") componentsPerElement = 3;
                else if (accessor.type == "VEC4") componentsPerElement = 4;
                else if (accessor.type == "SCALAR") componentsPerElement = 1;
                else if (accessor.type == "MAT2") componentsPerElement = 4;
                else if (accessor.type == "MAT3") componentsPerElement = 9;
                else if (accessor.type == "MAT4") componentsPerElement = 16;

                size_t elementSize = componentSize * componentsPerElement;
                size_t stride = (bufferView.byteStride > 0) ? bufferView.byteStride : elementSize;
                
                // 计算数据偏移
                size_t dataOffset = bufferView.byteOffset + accessor.byteOffset;
                
                // 获取缓冲区数据
                const std::vector<std::byte>& bufferData = getBufferData(bufferView.buffer);
                
                // 边界检查
                size_t totalSize = (accessor.count - 1) * stride + elementSize;
                if (dataOffset + totalSize > bufferData.size()) {
                    return result;
                }

                // 预分配结果内存
                if constexpr (std::is_same_v<T, float>) {
                    result.reserve(accessor.count * componentsPerElement);
                } else {
                    result.reserve(accessor.count);
                }

                // 直接读取数据，避免中间拷贝
                const std::byte* ptr = bufferData.data() + dataOffset;
                
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
                        } else if (accessor.componentType == 5121) { // UNSIGNED_BYTE
                            uint8_t value = 0;
                            std::memcpy(&value, ptr, sizeof(uint8_t));
                            result.push_back(static_cast<uint32_t>(value));
                        }
                    } else if constexpr (std::is_same_v<T, uint8_t>) {
                        if (accessor.componentType == 5121) { // UNSIGNED_BYTE
                            for (size_t j = 0; j < componentsPerElement; ++j) {
                                uint8_t value = 0;
                                std::memcpy(&value, ptr + j * sizeof(uint8_t), sizeof(uint8_t));
                                result.push_back(value);
                            }
                        }
                    }
                    ptr += stride;
                }

                return result;
            }

            // 数据成员
            GltfModel _model;
            std::vector<std::vector<std::byte>> _bufferData;  // 多个缓冲区的数据
            bool _loaded = false;
            
            // 获取指定缓冲区的数据
            const std::vector<std::byte>& getBufferData(int bufferIndex) const;



    public:

        const GltfModel& getModel() const { return _model; }
    };

 
}

