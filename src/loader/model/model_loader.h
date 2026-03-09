#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <string_view>
#include <algorithm>

#include "loader/core/loader.h"
#include "math/vector.ixx"
#include "math/vector2.h"
#include "math/rotator.h"

#include "math/mathFwd.h"

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
        std::vector<std::uint32_t> indices;
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
        using ProgressCallback = std::function<void(float, std::string_view)>;

        virtual ~IModelLoader() = default;

        /**
         * @brief 检查是否已加载
         * @return true 如果已加载
         */
        bool isLoaded() const noexcept  { return isLoader; }

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

        void setProgressCallback(ProgressCallback callback)
        {
            _progressCallback = std::move(callback);
        }

        void clearProgressCallback()
        {
            _progressCallback = nullptr;
        }

        bool isLoader = false;

    protected:
        void notifyProgress(float progress, std::string_view stage) const
        {
            if (_progressCallback)
            {
                _progressCallback(std::clamp(progress, 0.0f, 1.0f), stage);
            }
        }

    private:
        ProgressCallback _progressCallback;
    };
}

