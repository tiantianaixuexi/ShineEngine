#include "ShineGltf.h"

#include <algorithm>
#include <cassert>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "glaze/json/generic.hpp"

#include "util/file_util.ixx"
#include "util/path_util.h"
#include "util/encoding/url_util.h"

namespace shine::gltf {

// ===========================================================================
// GLB binary format constants
// ===========================================================================

static constexpr uint32_t GLB_MAGIC         = 0x46546C67u; // 'glTF' little-endian
static constexpr uint32_t GLB_VERSION        = 2u;
static constexpr uint32_t GLB_CHUNK_JSON     = 0x4E4F534Au; // 'JSON'
static constexpr uint32_t GLB_CHUNK_BIN      = 0x004E4942u; // 'BIN\0'
static constexpr size_t   GLB_HEADER_SIZE    = 12u;
static constexpr size_t   GLB_CHUNK_HDR_SIZE = 8u;

// ===========================================================================
// Glaze JSON aliases
// ===========================================================================

using json   = glz::generic_u64;
using jobj_t = json::object_t;
using jarr_t = json::array_t;

// ===========================================================================
// JSON traversal helpers
// ===========================================================================

static const json* find_key(const json& node, std::string_view key) noexcept {
    if (const auto* o = node.get_if<jobj_t>()) {
        auto it = o->find(key);
        if (it != o->end()) return &it->second;
    }
    return nullptr;
}

static std::optional<std::string_view> jstr(const json& node) noexcept {
    if (const auto* s = node.get_if<std::string>()) return std::string_view{*s};
    return {};
}

static std::optional<double> jnum(const json& node) noexcept {
    if (const auto* u = node.get_if<uint64_t>()) return static_cast<double>(*u);
    if (const auto* i = node.get_if<int64_t>())  return static_cast<double>(*i);
    if (const auto* d = node.get_if<double>())   return *d;
    return {};
}

static std::optional<int> jint(const json& node) noexcept {
    if (auto v = jnum(node)) return static_cast<int>(*v);
    return {};
}

static std::optional<bool> jbool(const json& node) noexcept {
    if (const auto* b = node.get_if<bool>()) return *b;
    return {};
}

// Get a std::string field from an object (for structs that use std::string), or return default.
static std::string obj_str(const json& obj, std::string_view key, std::string_view def = {}) {
    if (const auto* v = find_key(obj, key))
        if (auto s = jstr(*v)) return std::string{*s};
    return std::string{def};
}

// Get an SString field from an object (for engine-string fields), or return default.
static SString obj_sstr(const json& obj, std::string_view key, STextView def = {}) {
    if (const auto* v = find_key(obj, key))
        if (auto s = jstr(*v)) return SString{*s};
    return SString{def};
}

// Get an int field from an object, or return default.
static int obj_int(const json& obj, std::string_view key, int def = -1) {
    if (const auto* v = find_key(obj, key))
        if (auto i = jint(*v)) return *i;
    return def;
}

// Get a double field from an object, or return default.
static double obj_dbl(const json& obj, std::string_view key, double def = 0.0) {
    if (const auto* v = find_key(obj, key))
        if (auto d = jnum(*v)) return *d;
    return def;
}

// Get a bool field from an object, or return default.
static bool obj_bool(const json& obj, std::string_view key, bool def = false) {
    if (const auto* v = find_key(obj, key))
        if (auto b = jbool(*v)) return *b;
    return def;
}

// Collect a double array: ["values"] -> vector<double>
static std::vector<double> obj_dbl_array(const json& obj, std::string_view key) {
    std::vector<double> out;
    const auto* v = find_key(obj, key);
    if (!v) return out;
    const auto* arr = v->get_if<jarr_t>();
    if (!arr) return out;
    out.reserve(arr->size());
    for (const auto& elem : *arr)
        if (auto d = jnum(elem)) out.push_back(*d);
    return out;
}

// Collect an int array: ["values"] -> vector<int>
static std::vector<int> obj_int_array(const json& obj, std::string_view key) {
    std::vector<int> out;
    const auto* v = find_key(obj, key);
    if (!v) return out;
    const auto* arr = v->get_if<jarr_t>();
    if (!arr) return out;
    out.reserve(arr->size());
    for (const auto& elem : *arr)
        if (auto i = jint(elem)) out.push_back(*i);
    return out;
}

// Collect string array
static std::vector<std::string> obj_str_array(const json& obj, std::string_view key) {
    std::vector<std::string> out;
    const auto* v = find_key(obj, key);
    if (!v) return out;
    const auto* arr = v->get_if<jarr_t>();
    if (!arr) return out;
    out.reserve(arr->size());
    for (const auto& elem : *arr)
        if (auto s = jstr(elem)) out.emplace_back(*s);
    return out;
}

// ===========================================================================
// glTF type helpers
// ===========================================================================

static int ParseGltfType(std::string_view ty) noexcept {
    using sv = std::string_view;
    if (ty == sv{"SCALAR"}) return TYPE_SCALAR;
    if (ty == sv{"VEC2"})   return TYPE_VEC2;
    if (ty == sv{"VEC3"})   return TYPE_VEC3;
    if (ty == sv{"VEC4"})   return TYPE_VEC4;
    if (ty == sv{"MAT2"})   return TYPE_MAT2;
    if (ty == sv{"MAT3"})   return TYPE_MAT3;
    if (ty == sv{"MAT4"})   return TYPE_MAT4;
    return -1;
}

// ===========================================================================
// Per-section parsers
// ===========================================================================

static void ParseTextureInfo(const json& obj, GltfTextureInfo& out) {
    out.index    = obj_int(obj, "index",    -1);
    out.texCoord = obj_int(obj, "texCoord",  0);
}

static void ParseNormalTextureInfo(const json& obj, GltfNormalTextureInfo& out) {
    out.index    = obj_int(obj, "index",    -1);
    out.texCoord = obj_int(obj, "texCoord",  0);
    out.scale    = obj_dbl(obj, "scale",   1.0);
}

static void ParseOcclusionTextureInfo(const json& obj, GltfOcclusionTextureInfo& out) {
    out.index    = obj_int(obj, "index",    -1);
    out.texCoord = obj_int(obj, "texCoord",  0);
    out.strength = obj_dbl(obj, "strength",1.0);
}

static void ParsePbrMetallicRoughness(const json& obj, GltfPbrMetallicRoughness& out) {
    if (const auto* bcf = find_key(obj, "baseColorFactor"))
        out.baseColorFactor = obj_dbl_array(*bcf, ""); // handled below
    else
        out.baseColorFactor = {1.0, 1.0, 1.0, 1.0};

    // baseColorFactor is a JSON array directly
    if (const auto* bcf = find_key(obj, "baseColorFactor")) {
        if (const auto* arr = bcf->get_if<jarr_t>()) {
            out.baseColorFactor.clear();
            for (const auto& e : *arr) if (auto d = jnum(e)) out.baseColorFactor.push_back(*d);
        }
    }

    if (const auto* bct = find_key(obj, "baseColorTexture"))
        ParseTextureInfo(*bct, out.baseColorTexture);

    out.metallicFactor  = obj_dbl(obj, "metallicFactor",  1.0);
    out.roughnessFactor = obj_dbl(obj, "roughnessFactor", 1.0);

    if (const auto* mrt = find_key(obj, "metallicRoughnessTexture"))
        ParseTextureInfo(*mrt, out.metallicRoughnessTexture);
}

static GltfAsset ParseAsset(const json& obj) {
    GltfAsset a;
    a.version    = obj_str(obj, "version",    "2.0");
    a.generator  = obj_str(obj, "generator");
    a.minVersion = obj_str(obj, "minVersion");
    a.copyright  = obj_str(obj, "copyright");
    return a;
}

static GltfScene ParseScene(const json& obj) {
    GltfScene s;
    s.name  = obj_str(obj, "name");
    s.nodes = obj_int_array(obj, "nodes");
    return s;
}

static GltfNode ParseNode(const json& obj) {
    GltfNode n;
    n.name        = obj_str(obj, "name");
    n.camera      = obj_int(obj, "camera",  -1);
    n.skin        = obj_int(obj, "skin",    -1);
    n.mesh        = obj_int(obj, "mesh",    -1);
    n.children    = obj_int_array(obj, "children");
    n.rotation    = obj_dbl_array(obj, "rotation");
    n.scale       = obj_dbl_array(obj, "scale");
    n.translation = obj_dbl_array(obj, "translation");
    n.matrix      = obj_dbl_array(obj, "matrix");
    n.weights     = obj_dbl_array(obj, "weights");

    // KHR_lights_punctual
    if (const auto* exts = find_key(obj, "extensions")) {
        if (const auto* khr = find_key(*exts, "KHR_lights_punctual"))
            n.light = obj_int(*khr, "light", -1);
    }
    return n;
}

static GltfMesh ParseMesh(const json& obj) {
    GltfMesh m;
    m.name = obj_str(obj, "name");
    m.weights = obj_dbl_array(obj, "weights");

    const auto* primsArr = find_key(obj, "primitives");
    if (!primsArr) return m;
    const auto* arr = primsArr->get_if<jarr_t>();
    if (!arr) return m;

    m.primitives.reserve(arr->size());
    for (const auto& pobj : *arr) {
        GltfPrimitive prim;
        prim.material = obj_int(pobj, "material", -1);
        prim.indices  = obj_int(pobj, "indices",  -1);
        prim.mode     = obj_int(pobj, "mode",     -1);

        if (const auto* attribs = find_key(pobj, "attributes")) {
            if (const auto* ao = attribs->get_if<jobj_t>()) {
                for (const auto& [k, v] : *ao)
                    if (auto i = jint(v)) prim.attributes[k] = *i;
            }
        }

        if (const auto* targets = find_key(pobj, "targets")) {
            if (const auto* ta = targets->get_if<jarr_t>()) {
                for (const auto& tobj : *ta) {
                    std::map<std::string, int> tmap;
                    if (const auto* to = tobj.get_if<jobj_t>())
                        for (const auto& [k, v] : *to)
                            if (auto i = jint(v)) tmap[k] = *i;
                    prim.targets.push_back(std::move(tmap));
                }
            }
        }
        m.primitives.push_back(std::move(prim));
    }
    return m;
}

static GltfAccessor ParseAccessor(const json& obj) {
    GltfAccessor a;
    a.name          = obj_str(obj, "name");
    a.bufferView    = obj_int(obj, "bufferView",   -1);
    a.byteOffset    = static_cast<size_t>(obj_int(obj, "byteOffset", 0));
    a.componentType = obj_int(obj, "componentType", -1);
    a.normalized    = obj_bool(obj, "normalized", false);
    a.count         = static_cast<size_t>(obj_int(obj, "count", 0));

    if (const auto* ty = find_key(obj, "type"))
        if (auto s = jstr(*ty)) a.type = ParseGltfType(*s);

    a.minValues = obj_dbl_array(obj, "min");
    a.maxValues = obj_dbl_array(obj, "max");

    if (const auto* sp = find_key(obj, "sparse")) {
        a.sparse.isSparse = true;
        a.sparse.count    = obj_int(*sp, "count", 0);
        if (const auto* idx = find_key(*sp, "indices")) {
            a.sparse.indices.bufferView    = obj_int(*idx, "bufferView", -1);
            a.sparse.indices.byteOffset    = static_cast<size_t>(obj_int(*idx, "byteOffset", 0));
            a.sparse.indices.componentType = obj_int(*idx, "componentType", -1);
        }
        if (const auto* vals = find_key(*sp, "values")) {
            a.sparse.values.bufferView = obj_int(*vals, "bufferView", -1);
            a.sparse.values.byteOffset = static_cast<size_t>(obj_int(*vals, "byteOffset", 0));
        }
    }
    return a;
}

static GltfBufferView ParseBufferView(const json& obj) {
    GltfBufferView bv;
    bv.name       = obj_str(obj, "name");
    bv.buffer     = obj_int(obj, "buffer",     -1);
    bv.byteOffset = static_cast<size_t>(obj_int(obj, "byteOffset",  0));
    bv.byteLength = static_cast<size_t>(obj_int(obj, "byteLength",  0));
    bv.byteStride = static_cast<size_t>(obj_int(obj, "byteStride",  0));
    bv.target     = obj_int(obj, "target", 0);
    return bv;
}

static GltfBuffer ParseBuffer(const json& obj) {
    GltfBuffer b;
    b.name = obj_str(obj, "name");
    b.uri  = obj_str(obj, "uri");
    // data is filled later in LoadBuffers
    auto byteLen = static_cast<size_t>(obj_int(obj, "byteLength", 0));
    b.data.resize(byteLen);
    return b;
}

static GltfImage ParseImage(const json& obj) {
    GltfImage img;
    img.name       = obj_sstr(obj, "name");
    img.uri        = obj_sstr(obj, "uri");
    img.mimeType   = obj_sstr(obj, "mimeType");
    img.bufferView = obj_int(obj, "bufferView", -1);
    return img;
}

static GltfTexture ParseTexture(const json& obj) {
    GltfTexture t;
    t.name    = obj_sstr(obj, "name");
    t.sampler = obj_int(obj, "sampler",  0);
    t.source  = obj_int(obj, "source",   0);
    return t;
}

static GltfSampler ParseSampler(const json& obj) {
    GltfSampler s;
    s.name      = obj_sstr(obj, "name");
    s.minFilter = obj_int(obj, "minFilter", -1);
    s.magFilter = obj_int(obj, "magFilter", -1);
    s.wrapS     = obj_int(obj, "wrapS", static_cast<int>(TEXTURE_WRAP_REPEAT));
    s.wrapT     = obj_int(obj, "wrapT", static_cast<int>(TEXTURE_WRAP_REPEAT));
    return s;
}

static GltfMaterial ParseMaterial(const json& obj) {
    GltfMaterial m;
    m.name        = obj_str(obj,  "name");
    m.alphaMode   = obj_str(obj,  "alphaMode",   "OPAQUE");
    m.alphaCutoff = obj_dbl(obj,  "alphaCutoff", 0.5);
    m.doubleSided = obj_bool(obj, "doubleSided", false);

    if (const auto* ef = find_key(obj, "emissiveFactor")) {
        if (const auto* arr = ef->get_if<jarr_t>()) {
            m.emissiveFactor.clear();
            for (const auto& e : *arr) if (auto d = jnum(e)) m.emissiveFactor.push_back(*d);
        }
    }

    if (const auto* pbr = find_key(obj, "pbrMetallicRoughness"))
        ParsePbrMetallicRoughness(*pbr, m.pbrMetallicRoughness);
    if (const auto* nt = find_key(obj, "normalTexture"))
        ParseNormalTextureInfo(*nt, m.normalTexture);
    if (const auto* ot = find_key(obj, "occlusionTexture"))
        ParseOcclusionTextureInfo(*ot, m.occlusionTexture);
    if (const auto* et = find_key(obj, "emissiveTexture"))
        ParseTextureInfo(*et, m.emissiveTexture);

    return m;
}

static GltfSkin ParseSkin(const json& obj) {
    GltfSkin s;
    s.name                 = obj_sstr(obj, "name");
    s.inverseBindMatrices  = obj_int(obj, "inverseBindMatrices", -1);
    s.skeleton             = obj_int(obj, "skeleton",            -1);
    s.joints               = obj_int_array(obj, "joints");
    return s;
}

static GltfAnimationChannel ParseAnimationChannel(const json& obj) {
    GltfAnimationChannel ch;
    ch.sampler = obj_int(obj, "sampler", -1);
    if (const auto* tgt = find_key(obj, "target")) {
        ch.targetNode = obj_int(*tgt, "node");
        ch.targetPath = obj_sstr(*tgt, "path");
    }
    return ch;
}

static GltfAnimationSampler ParseAnimationSampler(const json& obj) {
    GltfAnimationSampler s;
    s.input         = obj_int(obj, "input",         -1);
    s.output        = obj_int(obj, "output",        -1);
    s.interpolation = obj_sstr(obj, "interpolation", "LINEAR");
    return s;
}

static GltfAnimation ParseAnimation(const json& obj) {
    GltfAnimation anim;
    anim.name = obj_sstr(obj, "name");

    if (const auto* chs = find_key(obj, "channels")) {
        if (const auto* arr = chs->get_if<jarr_t>())
            for (const auto& c : *arr)
                anim.channels.push_back(ParseAnimationChannel(c));
    }
    if (const auto* sps = find_key(obj, "samplers")) {
        if (const auto* arr = sps->get_if<jarr_t>())
            for (const auto& s : *arr)
                anim.samplers.push_back(ParseAnimationSampler(s));
    }
    return anim;
}

// ===========================================================================
// Section-check helpers
// ===========================================================================

static SString CheckRequiredSections(const GltfModel& mdl, SectionCheck check) {
    using SC = SectionCheck;
    if ((static_cast<int>(check) & static_cast<int>(SC::REQUIRE_VERSION)) &&
        mdl.asset.version.empty())
        return "asset.version is required";
    if ((static_cast<int>(check) & static_cast<int>(SC::REQUIRE_SCENES)) &&
        mdl.scenes.empty())
        return "scenes array is required";
    if ((static_cast<int>(check) & static_cast<int>(SC::REQUIRE_NODES)) &&
        mdl.nodes.empty())
        return "nodes array is required";
    if ((static_cast<int>(check) & static_cast<int>(SC::REQUIRE_ACCESSORS)) &&
        mdl.accessors.empty())
        return "accessors array is required";
    if ((static_cast<int>(check) & static_cast<int>(SC::REQUIRE_BUFFERS)) &&
        mdl.buffers.empty())
        return "buffers array is required";
    if ((static_cast<int>(check) & static_cast<int>(SC::REQUIRE_BUFFER_VIEWS)) &&
        mdl.bufferViews.empty())
        return "bufferViews array is required";
    return {};
}

// ===========================================================================
// Main JSON → GltfModel parser
// ===========================================================================

std::expected<bool, SString> ShineGltf::ParseFromString(
    const char*  str,
    const size_t length,
    STextView    base_dir,
    SectionCheck check_sections)
{
    // ---- Parse raw JSON -----
    json doc;
    {
        std::string_view sv(str, length);
        auto ec = glz::read_json(doc, sv);
        if (ec) {
            return std::unexpected(SString("JSON parse error: ") + SString{glz::format_error(ec, sv)});
        }
    }

    const auto* root = doc.get_if<jobj_t>();
    if (!root)
        return std::unexpected<SString>("glTF root is not a JSON object");

    // ---- asset ----
    if (const auto* v = find_key(doc, "asset"))
        model_.asset = ParseAsset(*v);

    // ---- extensionsUsed / extensionsRequired ----
    {
        auto used = obj_str_array(doc, "extensionsUsed");
        model_.extensionsUsed.assign(used.begin(), used.end());
        auto req  = obj_str_array(doc, "extensionsRequired");
        model_.extensionsRequired.assign(req.begin(), req.end());
    }

    // ---- defaultScene ----
    model_.defaultScene = obj_int(doc, "scene", -1);

    // ---- scenes ----
    if (const auto* sv = find_key(doc, "scenes"))
        if (const auto* arr = sv->get_if<jarr_t>())
            for (const auto& s : *arr) model_.scenes.push_back(ParseScene(s));

    // ---- nodes ----
    if (const auto* nv = find_key(doc, "nodes"))
        if (const auto* arr = nv->get_if<jarr_t>())
            for (const auto& n : *arr) model_.nodes.push_back(ParseNode(n));

    // ---- meshes ----
    if (const auto* mv = find_key(doc, "meshes"))
        if (const auto* arr = mv->get_if<jarr_t>())
            for (const auto& m : *arr) model_.meshes.push_back(ParseMesh(m));

    // ---- accessors ----
    if (const auto* av = find_key(doc, "accessors"))
        if (const auto* arr = av->get_if<jarr_t>())
            for (const auto& a : *arr) model_.accessors.push_back(ParseAccessor(a));

    // ---- bufferViews ----
    if (const auto* bvv = find_key(doc, "bufferViews"))
        if (const auto* arr = bvv->get_if<jarr_t>())
            for (const auto& bv : *arr) model_.bufferViews.push_back(ParseBufferView(bv));

    // ---- buffers ----
    if (const auto* bv = find_key(doc, "buffers"))
        if (const auto* arr = bv->get_if<jarr_t>())
            for (const auto& b : *arr) model_.buffers.push_back(ParseBuffer(b));

    // ---- images ----
    if (const auto* iv = find_key(doc, "images"))
        if (const auto* arr = iv->get_if<jarr_t>())
            for (const auto& img : *arr) model_.images.push_back(ParseImage(img));

    // ---- textures ----
    if (const auto* tv = find_key(doc, "textures"))
        if (const auto* arr = tv->get_if<jarr_t>())
            for (const auto& t : *arr) model_.textures.push_back(ParseTexture(t));

    // ---- samplers ----
    if (const auto* sv2 = find_key(doc, "samplers"))
        if (const auto* arr = sv2->get_if<jarr_t>())
            for (const auto& s : *arr) model_.samplers.push_back(ParseSampler(s));

    // ---- materials ----
    if (const auto* matv = find_key(doc, "materials"))
        if (const auto* arr = matv->get_if<jarr_t>())
            for (const auto& m : *arr) model_.materials.push_back(ParseMaterial(m));

    // ---- skins ----
    if (const auto* skinv = find_key(doc, "skins"))
        if (const auto* arr = skinv->get_if<jarr_t>())
            for (const auto& s : *arr) model_.skins.push_back(ParseSkin(s));

    // ---- animations ----
    if (const auto* animv = find_key(doc, "animations"))
        if (const auto* arr = animv->get_if<jarr_t>())
            for (const auto& a : *arr) model_.animations.push_back(ParseAnimation(a));

    // ---- section check ----
    {
        SString err = CheckRequiredSections(model_, check_sections);
        if (!err.empty())
            return std::unexpected(std::move(err));
    }

    // ---- resolve buffers ----
    if (auto r = LoadBuffers(base_dir); !r) return r;

    return true;
}

// ===========================================================================
// Buffer loading
// ===========================================================================

std::expected<bool, SString> ShineGltf::LoadBuffers(STextView base_dir) {
    for (auto& buf : model_.buffers) {
        if (buf.uri.empty()) {
            // GLB embedded buffer: fill from bin_chunk_
            if (!bin_chunk_.empty() && bin_chunk_.size() >= buf.data.size()) {
                auto src = bin_chunk_.first(buf.data.size());
                std::ranges::transform(src, buf.data.begin(), [](std::byte b) {
                    return std::to_integer<unsigned char>(b);
                });
            }
            continue;
        }

        // buf.uri is std::string — convert explicitly via std::string_view
        const STextView uri{std::string_view{buf.uri}};

        if (IsDataURI(uri)) {
            // data: URI — base64-decode
            SString mime;
            auto result = util::decodeDataURIWithMimeType(uri, mime, buf.data.size());
            if (!result) {
                return std::unexpected<SString>("Failed to decode data URI for buffer");
            }
            buf.data.assign(result->begin(), result->end());
        } else {
            // External file
            SString resolved = util::join_path(base_dir, uri);
            SString decoded  = util::urlDecode(resolved);
            auto   fileRes   = util::read_file_bytes(STextView{decoded});
            if (!fileRes) {
                return std::unexpected(SString("Failed to read buffer file: ") + decoded);
            }
            auto& bytes = *fileRes;
            buf.data.resize(bytes.size());
            std::ranges::transform(bytes, buf.data.begin(), [](std::byte b) {
                return std::to_integer<unsigned char>(b);
            });
        }
    }
    return true;
}

// ===========================================================================
// Image loading
// ===========================================================================

std::expected<bool, SString> ShineGltf::LoadImages(STextView base_dir) {
    for (auto& img : model_.images) {
        if (images_as_is_) {
            img.as_is = true;
            // Leave raw bytes in img.image if already loaded via bufferView
        }

        if (!img.uri.empty()) {
            if (IsDataURI(img.uri)) {
                SString mime;
                auto result = util::decodeDataURIWithMimeType(img.uri, mime);
                if (!result) {
                    return std::unexpected<SString>("Failed to decode image data URI");
                }
                img.image.assign(result->begin(), result->end());
                img.mimeType = std::move(mime);
            } else {
                SString resolved = util::join_path(base_dir, img.uri);
                SString decoded  = util::urlDecode(resolved);
                auto   fileRes   = util::read_file_bytes(STextView{decoded});
                if (!fileRes) {
                    return std::unexpected(SString("Failed to read image file: ") + decoded);
                }
                auto& bytes = *fileRes;
                img.image.resize(bytes.size());
                std::ranges::transform(bytes, img.image.begin(), [](std::byte b) {
                    return std::to_integer<unsigned char>(b);
                });
            }
        } else if (img.bufferView >= 0 &&
                   img.bufferView < static_cast<int>(model_.bufferViews.size())) {
            // Embedded image via bufferView
            const auto& bv = model_.bufferViews[static_cast<size_t>(img.bufferView)];
            if (bv.buffer >= 0 &&
                bv.buffer < static_cast<int>(model_.buffers.size())) {
                const auto& bufData = model_.buffers[static_cast<size_t>(bv.buffer)].data;
                if (bv.byteOffset + bv.byteLength <= bufData.size()) {
                    auto beg = bufData.cbegin() + static_cast<std::ptrdiff_t>(bv.byteOffset);
                    img.image.assign(beg, beg + static_cast<std::ptrdiff_t>(bv.byteLength));
                }
            }
        }
    }
    return true;
}

// ===========================================================================
// GLB parsing
// ===========================================================================

bool ShineGltf::IsBinaryGlb(std::span<const std::byte> data) noexcept {
    if (data.size() < 4) return false;
    const uint32_t magic = std::to_integer<uint32_t>(data[0])
                         | (std::to_integer<uint32_t>(data[1]) << 8u)
                         | (std::to_integer<uint32_t>(data[2]) << 16u)
                         | (std::to_integer<uint32_t>(data[3]) << 24u);
    return magic == GLB_MAGIC;
}

std::expected<bool, SString> ShineGltf::ParseGlb(
    std::span<const std::byte> data,
    STextView                  base_dir,
    SectionCheck               check_sections)
{
    if (data.size() < GLB_HEADER_SIZE)
        return std::unexpected<SString>("GLB file too small for header");

    // Read a little-endian uint32 from the span at the given byte offset.
    auto read_u32 = [&data](size_t off) noexcept -> uint32_t {
        return std::to_integer<uint32_t>(data[off])
             | (std::to_integer<uint32_t>(data[off + 1]) << 8u)
             | (std::to_integer<uint32_t>(data[off + 2]) << 16u)
             | (std::to_integer<uint32_t>(data[off + 3]) << 24u);
    };

    const uint32_t magic       = read_u32(0);
    const uint32_t version     = read_u32(4);
    const uint32_t totalLength = read_u32(8);

    if (magic != GLB_MAGIC)
        return std::unexpected<SString>("Not a GLB file (bad magic)");
    if (version != GLB_VERSION)
        return std::unexpected<SString>("Unsupported GLB version");
    if (totalLength > data.size())
        return std::unexpected<SString>("GLB length field exceeds data size");

    // ---- iterate chunks ----
    const char*  jsonStr    = nullptr;
    size_t       jsonLen    = 0;
    std::span<const std::byte> binChunk{};

    size_t offset = GLB_HEADER_SIZE;
    while (offset + GLB_CHUNK_HDR_SIZE <= data.size()) {
        const uint32_t chunkLength = read_u32(offset);
        const uint32_t chunkType   = read_u32(offset + 4);
        offset += GLB_CHUNK_HDR_SIZE;

        if (offset + chunkLength > data.size()) break;

        if (chunkType == GLB_CHUNK_JSON && jsonStr == nullptr) {
            jsonStr = static_cast<const char*>(static_cast<const void*>(data.subspan(offset).data()));
            jsonLen = chunkLength;
        } else if (chunkType == GLB_CHUNK_BIN && binChunk.empty()) {
            binChunk = data.subspan(offset, chunkLength);
        }

        offset += chunkLength;
    }

    if (!jsonStr || jsonLen == 0)
        return std::unexpected<SString>("GLB has no JSON chunk");

    // Store bin chunk span for buffer loading
    bin_chunk_ = binChunk;

    return ParseFromString(jsonStr, jsonLen, base_dir, check_sections);
}

// ===========================================================================
// Public entry points
// ===========================================================================

std::expected<bool, SString> ShineGltf::LoadFromFile(
    STextView path, SectionCheck check_sections)
{
    model_     = GltfModel{};
    bin_chunk_ = {};

    auto fileResult = util::read_full_file(path);
    if (!fileResult)
        return std::unexpected(fileResult.error());

    auto& fmv        = *fileResult;
    const auto& span = fmv.view.content; // already a std::span<const std::byte>

    base_dir_ = SString{util::get_directory(path)};

    if (IsBinaryGlb(span))
        return ParseGlb(span, STextView{base_dir_}, check_sections);

    // ASCII glTF
    return ParseFromString(
        static_cast<const char*>(static_cast<const void*>(span.data())),
        span.size(),
        STextView{base_dir_},
        check_sections);
}

std::expected<bool, SString> ShineGltf::LoadFromMemory(
    std::span<const std::byte> data,
    STextView                  base_dir,
    SectionCheck               check_sections)
{
    model_     = GltfModel{};
    bin_chunk_ = {};
    base_dir_  = SString{base_dir};

    if (data.empty())
        return std::unexpected<SString>("Empty data span");

    if (IsBinaryGlb(data))
        return ParseGlb(data, base_dir, check_sections);

    return ParseFromString(
        static_cast<const char*>(static_cast<const void*>(data.data())),
        data.size(),
        base_dir,
        check_sections);
}

} // namespace shine::gltf
