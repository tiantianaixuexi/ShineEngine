#pragma once


#include "fmt/format.h"
#include "shine_define.h"

#include <string>
#include <vector>
#include <expected>








namespace shine::model
{

    struct objTexture
    {
        std::string name;
        std::string path;

        objTexture() {
			name = "";
            path = "";
        }
    };

    // Obj 材质信息结构体
    struct EXPORT_OBJ objMaterial
    {
        objMaterial() = default;

        std::string name;          // 材质名称

		float       Ambient[3];            // 环境遮挡 AO
		float       Diffuse[3];            // 漫反射颜色
		float       Specular[3];           // 高光反射颜色
		float       Emissive[3];           // 自发光颜色
		float       Shininess;             // 光泽度
		float       Transparency;          // 透明度
		float       Refraction;            // 折射率
		float       TransmissionFilter[3]; // 透射滤镜颜色
		float       Dissolve;              // 溶解度（透明度）
		int         IlluminationModel;     // 照明模型

		bool        isDefault;             // 是否为默认材质或者没有mtlib文件


        unsigned int         AmbientMapIndex;       // 环境贴图索引
        unsigned int         DiffuseMapIndex;       // 漫反射贴图索引
        unsigned int         SpecularMapIndex;      // 高光反射贴图索引
        unsigned int         EmissiveMapIndex;      // 自发光贴图索引
        unsigned int         ShininessMapIndex;     // 光泽度贴图索引
        unsigned int         TransparencyMapIndex;  // 透明度贴图索引
        unsigned int         RefractionMapIndex;    // 折射率贴图索引
        unsigned int         DissolveMapIndex;      // 溶解度贴图索引
        unsigned int         BumpMapIndex;          // 凹凸贴图索引


    };

    struct ObjIndex
    {
        u64 vertexIndex;    // 顶点索引
        u64 normalIndex;    // 法线索引
        u64 texCoordIndex;  // 纹理坐标索引

        ObjIndex() {
            vertexIndex = 0;
            normalIndex = 0;
            texCoordIndex = 0;
        }
    };

    struct EXPORT_OBJ ObjGroup
    {
		std::string name;              // 组名称

		u32 face_count;                // 面数量
		u32 face_offset;               // 面偏移
		u32 index_offset;              // 索引偏移

        ObjGroup() {
            name = "";
            face_count = 0;
            face_offset = 0;
            index_offset = 0;
        }
    };


    /* 注意：
    在positions、texcoords、normals 和 textures 数组的第一个位置添加了一个虚拟初始化的值。
    因此，有效的数组索引从1 开始，索引为0 表示该属性不存在
    */
    struct EXPORT_OBJ ObjMesh
    {
		u32  position_count;   // 顶点数量
        std::vector<f32> positions;    // 顶点坐标数组

		u32  texcoord_count;  // 纹理坐标数量
        std::vector<f32> texcoords;    // 纹理坐标数组

		u32  normal_count;    // 法线数量
		std::vector<f32> normals;      // 法线数组

		u32  color_count;    // 颜色数量
        std::vector<f32> colors;      // 颜色数组

		u32  face_count;     // 面数量
        std::vector<u32> face_vertices;  // 面顶点索引数组
        std::vector<u32> face_materials; // 面材质索引数组
        std::vector<u8>  face_lines;      // 面线索引数组

		u32  index_count;    // 索引数量
		ObjIndex indices;	// 索引数组

		u32 material_count; // 材质数量
        std::vector<objMaterial> materials; // 材质数组

		u32 texture_count; // 纹理数量
        std::vector<objTexture> textures; // 纹理数组

		u32 object_count; // 对象数量
        std::vector<ObjGroup> objects; // 对象数组

		u32 group_count; // 组数量
        std::vector<ObjGroup> groups; // 组数组

        ObjMesh() {
            position_count = 0;
            positions = {};
            texcoord_count = 0;
            texcoords = {};
            normal_count = 0;
            normals = {};
            color_count = 0;
            colors = {};
            face_count = 0;
            face_vertices = {};
            face_materials = {};
            face_lines = {};
            index_count = 0;
            indices = {};
            material_count = 0;
            materials = {};
            texture_count = 0;
            textures = {};
            object_count = 0;
            objects = {};
            group_count = 0;
        }
    };

    struct EXPORT_OBJ ObjData
    {
        ObjMesh* mesh;

        ObjGroup object;
		ObjGroup group;

        u32 material;   //当前材质索引
		u32 line;       //当前行索引

        // Base path for materials/textures
        std::string base;

    };


    /**
     * @brief OBJ 文件格式插件类
     *
     * 支持 .obj 格式的3D模型文件处理
     */
    class EXPORT_OBJ obj
    {
    public:
        /**
         * @brief 构造函数
         */
         obj(){};


         ObjMesh* mesh = nullptr;
         ObjData data  = {};


        /**
         * @brief 加载文件
         * @param filePath 文件路径
         * @return 成功返回ObjMesh指针
         */
        std::expected<ObjMesh*,std::string> parseObjFile(std::string_view filePath);


        bool isWhitespace(char c);
        bool isDigit(char c);
        void parseLine(std::string_view line);
        void parseMtlFile(std::string_view line);
    };
}

