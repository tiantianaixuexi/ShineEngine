#include "wasm_compat.h"



namespace shine::wasm{

extern "C" size_t my_strlen(const char* s)
{
    size_t n = 0;
    while (s[n]) ++n;  // 假设 s 是有效的以 '\0' 结尾的字符串
    return n;
}


}