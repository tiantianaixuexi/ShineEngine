
#pragma once

#include <vector>
#include <string>

#ifndef __EMSCRIPTEN__
   #include <string_view>
#endif


namespace shine::base64
{
    EXPORT_BASE64 std::string base64_encode(const std::vector<unsigned char>& data) noexcept;
    EXPORT_BASE64 std::string base64_encode(std::string_view data) noexcept;
    EXPORT_BASE64 std::vector<unsigned char> base64_decode(std::string_view encoded_string) noexcept;

#ifdef __EMSCRIPTEN__
    // WASM-specific overloads for better JavaScript interop
    EXPORT_BASE64 std::string base64_encode(const std::string& data);
    EXPORT_BASE64 std::vector<unsigned char> base64_decode(const std::string& encoded_string);
    EXPORT_BASE64 std::string base64_encode(const char* data, size_t length);
    EXPORT_BASE64 std::string base64_encode(const uint8_t* data, size_t length);
    EXPORT_BASE64 std::vector<uint8_t> base64_decode_to_uint8(const std::string& encoded_string);
#endif
}

