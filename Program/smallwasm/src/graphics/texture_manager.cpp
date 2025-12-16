#include "texture_manager.h"
#include "../engine/engine.h"
#include "../logfmt.h"

namespace shine::graphics 
{

static TextureManager s_texMgr;

TextureManager& TextureManager::instance() {
    return s_texMgr;
}

int TextureManager::request_url(const char* url, int urlLen, int* out_texId, int* out_w, int* out_h) {
    return m_loader.request_async_url(SHINE_ENGINE.getCtx(), url, urlLen, out_texId, out_w, out_h);
}

int TextureManager::request_dataurl( const char* data, int dataLen, int* out_texId, int* out_w, int* out_h) {
    return m_loader.request_async_dataurl(SHINE_ENGINE.getCtx(), data, dataLen, out_texId, out_w, out_h);
}

int TextureManager::request_base64(const char* mime, int mimeLen, const char* b64, int b64Len,
                                   int* out_texId, int* out_w, int* out_h) {
    return m_loader.request_async_base64(SHINE_ENGINE.getCtx(), mime, mimeLen, b64, b64Len, out_texId, out_w, out_h);
}


void TextureManager::on_loaded(int reqId, int texId, int w, int h) {
    LOG("on_loaded", reqId);
    m_loader.on_loaded(reqId, texId, w, h);
}

void TextureManager::on_failed(int reqId) {
    m_loader.on_failed(reqId);
}

} // namespace graphics

