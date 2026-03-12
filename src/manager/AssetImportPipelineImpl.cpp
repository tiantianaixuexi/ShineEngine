#include "manager/AssetImportPipelineImpl.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

#include "fmt/format.h"
#include "image/jpeg.h"
#include "image/png.h"
#include "loader/model/gltfLoader.h"
#include "loader/model/objLoader.h"
#include "manager/AssetCatalogImpl.h"
#include "util/file_util.ixx"
#include "util/string_util.ixx"
#include "util/timer/FunctionTimer.h"

namespace shine::manager
{
    AssetImportPipelineImpl::AssetImportPipelineImpl(AssetCatalogImpl& catalog)
        : catalog_(catalog)
    {
    }

    AssetHandle AssetImportPipelineImpl::LoadTextureAsset(const std::string& filePath)
    {
        shine::util::FunctionTimer timer("AssetManager::LoadTextureAsset", shine::util::TimerPrecision::Nanoseconds);
        const auto loadedHandle = catalog_.GetAssetHandleByPath(filePath);
        if (loadedHandle.isValid())
        {
            return { .id = loadedHandle.id, .generation = loadedHandle.generation, .type = EAssetType::Image, .path = filePath, .fromPackage = loadedHandle.fromPackage };
        }

        std::string ext = util::StringUtil::NormalizeFileExtension(filePath);
        auto loader = CreateImageLoader(ext);
        if (!loader)
        {
            fmt::print("AssetManager: 不支持的图片格式: {}\n", filePath);
            return {};
        }

        if (!loader->loadFromFile(filePath.c_str()))
        {
            fmt::print(FMT_STRING("AssetManager: 图片文件加载失败: {} - 错误: {}\n"), filePath, static_cast<int>(loader->getLastError()));
            return {};
        }

        auto decodeResult = loader->decode();
        if (!decodeResult.has_value() || !loader->isDecoded() || loader->getImageData().empty())
        {
            fmt::print(FMT_STRING("AssetManager: 图片解码失败: {} - {}\n"), filePath, decodeResult.has_value() ? "" : decodeResult.error());
            return {};
        }

        return catalog_.RegisterRuntimeAsset(EAssetType::Image, filePath, std::make_unique<RuntimeImageAsset>(filePath, std::move(loader)));
    }

    AssetHandle AssetImportPipelineImpl::LoadImageFromMemory(const void* data, size_t size, const std::string& formatHint)
    {
        shine::util::FunctionTimer timer("AssetManager::LoadImageFromMemory", shine::util::TimerPrecision::Nanoseconds);
        std::string format = formatHint.empty() ? DetectImageFormat(data, size) : formatHint;
        if (format.empty())
        {
            return {};
        }

        auto loader = CreateImageLoader(format);
        if (!loader || !loader->loadFromMemory(data, size))
        {
            return {};
        }
        auto decodeResult = loader->decode();
        if (!decodeResult.has_value() || !loader->isDecoded() || loader->getImageData().empty())
        {
            return {};
        }

        const auto memoryPath = fmt::format("memory://image/{}", catalog_.PeekNextHandleId());
        return catalog_.RegisterRuntimeAsset(EAssetType::Image, memoryPath, std::make_unique<RuntimeImageAsset>(memoryPath, std::move(loader)));
    }

    AssetHandle AssetImportPipelineImpl::LoadModel(const std::string& filePath)
    {
        return LoadModel(filePath, nullptr);
    }

    AssetHandle AssetImportPipelineImpl::LoadModel(const std::string& filePath, loader::IModelLoader::ProgressCallback progressCallback)
    {
        shine::util::FunctionTimer timer("AssetManager::LoadModel", shine::util::TimerPrecision::Nanoseconds);
        const auto loadedHandle = catalog_.GetAssetHandleByPath(filePath);
        if (loadedHandle.isValid())
        {
            return { .id = loadedHandle.id, .generation = loadedHandle.generation, .type = EAssetType::Model, .path = filePath, .fromPackage = loadedHandle.fromPackage };
        }

        std::string ext = util::StringUtil::NormalizeFileExtension(filePath);
        if (!util::file_exists(SString::from_utf8(filePath)))
        {
            return {};
        }

        auto loader = CreateModelLoader(ext);
        if (!loader)
        {
            return {};
        }
        if (progressCallback)
        {
            loader->setProgressCallback(std::move(progressCallback));
        }
        if (!loader->loadFromFile(filePath.c_str()))
        {
            return {};
        }

        return catalog_.RegisterRuntimeAsset(EAssetType::Model, filePath, std::make_unique<RuntimeModelAsset>(filePath, std::move(loader)));
    }

    AssetHandle AssetImportPipelineImpl::LoadMapAsset(const std::string& filePath)
    {
        const auto loadedHandle = catalog_.GetAssetHandleByPath(filePath);
        if (loadedHandle.isValid())
        {
            return { .id = loadedHandle.id, .generation = loadedHandle.generation, .type = EAssetType::Map, .path = filePath, .fromPackage = loadedHandle.fromPackage };
        }

        std::string mapName = std::filesystem::path(filePath).stem().string();
        if (mapName.empty())
        {
            mapName = fmt::format("Map_{}", catalog_.PeekNextHandleId());
        }

        auto mapAsset = std::make_unique<gameplay::world::MapAsset>(mapName, filePath);
        return catalog_.RegisterRuntimeAsset(EAssetType::Map, filePath, std::make_unique<RuntimeMapAsset>(std::move(mapAsset)));
    }

    std::unique_ptr<loader::IImageLoader> AssetImportPipelineImpl::CreateImageLoader(const std::string& format) const
    {
        std::string fmt = format;
        std::transform(fmt.begin(), fmt.end(), fmt.begin(), ::tolower);
        if (fmt == "png")
        {
            return std::make_unique<shine::image::png>();
        }
        if (fmt == "jpeg" || fmt == "jpg")
        {
            return std::make_unique<shine::image::jpeg>();
        }
        return nullptr;
    }

    std::unique_ptr<loader::IModelLoader> AssetImportPipelineImpl::CreateModelLoader(const std::string& format) const
    {
        std::string fmt = format;
        std::ranges::transform(fmt,fmt.begin(), ::tolower);
        if (fmt == "gltf" || fmt == "glb")
        {
            return std::make_unique<shine::loader::gltfLoader>();
        }
        if (fmt == "obj")
        {
            return std::make_unique<shine::loader::objLoader>();
        }
        return nullptr;
    }

    std::string AssetImportPipelineImpl::DetectImageFormat(const void* data, size_t size) const
    {
        if (!data || size < 12)
        {
            return "";
        }
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        if (size >= 8 && bytes[0] == 0x89 && bytes[1] == 0x50 && bytes[2] == 0x4E && bytes[3] == 0x47 &&
            bytes[4] == 0x0D && bytes[5] == 0x0A && bytes[6] == 0x1A && bytes[7] == 0x0A)
        {
            return "png";
        }
        if (size >= 2 && bytes[0] == 0xFF && bytes[1] == 0xD8)
        {
            return "jpeg";
        }
        return "";
    }
}
