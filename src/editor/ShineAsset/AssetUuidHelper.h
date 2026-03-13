#pragma once
// ============================================================
//  AssetUuidHelper — UUID generation and validation for assets.
//
//  Bridges shine::algorithm::UUID (RFC 9562, 128-bit) to the
//  canonical string representation stored in .sasset files:
//      "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx"
//
//  Internal API uses SString / STextView (engine string rules).
//  The only std::string boundary is UUID::ToString() from the
//  algorithm library.
// ============================================================

#include <optional>
#include <string>   // external boundary — UUID::ToString() returns std::string

#include "Algorithm/uuid.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Generation
    // -----------------------------------------------------------------------

    /// Generate a new random V4 UUID string ("8-4-4-4-12" lower-case hex).
    [[nodiscard]] SString GenerateUUIDString();

    /// Generate a time-ordered V7 UUID string.
    /// Prefer for sub-assets created together; they sort by creation time.
    [[nodiscard]] SString GenerateV7UUIDString();

    // -----------------------------------------------------------------------
    //  Parsing
    // -----------------------------------------------------------------------

    /// Parse a UUID string and return the engine UUID type.
    /// Accepts both hyphenated ("8-4-4-4-12") and compact (32 hex chars) forms.
    /// Returns nullopt if the string is malformed.
    [[nodiscard]] std::optional<shine::algorithm::UUID>
    ParseUUIDString(STextView uuidStr);

    // -----------------------------------------------------------------------
    //  Conversion
    // -----------------------------------------------------------------------

    /// Convert an engine UUID to canonical lower-case hyphenated string.
    [[nodiscard]] SString UUIDToString(const shine::algorithm::UUID& id);

    /// Return true if the string is a syntactically valid UUID
    /// (does NOT check variant/version bits).
    [[nodiscard]] bool IsValidUUIDString(STextView s) noexcept;

    /// Return the nil UUID string ("00000000-0000-0000-0000-000000000000").
    [[nodiscard]] SString NilUUIDString();

} // namespace shine::editor::asset
