#pragma once

#include <cstdint>

namespace openloco
{
    using string_id = uint32_t;

    namespace screen_flags
    {
        constexpr uint8_t title = 1 << 0;
        constexpr uint8_t editor = 1 << 1;
        constexpr uint8_t unknown_2 = 1 << 2;
    }

    void * hInstance();
    const char * lpCmdLine();
}
