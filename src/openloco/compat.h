#pragma once

#if defined(COMPAT_STD_BYTE)
#include <cstdint>

namespace std
{
    enum class byte : uint8_t
    {
    };
}
#endif
