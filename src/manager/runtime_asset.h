#pragma once

#include <memory>
#include <string>

#include "Engine/Macro/RuntimeEditorSplit.h"
#include "EngineCore/asset/BaseAsset.h"
#include "loader/image/image_loader.h"
#include "loader/model/model_loader.h"
#include "gameplay/world/map_asset.h"

namespace shine::editor::asset
{
    class EditorRuntimeAssetBridge;
}

namespace shine::manager
{
    class IRuntimeAsset : public shine::editor::asset::IAssetBase
    {
    public:
        virtual ~IRuntimeAsset() = default;

    protected:
        RUNTIME_DATA(bool streamable_ = false;)
        RUNTIME_DATA(void(*streamCallback_)(IRuntimeAsset&) = nullptr;)
        EDITOR_DATA(std::string importProfile_;)
        EDITOR_DATA(struct EditorFlags
        {
            bool needThumbnail = false;
            bool showInInspector = true;
        } editorFlags_;)
    };

    class RuntimeImageAsset final : public IRuntimeAsset
    {
    public:
        RuntimeImageAsset(SString path, std::unique_ptr<loader::IImageLoader> loader)
            : loader_(std::move(loader))
        {
            //Init(path, path, EAssetKind::Texture);
        }

        loader::IImageLoader* getLoader() const noexcept { return loader_.get(); }

    private:
        std::unique_ptr<loader::IImageLoader> loader_;
    };

    class RuntimeModelAsset final : public IRuntimeAsset
    {
    public:
        RuntimeModelAsset(const std::string& path, std::unique_ptr<loader::IModelLoader> loader)
            : loader_(std::move(loader))
        {
           // Init(path, path, shine::editor::asset::EEditorAssetType::StaticMesh);
        }

        loader::IModelLoader* getLoader() const noexcept { return loader_.get(); }

    private:
        std::unique_ptr<loader::IModelLoader> loader_;
    };

    class RuntimeMapAsset final : public IRuntimeAsset
    {
    public:
        explicit RuntimeMapAsset(std::unique_ptr<shine::gameplay::world::MapAsset> mapAsset)
            : mapAsset_(std::move(mapAsset))
        {
            if (mapAsset_)
            {
                //Init(mapAsset_->getName(), mapAsset_->GetPath(), shine::editor::asset::EEditorAssetType::Scene);
            }
        }

        shine::gameplay::world::MapAsset* getMap() const noexcept { return mapAsset_.get(); }

    private:
        std::unique_ptr<shine::gameplay::world::MapAsset> mapAsset_;
    };

    void RegisterRuntimeAssetLoaders(shine::editor::asset::EditorRuntimeAssetBridge& bridge);
}
