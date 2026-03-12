#pragma once

#include <map>
#include <vector>

#include "shine_define.h"
#include "string/shine_string.h"

#include "util/function/EventHandle.h"

namespace shine::gltf {

constexpr u32 COMPONENT_TYPE_BYTE           = 5120;
constexpr u32 COMPONENT_TYPE_UNSIGNED_BYTE  = 5121;
constexpr u32 COMPONENT_TYPE_SHORT          = 5122;
constexpr u32 COMPONENT_TYPE_UNSIGNED_SHORT = 5123;
constexpr u32 COMPONENT_TYPE_INT            = 5124;
constexpr u32 COMPONENT_TYPE_UNSIGNED_INT   = 5125;
constexpr u32 COMPONENT_TYPE_FLOAT          = 5126;
constexpr u32 COMPONENT_TYPE_DOUBLE         = 5130;

constexpr u32 TYPE_VEC2   = 2;
constexpr u32 TYPE_VEC3   = 3;
constexpr u32 TYPE_VEC4   = 4;
constexpr u32 TYPE_MAT2   = 32 + 2;
constexpr u32 TYPE_MAT3   = 32 + 3;
constexpr u32 TYPE_MAT4   = 32 + 4;
constexpr u32 TYPE_SCALAR = 64 + 1;
constexpr u32 TYPE_VECTOR = 64 + 4;
constexpr u32 TYPE_MATRIX = 64 + 16;

constexpr u32 TEXTURE_WRAP_REPEAT = 10497;

static inline s32 GetComponentSizeInBytes(u32 componentType) noexcept {
    switch (componentType) {
    case COMPONENT_TYPE_BYTE:
    case COMPONENT_TYPE_UNSIGNED_BYTE:
        return 1;
    case COMPONENT_TYPE_SHORT:
    case COMPONENT_TYPE_UNSIGNED_SHORT:
        return 2;
    case COMPONENT_TYPE_INT:
    case COMPONENT_TYPE_UNSIGNED_INT:
    case COMPONENT_TYPE_FLOAT:
        return 4;
    case COMPONENT_TYPE_DOUBLE:
        return 8;
    default:
        return -1;
    }
};

static inline s32 GetNumComponentsInType(u32 ty) {
    if (ty == TYPE_SCALAR) {
        return 1;
    } else if (ty == TYPE_VEC2) {
        return 2;
    } else if (ty == TYPE_VEC3) {
        return 3;
    } else if (ty == TYPE_VEC4) {
        return 4;
    } else if (ty == TYPE_MAT2) {
        return 4;
    } else if (ty == TYPE_MAT3) {
        return 9;
    } else if (ty == TYPE_MAT4) {
        return 16;
    } else {
        // Unknown component type
        return -1;
    }
}

enum class GLTF_TYPE : u8 {
    GLTF_NULL,
    GLTF_REAL,
    GLTF_INT,
    GLTF_BOOL,
    GLTF_STRING,
    GLTF_ARRAY,
    GLTF_BINARY,
    GLTF_OBJECT
};

using ColorValue = std::array<double, 4>;

class Value {
public:
    using Array  = std::vector<Value>;
    using Object = std::map<std::string, Value>;

    Value() = default;

    explicit Value(bool b) noexcept : type_(GLTF_TYPE::GLTF_BOOL), boolean_value_(b) {};
    explicit Value(int i) noexcept : type_(GLTF_TYPE::GLTF_INT), int_value_(i), real_value_(i) {};

    explicit Value(double n) noexcept : type_(GLTF_TYPE::GLTF_REAL), real_value_(n) {};
    explicit Value(const std::string &s) noexcept : type_(GLTF_TYPE::GLTF_STRING), string_value_(s) {};
    explicit Value(STextView s) noexcept
        : type_(GLTF_TYPE::GLTF_STRING), string_value_(std::move(s)) {};
    explicit Value(const char *s) noexcept : type_(GLTF_TYPE::GLTF_STRING), string_value_(s) {};
    explicit Value(const unsigned char *p, size_t n) noexcept : type_(GLTF_TYPE::GLTF_BINARY) {
        binary_value_.resize(n);
        memcpy(binary_value_.data(), p, n);
    }
    explicit Value(std::vector<unsigned char> &&v) noexcept
        : type_(GLTF_TYPE::GLTF_BINARY),
          binary_value_(std::move(v)) {}
    explicit Value(const Array &a) noexcept : type_(GLTF_TYPE::GLTF_ARRAY), array_value_(a) {};
    explicit Value(Array &&a) noexcept : type_(GLTF_TYPE::GLTF_ARRAY),
                                         array_value_(std::move(a)) {}

