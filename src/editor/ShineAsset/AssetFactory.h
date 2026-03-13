#pragma once
// ============================================================
//  AssetFactory — per-type creator functions for RuntimeAssetRegistry.
//
//  Each asset type registers a creator that knows how to instantiate
//  a concrete AssetBase subclass given a UUID.  The registry uses
//  the creator when fulfilling a load request for an unknown type.
//
//  Adding a new asset type:
//      RuntimeAssetRegistry::Get().RegisterFactory(
//          AssetTypeId::Texture,
//          [](STextView uuid) -> std::shared_ptr<AssetBase> {
//              return std::make_shared<TextureAsset>(uuid);
//          });
//
//  This file has NO knowledge of any concrete asset type.
// ============================================================

#include <functional>
#include <memory>

#include "AssetBase.h"
#include "string/shine_text_view.h"

namespace shine::asset
{
    // -----------------------------------------------------------------------
    //  Creator signature:
    //      uuid  — the UUID string for the new asset instance
    //  Returns a heap-allocated concrete AssetBase or nullptr on failure.
    // -----------------------------------------------------------------------
    using AssetCreatorFn = std::function<std::shared_ptr<AssetBase>(STextView uuid)>;

} // namespace shine::asset
