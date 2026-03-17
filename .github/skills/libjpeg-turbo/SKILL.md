---
name: libjpeg-turbo
description: "JPEG compression/decompression using libjpeg-turbo (TurboJPEG API and classic jpeglib API). Invoke when compressing RGB/RGBA buffers to JPEG, decompressing JPEG files/memory to pixels, querying JPEG header info, setting quality/subsampling, handling pixel formats (TJPF_*), error handling with tj3GetErrorStr, or integrating with the engine's jpeg IImageLoader. Also covers lossless transforms, YUV encode/decode, and scaling."
---

# libjpeg-turbo — JPEG 编解码

本项目使用 **libjpeg-turbo**（TurboJPEG 3.x API）进行 JPEG 编解码。  
源码位于 `src/third/libjpeg/`。  
引擎包装层位于 `src/image/jpeg.h` / `src/image/jpeg.cpp`（`shine::image::jpeg` 类，实现 `IImageLoader` 接口）。  
模块配置：`Module/image/jpeg.json`（静态库）。

> **优先使用 TurboJPEG API**（`turbojpeg.h`，`tj3*` 函数族），而非底层 `jpeglib.h`。  
> TurboJPEG API 更简洁、线程安全、并自动处理内存管理。

---

## 1. 头文件

```cpp
#include "turbojpeg.h"   // TurboJPEG API（推荐）
#include "jpeglib.h"     // 底层 IJG API（高级用途）
#include "jerror.h"      // 错误常量
```

---

## 2. 核心概念

### 2.1 实例初始化

```cpp
// 压缩
tjhandle compressor = tj3Init(TJINIT_COMPRESS);

// 解压
tjhandle decompressor = tj3Init(TJINIT_DECOMPRESS);

// 无损变换
tjhandle transformer = tj3Init(TJINIT_TRANSFORM);
```

> 使用完毕后 **必须** 调用 `tj3Destroy(handle)` 释放资源。

### 2.2 参数设置/读取

```cpp
tj3Set(handle, TJPARAM_QUALITY, 85);          // 压缩质量 1-100
tj3Set(handle, TJPARAM_SUBSAMP, TJSAMP_422);  // 色度子采样
tj3Set(handle, TJPARAM_FASTDCT, 1);           // 使用快速 DCT
tj3Set(handle, TJPARAM_PROGRESSIVE, 1);       // 渐进式 JPEG

int width = tj3Get(handle, TJPARAM_JPEGWIDTH);   // 解压后读取
int height = tj3Get(handle, TJPARAM_JPEGHEIGHT);
int subsamp = tj3Get(handle, TJPARAM_SUBSAMP);
int precision = tj3Get(handle, TJPARAM_PRECISION);
```

### 2.3 像素格式（TJPF）

| 枚举 | 通道数 | 说明 |
|------|--------|------|
| `TJPF_RGB` | 3 | R,G,B |
| `TJPF_BGR` | 3 | B,G,R |
| `TJPF_RGBX` | 4 | R,G,B,X（X 忽略） |
| `TJPF_RGBA` | 4 | R,G,B,A（解压时 A=255） |
| `TJPF_BGRA` | 4 | B,G,R,A |
| `TJPF_GRAY` | 1 | 灰度 |
| `TJPF_CMYK` | 4 | CMYK |

> 用 `tjPixelSize[pixelFormat]` 获取每像素样本数。

### 2.4 色度子采样（TJSAMP）

| 枚举 | MCU 块大小 | 说明 |
|------|-----------|------|
| `TJSAMP_444` | 8×8 | 无子采样（最高质量） |
| `TJSAMP_422` | 16×8 | 水平 2:1 |
| `TJSAMP_420` | 16×16 | 水平+垂直 2:1（最常用） |
| `TJSAMP_GRAY` | 8×8 | 灰度 |
| `TJSAMP_440` | 8×16 | 垂直 2:1 |
| `TJSAMP_411` | 32×8 | 水平 4:1 |