    explicit Value(const Object &o) noexcept : type_(GLTF_TYPE::GLTF_OBJECT), object_value_(o) {};
    explicit Value(Object &&o) noexcept : type_(GLTF_TYPE::GLTF_OBJECT),
                                          object_value_(std::move(o)) {};

    DEFAULT_METHODS(Value);

    [[nodiscard]] char Type() const noexcept { return static_cast<char>(type_); }

    [[nodiscard]] bool IsBool() const noexcept { return (type_ == GLTF_TYPE::GLTF_BOOL); }

    [[nodiscard]] bool IsInt() const noexcept { return (type_ == GLTF_TYPE::GLTF_INT); }

    [[nodiscard]] bool IsNumber() const noexcept { return (type_ == GLTF_TYPE::GLTF_REAL) || (type_ == GLTF_TYPE::GLTF_INT); }

    [[nodiscard]] bool IsReal() const noexcept { return (type_ == GLTF_TYPE::GLTF_REAL); }

    [[nodiscard]] bool IsString() const noexcept { return (type_ == GLTF_TYPE::GLTF_STRING); }

    [[nodiscard]] bool IsBinary() const noexcept { return (type_ == GLTF_TYPE::GLTF_BINARY); }

    [[nodiscard]] bool IsArray() const noexcept { return (type_ == GLTF_TYPE::GLTF_ARRAY); }

    [[nodiscard]] bool IsObject() const noexcept { return (type_ == GLTF_TYPE::GLTF_OBJECT); }

    // Use this function if you want to have number value as double.
    [[nodiscard]] double GetNumberAsDouble() const noexcept {
        if (type_ == GLTF_TYPE::GLTF_INT) {
            return double(int_value_);
        } else {
            return real_value_;
        }
    }

    // Use this function if you want to have number value as int.
    // TODO(syoyo): Support int value larger than 32 bits
    [[nodiscard]] int GetNumberAsInt() const noexcept {
        if (type_ == GLTF_TYPE::GLTF_REAL) {
            return int(real_value_);
        } else {
            return int_value_;
        }
    }

    // Accessor
    template <typename T>
    const T &Get() const;
    template <typename T>
    T &Get();

    // Lookup value from an array
    [[nodiscard]] const Value &Get(size_t idx) const {
        static Value null_value;
        assert(IsArray());
        return (idx < array_value_.size())
                   ? array_value_[idx]
                   : null_value;
    }

    // Lookup value from a key-value pair
    [[nodiscard]] const Value &Get(const std::string &key) const {
        static Value null_value;
        assert(IsObject());

        return object_value_.contains(key) ? object_value_.rbegin()->second : null_value;
    }

    [[nodiscard]] size_t ArrayLen() const {
        if (!IsArray())
            return 0;
        return array_value_.size();
    }

    // Valid only for object type.
    [[nodiscard]] bool Has(const std::string &key) const {
        if (!IsObject())
            return false;
        return object_value_.contains(key);
    }

    // List keys
    [[nodiscard]] std::vector<std::string> Keys() const {
        std::vector<std::string> keys;
        if (!IsObject())
            return keys; // empty

        for (auto &c : object_value_) {
            keys.push_back(c.first);
        }

        return keys;
    }

    [[nodiscard]] size_t Size() const noexcept { return (IsArray() ? ArrayLen() : Keys().size()); }

    bool operator==(const Value &other) const;

private:
    GLTF_TYPE type_ = GLTF_TYPE::GLTF_NULL;

    int                        int_value_  = 0;
    double                     real_value_ = 0.0;
    SString                    string_value_{};
    std::vector<unsigned char> binary_value_{};
    Array                      array_value_{};
    Object                     object_value_{};
    bool                       boolean_value_ = false;
};

struct Parameter {

    Parameter() = default;
    DEFAULT_METHODS(Parameter)
    bool operator==(const Parameter &) const;

    std::vector<double>       number_array;
    std::map<SString, double> json_double_value;

    SString string_value;
    double  number_value     = 0.0;
    bool    bool_value       = false;
    bool    has_member_value = false;

