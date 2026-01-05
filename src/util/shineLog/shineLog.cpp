#include "shineLog.h"
#include <iostream>

namespace shine::log
{
    void ShineLog::Log(const char* msg)
    {
        std::cout << "[ShineLog] " << msg << std::endl;
    }
}
