#ifdef SHINE_USE_MODULE

module;

#include "util/shine_define.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

module shine.util.string_util;

#else

#include "string_util.ixx"
#include "encoding_util.ixx"
#include "util/shine_define.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#endif

namespace shine::util
{
#ifdef _WIN32
    SString StringUtil::WstringToUTF8(std::wstring_view wstr)
    {
        return SString::from_utf8(EncodingUtil::WstringToUTF8(wstr));
    }

    std::wstring StringUtil::UTF8ToWstring(STextView u8str)
    {
        return EncodingUtil::UTF8ToWstring(u8str.sv());
    }

    SString StringUtil::ToNativeEncoding(STextView str)
    {
        const std::wstring wstr = EncodingUtil::UTF8ToWstring(str.sv());
        if (wstr.empty() && !str.empty())
        {
            return {};
        }
        return SString::from_utf8(EncodingUtil::WstringToANSI(wstr));
    }

    SString StringUtil::FromNativeEncoding(STextView str)
    {
        const std::wstring wstr = EncodingUtil::ANSIToWstring(str.sv());
        if (wstr.empty() && !str.empty())
        {
            return {};
        }
        return SString::from_utf8(EncodingUtil::WstringToUTF8(wstr));
    }
#endif
}