---

## 3. 压缩（RGB → JPEG）

```cpp
tjhandle handle = tj3Init(TJINIT_COMPRESS);
tj3Set(handle, TJPARAM_QUALITY, 90);
tj3Set(handle, TJPARAM_SUBSAMP, TJSAMP_420);

unsigned char* jpegBuf = nullptr;  // TJ 自动分配
size_t jpegSize = 0;

int result = tj3Compress8(
    handle,
    rgbBuffer,          // const unsigned char* 源像素数据
    width,              // 图像宽度
    0,                  // pitch（0 = width * pixelSize）
    height,             // 图像高度
    TJPF_RGB,           // 源像素格式
    &jpegBuf,           // 输出 JPEG 缓冲区
    &jpegSize           // 输出 JPEG 大小
);

if (result == -1) {
    // 错误处理
    const char* err = tj3GetErrorStr(handle);
    int errCode = tj3GetErrorCode(handle);  // TJERR_WARNING 或 TJERR_FATAL
}

// 使用 jpegBuf/jpegSize...

tj3Free(jpegBuf);      // 释放 TJ 分配的缓冲区
tj3Destroy(handle);     // 释放实例
```

### 3.1 预分配缓冲区

```cpp
// 计算最大缓冲区大小
size_t maxSize = tj3JPEGBufSize(width, height, TJSAMP_420);
unsigned char* jpegBuf = (unsigned char*)tj3Alloc(maxSize);
size_t jpegSize = maxSize;
tj3Set(handle, TJPARAM_NOREALLOC, 1);  // 禁止重分配

tj3Compress8(handle, src, w, 0, h, TJPF_RGB, &jpegBuf, &jpegSize);
// jpegSize 现在包含实际 JPEG 大小

tj3Free(jpegBuf);
```

---

## 4. 解压（JPEG → RGB/RGBA）

```cpp
tjhandle handle = tj3Init(TJINIT_DECOMPRESS);

// 第一步：读取 JPEG 头
int result = tj3DecompressHeader(handle, jpegBuf, jpegSize);
if (result == -1) { /* 错误处理 */ }

int width  = tj3Get(handle, TJPARAM_JPEGWIDTH);
int height = tj3Get(handle, TJPARAM_JPEGHEIGHT);
int subsamp = tj3Get(handle, TJPARAM_SUBSAMP);

// 第二步：分配目标缓冲区
int pixelFormat = TJPF_RGBA;
size_t bufSize = width * height * tjPixelSize[pixelFormat];
auto dstBuf = std::make_unique<unsigned char[]>(bufSize);

// 第三步：解压
result = tj3Decompress8(
    handle,
    jpegBuf,            // JPEG 数据
    jpegSize,           // JPEG 大小
    dstBuf.get(),       // 目标缓冲区
    0,                  // pitch（0 = width * pixelSize）
    pixelFormat         // 目标像素格式
);

if (result == -1) {
    const char* err = tj3GetErrorStr(handle);
}

tj3Destroy(handle);
```

### 4.1 缩放解压

```cpp
int numFactors;
tjscalingfactor* factors = tj3GetScalingFactors(&numFactors);

// 选择 1/2 缩放
tjscalingfactor half = {1, 2};
tj3SetScalingFactor(handle, half);

// 计算缩放后的尺寸
int scaledW = TJSCALED(width, half);
int scaledH = TJSCALED(height, half);

// 分配缩放后大小的缓冲区并解压
```

### 4.2 局部裁剪解压

```cpp
tj3DecompressHeader(handle, jpegBuf, jpegSize);

tjregion crop = {0, 0, 256, 256};  // x 必须是 MCU 块宽度的倍数
tj3SetCroppingRegion(handle, crop);

// 解压只输出裁剪区域
tj3Decompress8(handle, jpegBuf, jpegSize, dstBuf, 0, TJPF_RGB);
```

