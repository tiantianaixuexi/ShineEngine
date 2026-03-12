#include "ShineGltf.h"


#include "encoding/url_util.h"
#include "util/file_util.ixx"

namespace shine::gltf {

std::expected<bool,SString> ShineGltf::LoadGltfFromFile(STextView path, unsigned int check_sections) 
{
    // 打开文件句柄
    auto result = util::open_file_from_mapping(path);
    if(!result.has_value())
    {
        return std::unexpected<SString>(result.error());
    }

    
    util::FileMapView mappedFile_;
    mappedFile_.map = std::move(*result);

    //读取文件大小
    {
        auto _sizeResult = util::get_file_size(mappedFile_.map);
        if(!_sizeResult.has_value()){
            return std::unexpected<SString>(_sizeResult.error());
        }

        // 获取文件大小
        fileSize = *_sizeResult;
        if(fileSize == 0)
        {
            return std::unexpected<SString>("文件大小为 0");
        }
    }

    STextView basedir = util::getBaseDir(path);

    
    
};

} // namespace shine::gltf