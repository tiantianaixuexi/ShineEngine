#include "gltfLoader.h"

#include <map>
#include <vector>
#include <string>


#include "fmt/format.h"

#include "math/vector.h"
#include "math/vector2.h"
#include "math/rotator.h"
#include "math/quat.h"

#include "util/timer/function_timer.h"
// #include "tinygltf/tiny_gltf.h" // Temporarily disabled


namespace shine::loader
{
    using namespace shine::math;

    bool gltfLoader::loadFromMemory(const void *data, size_t size)
    {
        return false;
    }

    bool gltfLoader::loadFromFile(const char *filePath)
    {
        util::FunctionTimer timer("gltfLoader::loadFromFile");

        // Temporarily disabled tinygltf loading
        fmt::println("glTF loading temporarily disabled - tinygltf JSON dependency issue");
        return false;

        /*
        tinygltf::TinyGLTF gltfload;
        tinygltf::Model* model = new tinygltf::Model();
        std::string err;
        std::string warn;


        auto result = gltfload.LoadBinaryFromFile(model,&err,&warn,filePath);
        if (!result)
        {
            fmt::println("Failed loading Gltf from file, err: {} , warn: {}", err, warn);
            return false;
        }

        const auto gltf_model = *model;
        int nodeIndex = -1;
        FVector3f translation(0.0);
        FRotator3f rotation(0.0, 0.0, 0.0);
        FVector3f scale(1.0);

        std::map<int,std::string> _meshNames;
        std::map<int,std::vector<FVector>>   s_vertices;
        std::map<int,std::vector<FVector>>   s_normals;
        std::map<int,std::vector<FVector2>>   s_texcoords;
        std::map<int,std::vector<unsigned int>> s_indices;
        std::map<int,std::tuple<FVector3f, FRotator3f, FVector3f>> _transforms;

        for (const auto& node : gltf_model.nodes)
        {
            if (!node.translation.empty())
            {
                translation = FVector3f(node.translation[0],node.translation[1],node.translation[2]);
            }

            if (!node.rotation.empty())
            {
            	const auto& quaternion = FQuatf(node.rotation[3],node.rotation[0],node.rotation[1],node.rotation[2]);
                rotation = quaternion.eulerAngles();
            }

            if (!node.scale.empty())
            {
                scale = FVector3f(node.scale[0],node.scale[1],node.scale[2]);
            }

            ++nodeIndex;


            const auto& mesh =gltf_model.meshes[node.mesh];
            int meshIndex = nodeIndex;
            _meshNames.emplace(meshIndex,mesh.name);
            _transforms.emplace(meshIndex,std::make_tuple(translation,rotation,scale));

            for (const auto& primitive : mesh.primitives)
            {
                
                if (primitive.attributes.contains("POSITION"))
                {
                    const auto& positionAccessor = gltf_model.accessors[primitive.attributes.at("POSITION")];
                    const auto& positionBufferView = gltf_model.bufferViews[positionAccessor.bufferView];
                    const auto& positionBuffer = gltf_model.buffers[positionBufferView.buffer];

                    std::vector<FVector> _vertices;
                    const int* vertices = reinterpret_cast<const int*>(&positionBuffer.data[positionBufferView.byteOffset + positionAccessor.byteOffset]);
                    for (size_t i =0;i<positionAccessor.count;++i)
                    {
                        _vertices.push_back({vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]});
                    }
                    s_vertices.emplace(meshIndex,_vertices);
                }

                if (primitive.attributes.contains("NORMAL"))
                {
                    const auto& normalAccessor   = gltf_model.accessors[primitive.attributes.at("NORMAL")];
                    const auto& normalBufferView = gltf_model.bufferViews[normalAccessor.bufferView];
                    const auto& normalBuffer = gltf_model.buffers[normalBufferView.buffer];

                    std::vector<FVector> _normals;
                    const int* normals = reinterpret_cast<const int*>(&normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset]);
                    for (size_t i =0;i<normalAccessor.count;++i)
                    {
                        _normals.push_back({normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]});
                    }

                    s_normals.emplace(meshIndex,_normals);
                }

                // UV
                if (primitive.attributes.contains("TEXCOORD_0"))
                {
                    const auto& texcoordAccessor = gltf_model.accessors[primitive.attributes.at("TEXCOORD_0")];
                    const auto& texcoordBufferView = gltf_model.bufferViews[texcoordAccessor.bufferView];
                    const auto& texcoordBuffer = gltf_model.buffers[texcoordBufferView.buffer];


                    std::vector<FVector2> _texcoords;
                    const int* texcoords = reinterpret_cast<const int*>(&texcoordBuffer.data[texcoordBufferView.byteOffset+texcoordAccessor.byteOffset]);
                    for (size_t i =0;i<texcoordAccessor.count;++i)
                    {
                        _texcoords.push_back({ texcoords[i*2], texcoords[i*2+1] });
                    }
                    s_texcoords.emplace(meshIndex,_texcoords);
                }


                if (primitive.indices>= 0 )
                {
                    const auto& indexAccessor = gltf_model.accessors[primitive.indices];
                    const auto& indexBufferView = gltf_model.bufferViews[indexAccessor.bufferView];
                    const auto& indexBuffer = gltf_model.buffers[indexBufferView.buffer];

                    std::vector<unsigned int> _indices;
                    if (indexAccessor.componentType == 5123)
                    {
                        const unsigned short* indices = reinterpret_cast<const unsigned short*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
                        for (size_t i =0;i<indexAccessor.count;++i)
                        {
                            _indices.push_back(indices[i]);
                            
                        }
                    }else if (indexAccessor.componentType == 5125)
                    {
                        const unsigned int* indices = reinterpret_cast<const unsigned int*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
                        for (size_t i = 0; i < indexAccessor.count; ++i)
                            _indices.push_back(indices[i]);
                    }
                    s_indices.emplace(meshIndex,_indices);
                }

                if (primitive.material >=0 && primitive.material < gltf_model.materials.size())
                {
                    tinygltf::Material _material;
 
                    const auto& material = gltf_model.materials[primitive.material];
                    fmt::println("material name : {}", material.name);


                    // get maeterial base color texture index;
                    const int textIndex = material.pbrMetallicRoughness.baseColorTexture.index;
                    if (textIndex != -1)
                    {
                        fmt::println("material {}  base color texture index : {}",material.name, textIndex);
                    }
                }
            }

            const int texture_num = gltf_model.textures.size();
            fmt::println("texture num : {}", texture_num);

            for (size_t i = 0;i<texture_num;++i)
            {
                tinygltf::Texture const & texture = gltf_model.textures[i];
                fmt::println("texture name : {}", texture.name);
                fmt::println("texture source:{}", texture.source);
            }
        }

        return nullptr; // model
        */
        return false;
    }
    
    void gltfLoader::unload()
    {
    }
    
}


