#pragma once

#include "util/Algorithm/uuid.h"


namespace shine::editor::asset
{

    class IAssetBase
    {
    public:
        IAssetBase() = default;
        virtual ~IAssetBase() = default;
        
        void Init();


    private:

        std::string     assetName;  // 资产名字
        std::string     assetPath;  // 资产路径
        std::string     assetMd5;   // 资产MD5值
        std::string     creatrTime; // 资产创建时间
        algorithm::UUID assetID;    // 资产唯一ID
    };

}