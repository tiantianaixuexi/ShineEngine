# tinygltf – Header-only glTF 2.0 Loader and Serializer


```cpp
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION   // 如果不使用图像加载，可以省略
#define STB_IMAGE_WRITE_IMPLEMENTATION // 如果不使用图像写入，可以省略
#include "tiny_gltf.h"
```

如果需要禁用某些依赖（如文件系统、stb_image），可以在包含头文件前定义相应的宏（详见源码注释）。

## 2. 核心数据结构

库使用一系列结构体来描述 glTF 的各个组件，它们与规范中的对象一一对应。所有结构体都提供了默认构造函数和移动语义，并重载了 `operator==` 用于比较。

| 结构体 | 描述 |
|--------|------|
| `Model` | 根容器，包含所有 glTF 元素（场景、节点、网格、材质等）。 |
| `Asset` | 资产信息（版本、生成器、版权等）。 |
| `Scene` | 场景，包含节点索引列表。 |
| `Node` | 节点，包含变换（TRS 或矩阵）、子节点、网格、相机、皮肤等引用。 |
| `Mesh` | 网格，由多个 `Primitive` 组成。 |
| `Primitive` | 绘制单元，包含属性字典（如 `POSITION`、`NORMAL`）、索引、材质、绘制模式。 |
| `Accessor` | 访问器，描述缓冲区视图中的数据布局（类型、分量、归一化等）。 |
| `BufferView` | 缓冲区视图，描述一段连续的数据块。 |
| `Buffer` | 缓冲区，存储原始二进制数据。 |
| `Image` | 图像，可引用缓冲区视图或外部 URI。 |
| `Texture` | 纹理，关联图像和采样器。 |
| `Sampler` | 采样器，定义纹理过滤和包裹模式。 |
| `Material` | 材质，包含 PBR 参数、纹理引用等。 |
| `Animation` | 动画，包含通道和采样器。 |
| `Skin` | 蒙皮，定义关节矩阵和骨架。 |
| `Camera` | 相机（透视或正交）。 |
| `Light` | 光源（KHR_lights_punctual 扩展）。 |
| `AudioEmitter` / `AudioSource` | 音频相关（KHR_audio 扩展）。 |
| `Value` | 通用 JSON 值，用于解析 `extras` 和扩展中的任意数据。 |

## 3. 加载器类 `TinyGLTF`

`TinyGLTF` 类提供了加载和保存 glTF 资产的主要接口。

### 3.1 构造函数与设置

```cpp
class TinyGLTF {
public:
    TinyGLTF();
    ~TinyGLTF();

    // 设置解析严格程度（宽松或严格）
    void SetParseStrictness(ParseStrictness strictness);

    // 设置是否序列化默认值（默认为 false）
    void SetSerializeDefaultValues(bool enabled);

    // 设置是否保存 extras/extensions 的原始 JSON 字符串
    void SetStoreOriginalJSONForExtrasAndExtensions(bool enabled);

    // 设置是否保留图像通道（加载图像时）
    void SetPreserveImageChannels(bool onoff);

    // 设置是否以原始编码加载图像（不解码）
    void SetImagesAsIs(bool onoff);

    // 设置最大允许的外部文件大小（默认 2GB）
    void SetMaxExternalFileSize(size_t max_bytes);

    // 设置图像加载回调（替换默认的 stb_image 加载）
    void SetImageLoader(LoadImageDataFunction func, void* user_data);
    void RemoveImageLoader();

    // 设置图像写入回调（用于序列化）
    void SetImageWriter(WriteImageDataFunction func, void* user_data);

    // 设置 URI 编码/解码回调
    bool SetURICallbacks(URICallbacks callbacks, std::string* err = nullptr);

    // 设置文件系统回调（默认使用 std::ifstream 等）
    bool SetFsCallbacks(FsCallbacks callbacks, std::string* err = nullptr);
};
```

### 3.2 加载方法

```cpp
// 从 ASCII 文件加载
bool LoadASCIIFromFile(Model* model, std::string* err, std::string* warn,
                       const std::string& filename,
                       unsigned int check_sections = REQUIRE_VERSION);

// 从 ASCII 字符串加载（内存）
bool LoadASCIIFromString(Model* model, std::string* err, std::string* warn,
                         const char* str, unsigned int length,
                         const std::string& base_dir,
                         unsigned int check_sections = REQUIRE_VERSION);

// 从二进制文件加载（.glb）
bool LoadBinaryFromFile(Model* model, std::string* err, std::string* warn,
                        const std::string& filename,
                        unsigned int check_sections = REQUIRE_VERSION);

// 从二进制内存加载（.glb）
bool LoadBinaryFromMemory(Model* model, std::string* err, std::string* warn,
                          const unsigned char* bytes, unsigned int length,
                          const std::string& base_dir = "",
                          unsigned int check_sections = REQUIRE_VERSION);
```

