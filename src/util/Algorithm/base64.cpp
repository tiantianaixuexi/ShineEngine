#ifdef SHINE_USE_MODULE

module shine.base64;

#else

#include "base64.ixx"

#endif

namespace shine::base64 {

    namespace { // Anonymous namespace for internal linkage

        constexpr char base64_chars[65] = { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };

        // Precomputed base64 decode table
        constexpr unsigned char base64_decode_table[256] = {
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,62,255,255,255,63,
            52,53,54,55,56,57,58,59,60,61,255,255,255,255,255,255,
            255,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,23,24,25,255,255,255,255,255,
            255,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,49,50,51,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        };

        constexpr unsigned char getBase64Index(unsigned char c) noexcept
        {
            return base64_decode_table[c];
        }

        constexpr bool isBase64Char(unsigned char c) noexcept
        {
            return base64_decode_table[c] != 255;
        }

        // Calculate decoded size
        constexpr size_t getDecodedSize(std::string_view encoded) noexcept
        {
            if (encoded.empty()) return 0;

            size_t padding = 0;
            if (encoded.size() >= 2 && encoded[encoded.size() - 1] == '=') padding++;
            if (encoded.size() >= 3 && encoded[encoded.size() - 2] == '=') padding++;

            return (encoded.size() * 3) / 4 - padding;
        }

        // Decode 4 chars to 3 bytes
        void decodeBlock(const unsigned char input[4], unsigned char output[3]) noexcept
        {
            unsigned char sixBitChars[4];
            for (int i = 0; i < 4; i++) {
                sixBitChars[i] = getBase64Index(input[i]);
            }

            output[0] = (sixBitChars[0] << 2) + ((sixBitChars[1] & 0x30) >> 4);
            output[1] = ((sixBitChars[1] & 0xf) << 4) + ((sixBitChars[2] & 0x3c) >> 2);
            output[2] = ((sixBitChars[2] & 0x3) << 6) + sixBitChars[3];
        }

        // Encode 3 bytes to 4 chars
        void encodeBlock(const unsigned char input[3], char output[4]) noexcept
        {
            unsigned char char_array_4[4];
            char_array_4[0] = (input[0] & 0xfc) >> 2;
            char_array_4[1] = ((input[0] & 0x03) << 4) + ((input[1] & 0xf0) >> 4);
            char_array_4[2] = ((input[1] & 0x0f) << 2) + ((input[2] & 0xc0) >> 6);
            char_array_4[3] = input[2] & 0x3f;

            for (int i = 0; i < 4; i++) {
                output[i] = base64_chars[char_array_4[i]];
            }
        }
    } // namespace

    std::string base64_decode(std::string_view encoded_string) noexcept
    {
        if (encoded_string.empty()) return {};

        const size_t in_len = encoded_string.size();
        
        // Optimize: Allocate once
        std::string ret(getDecodedSize(encoded_string), '\0');
        unsigned char* out_ptr = reinterpret_cast<unsigned char*>(ret.data());
        size_t out_idx = 0;

        size_t pos = 0;
        while (pos + 4 <= in_len) {
            // Check for padding or invalid chars early
            if (encoded_string[pos] == '=' || !isBase64Char(encoded_string[pos])) break;

            unsigned char char_array_4[4];
            for (int i = 0; i < 4; i++) {
                char_array_4[i] = static_cast<unsigned char>(encoded_string[pos + i]);
            }

            unsigned char char_array_3[3];
            decodeBlock(char_array_4, char_array_3);

            // Direct write
            if (out_idx + 3 <= ret.size()) {
                out_ptr[out_idx++] = char_array_3[0];
                out_ptr[out_idx++] = char_array_3[1];
                out_ptr[out_idx++] = char_array_3[2];
            }

            pos += 4;
        }

        // Handle remaining incomplete block
        const size_t remaining = in_len - pos;
        if (remaining > 0 && remaining <= 4) {
            unsigned char char_array_4[4] = { 0 };
            size_t valid_chars = 0;

            for (size_t i = 0; i < remaining; i++) {
                if (encoded_string[pos + i] != '=' && isBase64Char(encoded_string[pos + i])) {
                    char_array_4[valid_chars++] = static_cast<unsigned char>(encoded_string[pos + i]);
                }
            }

            if (valid_chars >= 2) {
                unsigned char char_array_3[3];
                decodeBlock(char_array_4, char_array_3);

                const size_t output_bytes = (valid_chars * 3) / 4;
                for (size_t i = 0; i < output_bytes; i++) {
                     if (out_idx < ret.size()) {
                        out_ptr[out_idx++] = char_array_3[i];
                     }
                }
            }
        }
        
        // Safety resize if calculation was slightly off (though it shouldn't be)
        if (out_idx != ret.size()) {
            ret.resize(out_idx);
        }

        return ret;
    }

    std::string base64_encode(std::string_view data) noexcept
    {
        if (data.empty()) return {};

        const size_t data_len = data.size();
        
        // Optimize: Allocate once
        size_t estimated_len = (data_len + 2) / 3 * 4;
        std::string ret(estimated_len, '\0');
        char* out_ptr = ret.data();
        size_t out_idx = 0;

        // Process full blocks
        for (size_t pos = 0; pos + 3 <= data_len; pos += 3) {
            unsigned char char_array_3[3];
            for (int i = 0; i < 3; i++) {
                char_array_3[i] = data[pos + i];
            }

            char char_array_4[4];
            encodeBlock(char_array_3, char_array_4);

            out_ptr[out_idx++] = char_array_4[0];
            out_ptr[out_idx++] = char_array_4[1];
            out_ptr[out_idx++] = char_array_4[2];
            out_ptr[out_idx++] = char_array_4[3];
        }

        // Process remaining
        if (const size_t remaining = data_len % 3; remaining > 0) {

            unsigned char char_array_3[3] = { 0 };
            for (size_t i = 0; i < remaining; i++) {
                char_array_3[i] = data[data_len - remaining + i];
            }

            char char_array_4[4];
            encodeBlock(char_array_3, char_array_4);

            for (size_t i = 0; i < remaining + 1; i++) {
                out_ptr[out_idx++] = char_array_4[i];
            }

            for (size_t i = remaining; i < 3; i++) {
                out_ptr[out_idx++] = '=';
            }
        }

        return ret;
    }

}
