
#include <string>
#include <vector>

#if defined(__cplusplus) && __cplusplus >= 201703L && !defined(__EMSCRIPTEN__)
#include <string_view>
#endif

#ifdef __EMSCRIPTEN__
#include <cstdint>
#endif

namespace shine
{
    namespace base64
    {
        constexpr static char base64_chars[65] = { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };

        // 预计算的 base64 解码查找表
        static constexpr unsigned char base64_decode_table[256] = {
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

        static constexpr auto getBase64Index(unsigned char c) noexcept -> unsigned char
        {
            return base64_decode_table[c];
        }

        static constexpr auto isBase64Char(unsigned char c) noexcept -> bool
        {
            return base64_decode_table[c] != 255;
        }

        // 计算 base64 解码后的输出大小
#if defined(__cplusplus) && __cplusplus >= 201703L && !defined(__EMSCRIPTEN__)
        static constexpr auto getDecodedSize(std::string_view encoded) noexcept -> size_t
#else
        static constexpr auto getDecodedSize(const std::string& encoded) noexcept -> size_t
#endif
        {
            if (encoded.empty()) return 0;

            size_t padding = 0;
            if (encoded.size() >= 2 && encoded[encoded.size() - 1] == '=') padding++;
            if (encoded.size() >= 3 && encoded[encoded.size() - 2] == '=') padding++;

            return (encoded.size() * 3) / 4 - padding;
        }

        // 解码 4 个 base64 字符为 3 个字节
        static void decodeBlock(const unsigned char input[4], unsigned char output[3]) noexcept
        {
            unsigned char sixBitChars[4];
            for (int i = 0; i < 4; i++) {
                sixBitChars[i] = getBase64Index(input[i]);
            }

            output[0] = (sixBitChars[0] << 2) + ((sixBitChars[1] & 0x30) >> 4);
            output[1] = ((sixBitChars[1] & 0xf) << 4) + ((sixBitChars[2] & 0x3c) >> 2);
            output[2] = ((sixBitChars[2] & 0x3) << 6) + sixBitChars[3];
        }

        // 编码 3 个字节为 4 个 base64 字符
        static void encodeBlock(const unsigned char input[3], char output[4]) noexcept
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

#if defined(__cplusplus) && __cplusplus >= 201703L && !defined(__EMSCRIPTEN__)
        std::vector<unsigned char> base64_decode(std::string_view encoded_string) noexcept
#else
        std::vector<unsigned char> base64_decode(const std::string& encoded_string) noexcept
#endif
        {
            const size_t in_len = encoded_string.size();
            std::vector<unsigned char> ret;
            ret.reserve(getDecodedSize(encoded_string));

            size_t pos = 0;
            while (pos + 4 <= in_len) {
                // 检查是否有填充字符
                if (encoded_string[pos] == '=' || !isBase64Char(encoded_string[pos])) break;

                unsigned char char_array_4[4];
                for (int i = 0; i < 4; i++) {
                    char_array_4[i] = static_cast<unsigned char>(encoded_string[pos + i]);
                }

                unsigned char char_array_3[3];
                decodeBlock(char_array_4, char_array_3);

                for (int i = 0; i < 3; i++) {
                	ret.push_back(char_array_3[i]);
                }

                pos += 4;
            }

            // 处理最后可能不完整的块（包含填充）
            const size_t remaining = in_len - pos;
            if (remaining > 0 && remaining <= 4) {
                unsigned char char_array_4[4] = { 0 };
                size_t valid_chars = 0;

                // 复制有效字符，跳过填充字符
                for (size_t i = 0; i < remaining; i++) {
                    if (encoded_string[pos + i] != '=' && isBase64Char(encoded_string[pos + i])) {
                        char_array_4[valid_chars++] = static_cast<unsigned char>(encoded_string[pos + i]);
                    }
                }

                if (valid_chars >= 2) { // 至少需要2个有效字符
                    unsigned char char_array_3[3];
                    decodeBlock(char_array_4, char_array_3);

                    // 根据有效字符数决定输出字节数
                    size_t output_bytes = (valid_chars * 3) / 4;
                    for (size_t i = 0; i < output_bytes; i++) {
                        ret.push_back(char_array_3[i]);
                    }
                }
            }

            return ret;
        }


        std::string base64_encode(const std::vector<unsigned char>& data) noexcept {
            const size_t data_len = data.size();
            std::string ret;
            ret.reserve((data_len + 2) / 3 * 4);

            // 处理完整的 3 字节块
            for (size_t pos = 0; pos + 3 <= data_len; pos += 3) {
                unsigned char char_array_3[3];
                for (int i = 0; i < 3; i++) {
                    char_array_3[i] = data[pos + i];
                }

                char char_array_4[4];
                encodeBlock(char_array_3, char_array_4);

                for (int i = 0; i < 4; i++) {
                    ret += char_array_4[i];
                }
            }

            // 处理最后可能不完整的块
            const size_t remaining = data_len % 3;
            if (remaining > 0) {
                unsigned char char_array_3[3] = { 0 };
                for (size_t i = 0; i < remaining; i++) {
                    char_array_3[i] = data[data_len - remaining + i];
                }

                char char_array_4[4];
                encodeBlock(char_array_3, char_array_4);

                // 添加编码后的字符
                for (size_t i = 0; i < remaining + 1; i++) {
                    ret += char_array_4[i];
                }

                // 添加填充字符
                for (size_t i = remaining; i < 3; i++) {
                    ret += '=';
                }
            }

            return ret;
        }

#if defined(__cplusplus) && __cplusplus >= 201703L && !defined(__EMSCRIPTEN__)
        std::string base64_encode(std::string_view data) noexcept
        {
            return base64_encode(std::vector<unsigned char>(data.begin(), data.end()));
        }
#endif


#ifdef __EMSCRIPTEN__

        // WASM-specific overloads for better JavaScript interop
        std::string base64_encode(const std::string& data)
        {
            return base64_encode(std::vector<unsigned char>(data.begin(), data.end()));
        }

        std::vector<unsigned char> base64_decode(const std::string& encoded_string)
        {
            return base64_decode(std::string_view(encoded_string));
        }

        std::string base64_encode(const char* data, size_t length)
        {
            if (!data || length == 0) return {};
            return base64_encode(std::vector<unsigned char>(data, data + length));
        }

        std::string base64_encode(const uint8_t* data, size_t length)
        {
            if (!data || length == 0) return {};
            return base64_encode(std::vector<unsigned char>(data, data + length));
        }

        std::vector<uint8_t> base64_decode_to_uint8(const std::string& encoded_string)
        {
            auto result = base64_decode(std::string_view(encoded_string));
            return std::vector<uint8_t>(result.begin(), result.end());
        }

#endif
    }
}


