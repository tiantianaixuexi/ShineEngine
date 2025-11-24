
#pragma once

#include <vector>
#include <string>

#ifndef __EMSCRIPTEN__
   #include <string_view>
#endif


namespace shine::base64
{
     std::string base64_encode(const std::vector<unsigned char>& data) noexcept;
     std::string base64_encode(std::string_view data) noexcept;
     std::vector<unsigned char> base64_decode(std::string_view encoded_string) noexcept;

#ifdef __EMSCRIPTEN__
    // WASM-specific overloads for better JavaScript interop
     std::string base64_encode(const std::string& data);
     std::vector<unsigned char> base64_decode(const std::string& encoded_string);
     std::string base64_encode(const char* data, size_t length);
     std::string base64_encode(const uint8_t* data, size_t length);
     std::vector<uint8_t> base64_decode_to_uint8(const std::string& encoded_string);
#endif
}