    [[nodiscard]] int TextureIndex() const noexcept {
        const auto it = json_double_value.find("index");
        return it != json_double_value.end() ? (int)it->second : -1;
    }

    [[nodiscard]] int TextureTexCoord() const noexcept {
        const auto it = json_double_value.find("texCoord");
        return it != json_double_value.end() ? (int)it->second : 0;
    }

    [[nodiscard]] double TextureScale() const noexcept {
        const auto it = json_double_value.find("scale");
        return it != json_double_value.end() ? it->second : 1;
    }

    [[nodiscard]] double TextureStrength() const noexcept {
        const auto it = json_double_value.find("strength");
        return it != json_double_value.end() ? it->second : 1;
    }

    [[nodiscard]] double Factor() const noexcept { return number_value; }

    [[nodiscard]] ColorValue ColorFactor() const noexcept {
        return {// this aggregate initialize the std::array object, and uses C++11 RVO.
                number_array[0],
                number_array[1],
                number_array[2],
                (number_array.size() > 3 ? number_array[3] : 1.0)};
    }
};

using ParameterMap = std::map<SString, Parameter>;
using ExtensionMap = std::map<SString, Value>;

struct GltfAnimationChannel {
    int sampler{-1};
    int targetNode{-1};

    SString targetPath{};

    Value        extras;
    ExtensionMap extensions;
    Value        targetExtras;
    ExtensionMap targetExtensions;

    // 拓展
    SString extransJsonString;
    SString extensionsJsonString;
    SString TargetExtrasJsonString;
    SString TargetExtensionsJsonString;

    GltfAnimationChannel() = default;
    DEFAULT_METHODS(GltfAnimationChannel)
    bool operator==(const GltfAnimationChannel &) const;
};

struct GltfAnimationSampler {
    int     input{-1};     // required
    int     output{-1};    // required
    SString interpolation; // "LINEAR", "STEP","CUBICSPLINE" or user defined
                           // string. default "LINEAR"
    Value        extras;
    ExtensionMap extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    SString extras_json_string;
    SString extensions_json_string;

    GltfAnimationSampler() : interpolation("LINEAR") {}
    DEFAULT_METHODS(GltfAnimationSampler)
    bool operator==(const GltfAnimationSampler &) const;
};

struct GltfAnimation {
    SString                           name;
    std::vector<GltfAnimationChannel> channels;
    std::vector<GltfAnimationSampler> samplers;
    Value                             extras;
    ExtensionMap                      extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    SString extras_json_string;
    SString extensions_json_string;

    GltfAnimation() = default;
    DEFAULT_METHODS(GltfAnimation)
    bool operator==(const GltfAnimation &) const;
};

struct GltfSkin {
    SString          name{};
    int              inverseBindMatrices{-1}; // required here but not in the spec
    int              skeleton{-1};            // The index of the node used as a skeleton root
    std::vector<int> joints;                  // Indices of skeleton nodes

    Value        extras;
    ExtensionMap extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    SString extras_json_string;
    SString extensions_json_string;

    GltfSkin() = default;
    DEFAULT_METHODS(GltfSkin)
    bool operator==(const GltfSkin &) const;
};

struct GltfSampler {
    SString name;
    // glTF 2.0 spec does not define default value for `minFilter` and
    // `magFilter`. Set -1 in TinyGLTF(issue #186)
    int minFilter =
        -1; // optional. -1 = no filter defined. ["NEAREST", "LINEAR",
            // "NEAREST_MIPMAP_NEAREST", "LINEAR_MIPMAP_NEAREST",
            // "NEAREST_MIPMAP_LINEAR", "LINEAR_MIPMAP_LINEAR"]
    int magFilter =
        -1; // optional. -1 = no filter defined. ["NEAREST", "LINEAR"]
    int wrapS =
        TEXTURE_WRAP_REPEAT; // ["CLAMP_TO_EDGE", "MIRRORED_REPEAT",
                             // "REPEAT"], default "REPEAT"
    int wrapT =
        TEXTURE_WRAP_REPEAT; // ["CLAMP_TO_EDGE", "MIRRORED_REPEAT",
                             // "REPEAT"], default "REPEAT"
    // int wrapR = TINYGLTF_TEXTURE_WRAP_REPEAT;  // TinyGLTF extension. currently
    // not used.

    Value        extras;
    ExtensionMap extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    SString extras_json_string;
    SString extensions_json_string;

