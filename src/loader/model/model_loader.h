#pragma once

#include "loader/core/loader.h"
#include <vector>
#include <string>
#include "math/vector.h"
#include "math/vector2.h"
#include "math/rotator.h"

namespace shine::loader
{
    /**
     * @brief 顶点颜色结构（RGBA）
     */
    struct VertexColor {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
        
        VertexColor() = default;
        VertexColor(float _r, float _g, float _b, float _a = 1.0f) 
            : r(_r), g(_g), b(_b), a(_a) {}
    };

    /**
     * @brief 网格数据（转换为项目内部格式）
     */
    struct MeshData {
        std::string name;
        std::vector<math::FVector3f> vertices;
        std::vector<math::FVector3f> normals;
        std::vector<math::FVector2f> texcoords;
        std::vector<VertexColor> colors;  // 顶点颜色（如果存在）
        std::vector<uint32_t> indices;
        math::FVector3f translation{0.0f};
        math::FRotator3f rotation{0.0f, 0.0f, 0.0f};
        math::FVector3f scale{1.0f};
        int materialIndex = -1;  // 材质索引
    };

    /**
     * @brief 模型加载器接口 - 统一模型格式加载器的抽象接口
     * 继承自 IAssetLoader，提供模型特定的接口
     */
    class IModelLoader : public IAssetLoader
    {
    public:
        virtual ~IModelLoader() = default;

        /**
         * @brief 检查是否已加载
         * @return true 如果已加载
         */
        virtual bool isLoaded() const noexcept = 0;

        /**
         * @brief 提取网格数据（转换为项目内部格式）
         * @return 网格数据向量
         */
        virtual std::vector<MeshData> extractMeshData() const = 0;

        /**
         * @brief 获取网格数量
         * @return 网格数量
         */
        virtual size_t getMeshCount() const noexcept = 0;
    };
}

