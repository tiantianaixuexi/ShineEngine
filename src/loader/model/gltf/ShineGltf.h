#pragma once

#include <span>
#include <expected>

#include "ShineGltfDefine.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::gltf {

enum class GLTF_ParseStricteness : u8 {
    Permissive,
    Strict
};

[[nodiscard]] inline bool IsDataURI(STextView in) noexcept {
    return in.starts_with("data:application/octet-stream;base64,") ||
           in.starts_with("data:image/jpeg;base64,")               ||
           in.starts_with("data:image/png;base64,")                ||
           in.starts_with("data:image/bmp;base64,")                ||
           in.starts_with("data:image/gif;base64,")                ||
           in.starts_with("data:text/plain;base64")                ||
           in.starts_with("data:application/gltf-buffer;base64,");
}

// ============================================================
// ShineGltf — self-contained glTF 2.0 / GLB loader
// ============================================================
class ShineGltf {
public:
    ShineGltf()  = default;
    ~ShineGltf() = default;

    ShineGltf(const ShineGltf&)            = delete;
    ShineGltf& operator=(const ShineGltf&) = delete;
    ShineGltf(ShineGltf&&)                 noexcept = default;
    ShineGltf& operator=(ShineGltf&&)      noexcept = default;

    // ---- loading API --------------------------------------------------

    /// Load from file path (ASCII .gltf or binary .glb).
    std::expected<bool, SString> LoadFromFile(
        STextView path,
        SectionCheck check_sections = SectionCheck::REQUIRE_VERSION);

    /// Load from an in-memory byte span.
    /// @param base_dir  Directory used to resolve relative URIs.
    std::expected<bool, SString> LoadFromMemory(
        std::span<const std::byte> data,
        STextView                  base_dir       = {},
        SectionCheck               check_sections = SectionCheck::REQUIRE_VERSION);

    // ---- result accessors ---------------------------------------------

    [[nodiscard]] const GltfModel& GetModel()  const noexcept { return model_; }
    [[nodiscard]] GltfModel&       GetModel()        noexcept { return model_; }
    [[nodiscard]] GltfModel        TakeModel()       noexcept { return std::move(model_); }

    // ---- options ------------------------------------------------------

    void SetImagesAsIs(bool v)              noexcept { images_as_is_             = v; }
    void SetPreserveImageChannels(bool v)   noexcept { preserve_image_channels_  = v; }
    void SetStrictness(GLTF_ParseStricteness s) noexcept { strictness_ = s; }

private:
    // Parse JSON string into model_.
    std::expected<bool, SString> ParseFromString(
        const char*  str,
        size_t       length,
        STextView    base_dir,
        SectionCheck check_sections);

    // Returns true when the data starts with the GLB magic 'glTF'.
    [[nodiscard]] static bool IsBinaryGlb(std::span<const std::byte> data) noexcept;

    // Parse a GLB container, extract JSON + BIN chunks, then call ParseFromString.
    std::expected<bool, SString> ParseGlb(
        std::span<const std::byte> data,
        STextView                  base_dir,
        SectionCheck               check_sections);

    // Resolve and load all buffer URIs referenced from the model.
    std::expected<bool, SString> LoadBuffers(STextView base_dir);

    // Resolve and load all image URIs referenced from the model.
    std::expected<bool, SString> LoadImages(STextView base_dir);

private:
    GltfModel             model_;
    SString               base_dir_;

    // Binary chunk embedded in a GLB file (subspan of the mapped file data).
    std::span<const std::byte> bin_chunk_;

    GLTF_ParseStricteness strictness_               = GLTF_ParseStricteness::Strict;
    bool                  images_as_is_             = false;
    bool                  preserve_image_channels_  = false;
};

} // namespace shine::gltf