参数说明：
- `model`：输出，解析后的数据将填充到此对象。
- `err` / `warn`：错误和警告信息的输出字符串。
- `filename` / `str` / `bytes`：输入源。
- `base_dir`：用于解析外部资源（如纹理、缓冲区）的基目录。
- `check_sections`：位掩码，指定必须存在的顶级节（如 `REQUIRE_VERSION`、`REQUIRE_SCENE` 等），默认检查版本。

### 3.3 序列化方法

```cpp
// 将模型写入流（内存），可指定是否美化输出、是否写入二进制格式
bool WriteGltfSceneToStream(const Model* model, std::ostream& stream,
                            bool prettyPrint, bool writeBinary);

// 将模型写入文件
bool WriteGltfSceneToFile(const Model* model, const std::string& filename,
                          bool embedImages, bool embedBuffers,
                          bool prettyPrint, bool writeBinary);
```

序列化时，可以选择将图像和缓冲区嵌入为 Data URI 或写入外部文件。

## 4. 回调定制

库允许通过回调完全控制文件访问和资源处理，这对于嵌入式系统或特殊存储需求非常有用。

### 4.1 文件系统回调 `FsCallbacks`

```cpp
struct FsCallbacks {
    FileExistsFunction FileExists;                 // 检查文件是否存在
    ExpandFilePathFunction ExpandFilePath;         // 扩展路径（如展开 ~）
    ReadWholeFileFunction ReadWholeFile;           // 读取整个文件
    WriteWholeFileFunction WriteWholeFile;         // 写入整个文件
    GetFileSizeFunction GetFileSizeInBytes;        // 获取文件大小
    void* user_data;                               // 传递给回调的用户数据
};
```

默认实现使用 C++ 标准库文件操作，但用户可完全替换。

### 4.2 URI 回调 `URICallbacks`

```cpp
struct URICallbacks {
    URIEncodeFunction encode;   // 可选，URI 编码（用于序列化时生成最终 URI）
    URIDecodeFunction decode;   // 必需，URI 解码（用于将 URI 转换为文件路径）
    void* user_data;
};
```

默认解码函数执行百分号解码（如 `%20` → 空格）。

### 4.3 图像加载回调

```cpp
using LoadImageDataFunction = std::function<bool(
    Image* image, int image_idx, std::string* err, std::string* warn,
    int req_width, int req_height, const unsigned char* bytes, int size,
    void* user_data)>;
```

默认实现使用 `stb_image` 解码图像，并可根据设置保留通道或原样存储。

### 4.4 图像写入回调

```cpp
using WriteImageDataFunction = std::function<bool(
    const std::string* basepath, const std::string* filename,
    const Image* image, bool embedImages, const FsCallbacks* fs_cb,
    const URICallbacks* uri_cb, std::string* out_uri, void* user_data)>;
```

默认实现使用 `stb_image_write` 将图像编码为 PNG/JPEG，并处理嵌入或写入文件。

## 5. 使用示例

### 5.1 加载 ASCII glTF 文件

```cpp
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf.h"

int main() {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, "model.gltf");
    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }
    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }
    if (!ret) {
        printf("Failed to parse glTF\n");
        return -1;
    }

    // 访问数据
    for (const auto& mesh : model.meshes) {
        for (const auto& prim : mesh.primitives) {
            // 处理 primitive...
        }
    }
    return 0;
}
```

### 5.2 加载二进制 glTF 文件

```cpp
bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, "model.glb");
```

### 5.3 自定义文件系统回调（从内存加载）

```cpp
// 假设我们有一个内存中的 glTF 字符串和资源映射
std::string gltf_json = "...";
std::map<std::string, std::vector<unsigned char>> resources;

// 实现自定义读取回调
bool MyReadWholeFile(std::vector<unsigned char>* out, std::string* err,
                     const std::string& filepath, void* user_data) {
    auto it = resources.find(filepath);
    if (it != resources.end()) {
        *out = it->second;
        return true;
    }
    *err = "File not found in memory";
    return false;
}

// 设置回调
tinygltf::FsCallbacks fs_callbacks{};
fs_callbacks.ReadWholeFile = MyReadWholeFile;
fs_callbacks.FileExists = [](const std::string&, void*) { return true; }; // 总是返回存在
fs_callbacks.ExpandFilePath = [](const std::string& path, void*) { return path; };
fs_callbacks.GetFileSizeInBytes = [](size_t* sz, std::string*, const std::string& p, void*) { *sz = resources[p].size(); return true; };
fs_callbacks.WriteWholeFile = nullptr; // 不需要
loader.SetFsCallbacks(fs_callbacks, nullptr);

// 加载
loader.LoadASCIIFromString(&model, &err, &warn, gltf_json.c_str(),
                           gltf_json.size(), "" /* base_dir 此时无用 */);
```