    GltfSampler() = default;
    DEFAULT_METHODS(GltfSampler)
    bool operator==(const GltfSampler &) const;
};

struct GltfImage {

    GltfImage() = default;
    DEFAULT_METHODS(GltfImage)
    bool operator==(const GltfImage &) const;

    SString name{};
    int     width{};
    int     height{};
    int     component{};
    int     bits{};
    int     pixel_type{};

    std::vector<unsigned char> image;
    int                        bufferView{};

    SString mimeType;
    SString uri;

    Value        extras;
    ExtensionMap extensions;

    SString extras_json_string;
    SString extensions_json_string;

    bool as_is{false};
};

struct GltfTexture {
    GltfTexture() = default;
    DEFAULT_METHODS(GltfTexture)
    bool operator==(const GltfTexture &) const;

    SString name;

    int sampler{};
    int source{};

    Value        extras;
    ExtensionMap extensions;

    SString extrasJsonString;
    SString extensionsJsonString;
};

struct GltfTextureInfo {
    int index{-1};   // required.
    int texCoord{0}; // The set index of texture's TEXCOORD attribute used for
                     // texture coordinate mapping.

    Value        extras;
    ExtensionMap extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    GltfTextureInfo() = default;
    DEFAULT_METHODS(GltfTextureInfo)
    bool operator==(const GltfTextureInfo &) const;
};

struct GltfNormalTextureInfo {
    int index{-1};   // required
    int texCoord{0}; // The set index of texture's TEXCOORD attribute used for
                     // texture coordinate mapping.
    double scale{
        1.0}; // scaledNormal = normalize((<sampled normal texture value>
              // * 2.0 - 1.0) * vec3(<normal scale>, <normal scale>, 1.0))

    Value        extras;
    ExtensionMap extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    GltfNormalTextureInfo() = default;
    DEFAULT_METHODS(GltfNormalTextureInfo)
    bool operator==(const GltfNormalTextureInfo &) const;
};

struct GltfOcclusionTextureInfo {
    int index{-1};        // required
    int texCoord{0};      // The set index of texture's TEXCOORD attribute used for
                          // texture coordinate mapping.
    double strength{1.0}; // occludedColor = lerp(color, color * <sampled
                          // occlusion texture value>, <occlusion strength>)

    Value        extras;
    ExtensionMap extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    GltfOcclusionTextureInfo() = default;
    DEFAULT_METHODS(GltfOcclusionTextureInfo)
    bool operator==(const GltfOcclusionTextureInfo &) const;
};

// pbrMetallicRoughness class defined in glTF 2.0 spec.
struct GltfPbrMetallicRoughness {
    std::vector<double> baseColorFactor{1.0, 1.0, 1.0, 1.0}; // len = 4. default [1,1,1,1]
    GltfTextureInfo     baseColorTexture;
    double              metallicFactor{1.0};  // default 1
    double              roughnessFactor{1.0}; // default 1
    GltfTextureInfo     metallicRoughnessTexture;

    Value        extras;
    ExtensionMap extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    GltfPbrMetallicRoughness() = default;
    DEFAULT_METHODS(GltfPbrMetallicRoughness)

    bool operator==(const GltfPbrMetallicRoughness &) const;
};

struct GltfMaterial {
    std::string name;

    std::vector<double> emissiveFactor{0.0, 0.0, 0.0}; // length 3. default [0, 0, 0]
    std::string         alphaMode{"OPAQUE"};           // default "OPAQUE"
    double              alphaCutoff{0.5};              // default 0.5
    bool                doubleSided{false};            // default false
    std::vector<int>    lods;                          // level of detail materials (MSFT_lod)

    GltfPbrMetallicRoughness pbrMetallicRoughness;

    GltfNormalTextureInfo    normalTexture;
    GltfOcclusionTextureInfo occlusionTexture;
    GltfTextureInfo          emissiveTexture;

    // For backward compatibility
    // TODO(syoyo): Remove `values` and `additionalValues` in the next release.
    ParameterMap values;
    ParameterMap additionalValues;

    ExtensionMap extensions;
    Value        extras;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    GltfMaterial() = default;
    DEFAULT_METHODS(GltfMaterial)

