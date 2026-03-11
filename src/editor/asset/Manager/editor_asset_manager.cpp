#include "editor_asset_manager.h"

namespace shine::editor::asset
{
    // Legacy compatibility translation unit.
    //
    // EditorAssetManager is now implemented primarily inline in the header as a
    // thin adapter over the new AssetRegistry. This source file is intentionally
    // kept minimal so existing build scripts and project structure remain stable.
}