### 5.4 保存模型

```cpp
// 将模型写入文件，不嵌入图像和缓冲区，美化输出，ASCII 格式
loader.WriteGltfSceneToFile(&model, "output.gltf", false, false, true, false);
```

## 6. 注意事项

- **实现宏**：必须在一个 `.cpp` 文件中定义 `TINYGLTF_IMPLEMENTATION` 和其他实现宏，否则会导致链接错误。
- **线程安全**：默认的 RapidJSON 分配器不是线程安全的，如果需要并发解析多个文档，应定义 `TINYGLTF_USE_RAPIDJSON_CRTALLOCATOR` 使用 CRT 分配器。
- **文件大小限制**：默认最大外部文件大小为 2GB，可通过 `SetMaxExternalFileSize` 调整。
- **图像通道**：默认情况下图像会被扩展为 RGBA（4 通道），可通过 `SetPreserveImageChannels(true)` 保持原始通道。
- **二进制 glTF 填充**：GLB 格式要求 JSON 和 BIN 块按 4 字节对齐，库在解析时会检查并对齐填充，写入时也会自动填充。

## 7. API 参考摘要

### 主要类型

- `tinygltf::Model`
- `tinygltf::TinyGLTF`
- `tinygltf::FsCallbacks`
- `tinygltf::URICallbacks`
- `tinygltf::LoadImageDataFunction`
- `tinygltf::WriteImageDataFunction`

### 关键宏

- `TINYGLTF_IMPLEMENTATION` – 实例化库实现。
- `TINYGLTF_NO_FS` – 禁用默认文件系统回调（需自定义）。
- `TINYGLTF_NOEXCEPTION`：禁用 JSON 解析中的 C++ 异常。您可以使用-fno-exceptions`or` 或定义符号 ` JSON_NOEXCEPTIONand`TINYGLTF_NOEXCEPTION 来完全移除编译 TinyGLTF 时的 C++ 异常代码。
- `TINYGLTF_NO_STB_IMAGE`：不要使用 stb_image 加载图像。请改用其他方法TinyGLTF::SetImageLoader(LoadimageDataFunction LoadImageData, void *user_data)来设置加载图像的回调函数。
- `TINYGLTF_NO_STB_IMAGE_WRITE`：不要使用 stb_image_write 写入图像。请改用其他方法TinyGLTF::SetImageWriter(WriteimageDataFunction WriteImageData, void *user_data)来设置写入图像的回调函数。
-  `TINYGLTF_NO_EXTERNAL_IMAGE`：不要尝试加载外部图像文件。如果您不想在 glTF 解析期间加载图像文件，此选项会很有用。
- `TINYGLTF_ENABLE_DRACO`：启用 Draco 压缩。用户必须在项目文件中提供包含路径并链接相应的库。
-  `TINYGLTF_NO_INCLUDE_JSON` ：禁用json.hpp从内部包含，tiny_gltf.h因为它之前已被包含，或者你想在包含之前使用自定义路径包含它tiny_gltf.h。
- `TINYGLTF_NO_INCLUDE_RAPIDJSON` ：禁用从内部包含 RapidJson 的头文件，tiny_gltf.h因为它之前已经包含过，或者你想在包含之前使用自定义路径包含它tiny_gltf.h。
- `TINYGLTF_NO_INCLUDE_STB_IMAGE` ：禁用stb_image.h从内部包含，tiny_gltf.h因为它之前已被包含，或者你想在包含之前使用自定义路径包含它tiny_gltf.h。
- `TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE` ：禁用stb_image_write.h从内部包含，tiny_gltf.h因为它之前已被包含，或者你想在包含之前使用自定义路径包含它tiny_gltf.h。
- `TINYGLTF_USE_RAPIDJSON`：使用 RapidJSON 作为 JSON 解析器/序列化器。TinyGLTF 代码库中未包含 RapidJSON 文件。如果您启用此功能，请设置 RapidJSON 的包含路径。


---
