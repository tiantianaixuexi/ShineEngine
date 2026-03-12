#pragma once


#include "util/file_util.ixx"
#include "string/shine_text_view.h"

namespace shine::gltf {




enum class GLTF_ParseStricteness : u8 {
    Permissive,
    Strict
};


bool IsDataURI(STextView in)  noexcept {
    if (in.starts_with("data:application/octet-stream;base64,") == 0) {
        return true;
    }

    if (in.starts_with("data:image/jpeg;base64,") == 0) {
        return true;
    }

    if (in.starts_with("data:image/png;base64,") == 0) {
        return true;
    }

    if (in.starts_with("data:image/bmp;base64,") == 0) {
        return true;
    }

    if (in.starts_with("data:image/gif;base64,") == 0) {
        return true;
    }

    if (in.starts_with("data:text/plain;base64") == 0) {
        return true;
    }

    if (in.starts_with("data:application/gltf-buffer;base64,") == 0) {
        return true;
    }

    return false;
}


class ShineGltf
{
    ShineGltf() = default;

public:

    std::expected<bool,SString> LoadGltfFromFile(STextView path, unsigned int check_sections);

    util::FileMapView mappedFile_;

    u64 fileSize {};
};



}; // namespace shine::gltf