---

## 5. 无损变换

```cpp
tjhandle handle = tj3Init(TJINIT_TRANSFORM);

tjtransform xform = {};
xform.op = TJXOP_ROT90;                    // 旋转 90°
xform.options = TJXOPT_TRIM | TJXOPT_CROP; // 裁切不完整的 MCU 块

unsigned char* dstBuf = nullptr;
size_t dstSize = 0;

tj3Transform(handle, jpegBuf, jpegSize, 1, &dstBuf, &dstSize, &xform);

tj3Free(dstBuf);
tj3Destroy(handle);
```

变换操作：`TJXOP_NONE`, `TJXOP_HFLIP`, `TJXOP_VFLIP`, `TJXOP_TRANSPOSE`, `TJXOP_TRANSVERSE`, `TJXOP_ROT90`, `TJXOP_ROT180`, `TJXOP_ROT270`

---

## 6. YUV 编码/解码

```cpp
// RGB → YUV
tjhandle handle = tj3Init(TJINIT_COMPRESS);
tj3Set(handle, TJPARAM_SUBSAMP, TJSAMP_420);

size_t yuvSize = tj3YUVBufSize(width, 1, height, TJSAMP_420);
auto yuvBuf = std::make_unique<unsigned char[]>(yuvSize);

tj3EncodeYUV8(handle, rgbBuf, width, 0, height, TJPF_RGB, yuvBuf.get(), 1);

// YUV → RGB
tjhandle dec = tj3Init(TJINIT_DECOMPRESS);
tj3Set(dec, TJPARAM_SUBSAMP, TJSAMP_420);

tj3DecodeYUV8(dec, yuvBuf.get(), 1, rgbOut, width, 0, height, TJPF_RGB);
```

---

## 7. 错误处理模式

```cpp
// 严格模式：警告也停止
tj3Set(handle, TJPARAM_STOPONWARNING, 1);

// 检查错误
if (tj3Compress8(...) == -1) {
    int code = tj3GetErrorCode(handle);
    const char* msg = tj3GetErrorStr(handle);
    
    if (code == TJERR_FATAL) {
        // 不可恢复
    } else {
        // TJERR_WARNING: 输出可能损坏但部分可用
    }
}
```

---

## 8. 安全建议

```cpp
// 限制最大像素数，防止内存耗尽攻击
tj3Set(handle, TJPARAM_MAXPIXELS, 100 * 1024 * 1024);  // 1亿像素

// 限制渐进式 JPEG 扫描次数
tj3Set(handle, TJPARAM_SCANLIMIT, 500);

// 限制中间缓冲区内存
tj3Set(handle, TJPARAM_MAXMEMORY, 512);  // 512 MB
```

---

## 9. 与引擎集成

引擎的 `shine::image::jpeg` 类（在 `src/image/jpeg.h`）是一个手写的 JPEG 解码器，实现了 `IImageLoader` 接口：

```cpp
class jpeg : public loader::IImageLoader {
    bool loadFromFile(const char* filePath) override;
    bool loadFromMemory(const void* data, size_t size) override;
    void unload() override;

    std::expected<void, std::string> decode() override;           // → RGBA
    std::expected<std::vector<uint8_t>, std::string> decodeRGB() override;

    uint32_t getWidth() const noexcept override;
    uint32_t getHeight() const noexcept override;
    const std::vector<uint8_t>& getImageData() const noexcept override;
};
```

使用方式：
```cpp
shine::image::jpeg loader;
if (loader.loadFromFile("image.jpg")) {
    auto result = loader.decode();  // 解码到 RGBA
    if (result.has_value()) {
        const auto& rgba = loader.getImageData();
        uint32_t w = loader.getWidth();
        uint32_t h = loader.getHeight();
    }
}
```

如果需要更高性能或压缩能力，直接使用 TurboJPEG API（`tj3*` 函数）即可。

---

## 10. 底层 jpeglib API（高级用途）

