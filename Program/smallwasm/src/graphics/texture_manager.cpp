#include "texture_manager.h"

namespace shine {
namespace graphics {

static TextureManager s_texMgr;

TextureManager& TextureManager::instance() {
    return s_texMgr;
}

int TextureManager::request_url(int ctxId, const char* url, int urlLen, int* out_texId, int* out_w, int* out_h) {
    return m_loader.request_url(ctxId, url, urlLen, out_texId, out_w, out_h);
}

int TextureManager::request_dataurl(int ctxId, const char* data, int dataLen, int* out_texId, int* out_w, int* out_h) {
    return m_loader.request_dataurl(ctxId, data, dataLen, out_texId, out_w, out_h);
}

int TextureManager::request_base64(int ctxId, const char* mime, int mimeLen, const char* b64, int b64Len,
                                   int* out_texId, int* out_w, int* out_h) {
    return m_loader.request_base64(ctxId, mime, mimeLen, b64, b64Len, out_texId, out_w, out_h);
}

void TextureManager::on_loaded(int reqId, int texId, int w, int h) {
    m_loader.on_loaded(reqId, texId, w, h);
}

void TextureManager::on_failed(int reqId) {
    m_loader.on_failed(reqId);
}

} // namespace graphics
} // namespace shine
