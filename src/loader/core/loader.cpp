#include "loader.h"


namespace shine::loader
{
	bool IAssetLoader::validateAssetData(const void* data, size_t size) const
    {
        return data != nullptr && size > 0;
    }
}