仅在需要精细控制时使用（如自定义数据源/目标、高级错误处理）。

### 10.1 压缩（jpeglib）

```cpp
struct jpeg_compress_struct cinfo;
struct jpeg_error_mgr jerr;

cinfo.err = jpeg_std_error(&jerr);
jpeg_create_compress(&cinfo);

// 设置内存目标
unsigned char* outBuf = nullptr;
unsigned long outSize = 0;
jpeg_mem_dest(&cinfo, &outBuf, &outSize);

cinfo.image_width = width;
cinfo.image_height = height;
cinfo.input_components = 3;
cinfo.in_color_space = JCS_RGB;

jpeg_set_defaults(&cinfo);
jpeg_set_quality(&cinfo, 85, TRUE);

jpeg_start_compress(&cinfo, TRUE);

while (cinfo.next_scanline < cinfo.image_height) {
    JSAMPROW row = &rgbData[cinfo.next_scanline * width * 3];
    jpeg_write_scanlines(&cinfo, &row, 1);
}

jpeg_finish_compress(&cinfo);
jpeg_destroy_compress(&cinfo);
// outBuf / outSize 包含 JPEG 数据
free(outBuf);
```

### 10.2 解压（jpeglib + setjmp 错误恢复）

```cpp
struct jpeg_decompress_struct cinfo;
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
} jerr;

cinfo.err = jpeg_std_error(&jerr.pub);
jerr.pub.error_exit = [](j_common_ptr info) {
    auto* myerr = reinterpret_cast<my_error_mgr*>(info->err);
    longjmp(myerr->setjmp_buffer, 1);
};

if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    return;  // 错误恢复
}

jpeg_create_decompress(&cinfo);
jpeg_mem_src(&cinfo, jpegBuf, jpegSize);
jpeg_read_header(&cinfo, TRUE);
jpeg_start_decompress(&cinfo);

int row_stride = cinfo.output_width * cinfo.output_components;
while (cinfo.output_scanline < cinfo.output_height) {
    JSAMPROW row = &outBuf[cinfo.output_scanline * row_stride];
    jpeg_read_scanlines(&cinfo, &row, 1);
}

jpeg_finish_decompress(&cinfo);
jpeg_destroy_decompress(&cinfo);
```

---

## 11. 常见陷阱

| 问题 | 解决 |
|------|------|
| 忘记 `tj3Destroy` | **必须** 在所有路径上销毁，推荐 RAII 包装 |
| 忘记 `tj3Free` | TJ 分配的 JPEG 缓冲区必须用 `tj3Free` 释放，不能用 `free`/`delete` |
| pitch = 0 含义 | 等价于 `width * tjPixelSize[pixelFormat]`，非零时用于指定行对齐 |
| 解压前未调用 `tj3DecompressHeader` | 无法获取宽高和子采样信息 |
| TJPF_RGBX vs TJPF_RGBA | RGBX 解压时 X 分量未定义；RGBA 保证 A=255 |
| 错误时返回 -1 而非抛异常 | C API，必须手动检查返回值 |
| 裁剪区域 x 坐标对齐 | 必须是 MCU 块宽度的倍数 |

---

## 12. RAII 包装建议

```cpp
struct TjHandle {
    tjhandle handle = nullptr;
    TjHandle(int initType) : handle(tj3Init(initType)) {}
    ~TjHandle() { if (handle) tj3Destroy(handle); }
    TjHandle(const TjHandle&) = delete;
    TjHandle& operator=(const TjHandle&) = delete;
    operator tjhandle() const { return handle; }
    explicit operator bool() const { return handle != nullptr; }
};

struct TjBuffer {
    unsigned char* buf = nullptr;
    size_t size = 0;
    ~TjBuffer() { if (buf) tj3Free(buf); }
    TjBuffer(const TjBuffer&) = delete;
    TjBuffer& operator=(const TjBuffer&) = delete;
};
```
