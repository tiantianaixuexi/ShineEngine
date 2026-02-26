#include "asset_base.h"

namespace shine::editor::asset
{
    void IAssetBase::Init()
    {
      
        assetID = algorithm::UUID::GenerateV7();

    }
}