    bool operator==(const GltfMaterial &) const;
};

struct GltfBufferView {
    std::string name;
    int         buffer{-1};    // Required
    size_t      byteOffset{0}; // minimum 0, default 0
    size_t      byteLength{0}; // required, minimum 1. 0 = invalid
    size_t      byteStride{0}; // minimum 4, maximum 252 (multiple of 4), default 0 =
                               // understood to be tightly packed
    int target{0};             // ["ARRAY_BUFFER", "ELEMENT_ARRAY_BUFFER"] for vertex indices
                               // or attribs. Could be 0 for other data
    Value        extras;
    ExtensionMap extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    bool dracoDecoded{false}; // Flag indicating this has been draco decoded

    GltfBufferView() = default;
    DEFAULT_METHODS(GltfBufferView)
    bool operator==(const GltfBufferView &) const;
};

struct GltfAccessor {
    int bufferView{-1}; // optional in spec but required here since sparse
                        // accessor are not supported
    std::string  name;
    size_t       byteOffset{0};
    bool         normalized{false}; // optional.
    int          componentType{-1}; // (required) One of TINYGLTF_COMPONENT_TYPE_***
    size_t       count{0};          // required
    int          type{-1};          // (required) One of TINYGLTF_TYPE_***   ..
    Value        extras;
    ExtensionMap extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    std::vector<double>
        minValues; // optional. integer value is promoted to double
    std::vector<double>
        maxValues; // optional. integer value is promoted to double

    struct Sparse {
        int  count{0};
        bool isSparse{false};
        struct {
            size_t       byteOffset{0};
            int          bufferView{-1};
            int          componentType{-1}; // a TINYGLTF_COMPONENT_TYPE_ value
            Value        extras;
            ExtensionMap extensions;
            std::string  extras_json_string;
            std::string  extensions_json_string;
        } indices;
        struct {
            int          bufferView{-1};
            size_t       byteOffset{0};
            Value        extras;
            ExtensionMap extensions;
            std::string  extras_json_string;
            std::string  extensions_json_string;
        } values;
        Value        extras;
        ExtensionMap extensions;
        std::string  extras_json_string;
        std::string  extensions_json_string;
    };

    Sparse sparse;

    ///
    /// Utility function to compute byteStride for a given bufferView object.
    /// Returns -1 upon invalid glTF value or parameter configuration.
    ///
    [[nodiscard]] int ByteStride(const GltfBufferView &bufferViewObject) const {
        if (bufferViewObject.byteStride == 0) {
            // Assume data is tightly packed.
            int componentSizeInBytes =
                GetComponentSizeInBytes(static_cast<uint32_t>(componentType));
            if (componentSizeInBytes <= 0) {
                return -1;
            }

            int numComponents = GetNumComponentsInType(static_cast<uint32_t>(type));
            if (numComponents <= 0) {
                return -1;
            }

            return componentSizeInBytes * numComponents;
        } else {
            // Check if byteStride is a multiple of the size of the accessor's
            // component type.
            int componentSizeInBytes =
                GetComponentSizeInBytes(static_cast<uint32_t>(componentType));
            if (componentSizeInBytes <= 0) {
                return -1;
            }

            if ((bufferViewObject.byteStride % uint32_t(componentSizeInBytes)) != 0) {
                return -1;
            }
            return static_cast<int>(bufferViewObject.byteStride);
        }

        // unreachable return 0;
    }

    GltfAccessor() = default;
    DEFAULT_METHODS(GltfAccessor)
    bool operator==(const GltfAccessor &) const;
};

struct GltfPrimitive {
    std::map<std::string, int> attributes;               // (required) A dictionary object of
                                                         // integer, where each integer
                                                         // is the index of the accessor
                                                         // containing an attribute.
    int material{-1};                                    // The index of the material to apply to this primitive
                                                         // when rendering.
    int                                     indices{-1}; // The index of the accessor that contains the indices.
    int                                     mode{-1};    // one of TINYGLTF_MODE_***
    std::vector<std::map<std::string, int>> targets;     // array of morph targets,
    // where each target is a dict with attributes in ["POSITION, "NORMAL",
    // "TANGENT"] pointing
    // to their corresponding accessors
    ExtensionMap extensions;
    Value        extras;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    GltfPrimitive() = default;
    DEFAULT_METHODS(GltfPrimitive)
    bool operator==(const GltfPrimitive &) const;
};

struct GltfMesh {
    std::string                name;
    std::vector<GltfPrimitive> primitives;
    std::vector<double>        weights; // weights to be applied to the Morph Targets
    ExtensionMap               extensions;
    Value                      extras;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    GltfMesh() = default;
    DEFAULT_METHODS(GltfMesh)
    bool operator==(const GltfMesh &) const;
};

class GltfNode {
public:
    GltfNode() = default;

