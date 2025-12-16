#pragma once

#include "../util/tex_loader.h"

namespace shine {
namespace graphics {

class TextureManager {
public:
    static TextureManager& instance();

    // Async texture requests
    int request_url(int ctxId, const char* url, int urlLen, int* out_texId, int* out_w, int* out_h);
    int request_dataurl(int ctxId, const char* data, int dataLen, int* out_texId, int* out_w, int* out_h);
    int request_base64(int ctxId, const char* mime, int mimeLen, const char* b64, int b64Len,
                       int* out_texId, int* out_w, int* out_h);

    // Callbacks from JS
    void on_loaded(int reqId, int texId, int w, int h);
    void on_failed(int reqId);

private:
    shine::wasm::TexLoader m_loader;
};

} // namespace graphics
} // namespace shine
