#pragma once

namespace openloco
{
    namespace screen_flags
    {
        constexpr uint8_t title = 1 << 0;
        constexpr uint8_t editor = 1 << 1;
        constexpr uint8_t unknown_2 = 1 << 2;
    }

    void * hInstance();
    const char * lpCmdLine();
}