    DEFAULT_METHODS(GltfNode)

    bool operator==(const GltfNode &) const;

    int camera{-1}; // the index of the camera referenced by this node

    std::string         name;
    int                 skin{-1};
    int                 mesh{-1};
    int                 light{-1};   // light source index (KHR_lights_punctual)
    int                 emitter{-1}; // audio emitter index (KHR_audio)
    std::vector<int>    lods;        // level of detail nodes (MSFT_lod)
    std::vector<int>    children;
    std::vector<double> rotation;    // length must be 0 or 4
    std::vector<double> scale;       // length must be 0 or 3
    std::vector<double> translation; // length must be 0 or 3
    std::vector<double> matrix;      // length must be 0 or 16
    std::vector<double> weights;     // The weights of the instantiated Morph Target

    ExtensionMap extensions;
    Value        extras;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;
};

struct GltfBuffer {
    std::string                name;
    std::vector<unsigned char> data;
    std::string
        uri; // considered as required here but not in the spec (need to clarify)
             // uri is not decoded(e.g. whitespace may be represented as %20)
    Value        extras;
    ExtensionMap extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    GltfBuffer() = default;
    DEFAULT_METHODS(GltfBuffer)
    bool operator==(const GltfBuffer &) const;
};

struct GltfAsset {
    std::string  version = "2.0"; // required
    std::string  generator;
    std::string  minVersion;
    std::string  copyright;
    ExtensionMap extensions;
    Value        extras;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    GltfAsset() = default;
    DEFAULT_METHODS(GltfAsset)
    bool operator==(const GltfAsset &) const;
};

struct GltfScene {
    std::string      name;
    std::vector<int> nodes;
    std::vector<int> audioEmitters; // KHR_audio global emitters

    ExtensionMap extensions;
    Value        extras;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;

    GltfScene() = default;
    DEFAULT_METHODS(GltfScene)
    bool operator==(const GltfScene &) const;
};

struct GltfPositionalEmitter {
    double coneInnerAngle{6.283185307179586};
    double coneOuterAngle{6.283185307179586};
    double coneOuterGain{0.0};
    double maxDistance{100.0};
    double refDistance{1.0};
    double rolloffFactor{1.0};

    GltfPositionalEmitter() = default;
    DEFAULT_METHODS(GltfPositionalEmitter)
    bool operator==(const GltfPositionalEmitter &) const;

    ExtensionMap extensions;
    Value        extras;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;
};

class GltfModel {
public:
    GltfModel() = default;
    DEFAULT_METHODS(GltfModel)

    bool operator==(const GltfModel &) const;

    std::vector<GltfAccessor>   accessors;
    std::vector<GltfAnimation>  animations;
    std::vector<GltfBuffer>     buffers;
    std::vector<GltfBufferView> bufferViews;
    std::vector<GltfMaterial>   materials;
    std::vector<GltfMesh>       meshes;
    std::vector<GltfNode>       nodes;
    std::vector<GltfTexture>    textures;
    std::vector<GltfImage>      images;
    std::vector<GltfSkin>       skins;
    std::vector<GltfSampler>    samplers;
    std::vector<GltfScene>      scenes;

    int                      defaultScene{-1};
    std::vector<std::string> extensionsUsed;
    std::vector<std::string> extensionsRequired;

    GltfAsset asset;

    Value        extras;
    ExtensionMap extensions;

    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
    std::string extras_json_string;
    std::string extensions_json_string;
};

enum class SectionCheck : s8 {
    NO_REQUIRE           = 0x00,
    REQUIRE_VERSION      = 0x01,
    REQUIRE_SCENE        = 0x02,
    REQUIRE_SCENES       = 0x04,
    REQUIRE_NODES        = 0x08,
    REQUIRE_ACCESSORS    = 0x10,
    REQUIRE_BUFFERS      = 0x20,
    REQUIRE_BUFFER_VIEWS = 0x40,
    REQUIRE_ALL          = 0x7f
};



struct URICallbacks {

  util::EventHandle<STextView,STextView,SString&,void*> encode;  // Optional encode method
  util::EventHandle<STextView,SString&,void*> decode;  // Required decode method

  void *user_data;  // An argument that is passed to all uri callbacks
};
}; // namespace shine::gltf