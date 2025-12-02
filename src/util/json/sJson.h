#pragma once

#include "shine_define.h"


namespace shine::json
{

    enum class JsonType : u8 {
        None = 0,   // 000
        Raw  = 1,   // 001
        Null = 2,   // 010
        Bool = 3,   // 011
        Num  = 4,   // 100
        Str  = 5,   // 101
        Arr  = 6,   // 110
        Obj  = 7,   // 111
    };

    enum class JsonSubtype : u8 {
        None  = (0u << 3),   // ___00___
        False = (0u << 3),   // ___00___ 
        True  = (1u << 3),
    
        Uint  = (0u << 3),
        Sint  = (1u << 3),
        Real  = (2u << 3),
    
        NoEsc = (1u << 3),
    };

    constexpr u8 JSON_TYPE_MASK      = 0x07; // 00000111
    constexpr u8 JSON_TYPE_BIT       = 3;
    constexpr u8 JSON_SUBTYPE_MASK   = 0x18; // 00011000
    constexpr u8 JSON_SUBTYPE_BIT    = 2;
    constexpr u8 JSON_RESERVED_MASK  = 0xE0; // 11100000
    constexpr u8 JSON_RESERVED_BIT   = 3;
    constexpr u8 JSON_TAG_MASK       = 0xFF;
    constexpr u8 JSON_TAG_BIT        = 8;

    constexpr u8 JSON_PADDING_SIZE   = 4;


    constexpr u32 SJSON_READ_NOFLAG = 0;
    constexpr u32 SJSON_READ_INSITU = 1 << 0;
	constexpr u32 SJSON_READ_STOP_WHEN_NODE = 1 << 1;
	constexpr u32 SJSON_READ_ALLOW_TRAILING_COMMAS = 1 << 2;
	constexpr u32 SJSON_READ_ALLOW_COMMENTS = 1 << 3;
    constexpr u32 SJSON_READ_ALLOW_INF_AND_NAN = 1 << 4;
    constexpr u32 SJSON_READ_NUMBER_AS_RAW = 1 << 5;
    constexpr u32 SJSON_READ_ALLOW_INVALID_UNICODE = 1 << 6;
    constexpr u32 SJSON_READ_BIGNUM_AS_RAW = 1 << 7;
    constexpr u32 SJSON_READ_ALLOW_BOM = 1 << 8;
    constexpr u32 SJSON_READ_ADDLOW_EXT_NUMBER = 1 << 9;
    constexpr u32 SJSON_READ_ALLOW_EXT_ESCAPE = 1 << 10;
    constexpr u32 SJSON_READ_ALLOW_EXT_WHITESPACE = 1 << 11;
    constexpr u32 SJSON_READ_ALLOW_SIGNLE_QUOTED_STR = 1 << 12;
	constexpr u32 SJSON_READ_ALLOW_UNQUOTED_KEY = 1 << 13;
    constexpr u32 SJSON_READ_JSON5 =
        (1 << 2) | /* YYJSON_READ_ALLOW_TRAILING_COMMAS */
        (1 << 3) | /* YYJSON_READ_ALLOW_COMMENTS */
        (1 << 4) | /* YYJSON_READ_ALLOW_INF_AND_NAN */
        (1 << 9) | /* YYJSON_READ_ALLOW_EXT_NUMBER */
        (1 << 10) | /* YYJSON_READ_ALLOW_EXT_ESCAPE */
        (1 << 11) | /* YYJSON_READ_ALLOW_EXT_WHITESPACE */
        (1 << 12) | /* YYJSON_READ_ALLOW_SINGLE_QUOTED_STR */
        (1 << 13);  /* YYJSON_READ_ALLOW_UNQUOTED_KEY */


    // 8 bytes
    struct sjson_val_uni
    {
        union
        {
            u64  u64_val;
			s64  i64_val;
            f64  f64_val;
            const char* str_val;
			void* ptr_val;
            size_t  ofs_val;
        };
    };

    // 16 bytes.
    struct sjson_val
    {
        u64           tag; // type, subtype and length 
        sjson_val_uni uni;
    };

    struct sjson_doc
    {
        sjson_val* root;
        /** 解析JSON时读取的总字节数 */
        size_t dat_read;
        /** 解析JSON时读取的值的总数. */
        size_t val_read;
        /** The string pool used by JSON values (nullable). */
        char* str_pool;
    };

    static inline sjson_doc* sjson_read(const char* dat,size_t len,)

}