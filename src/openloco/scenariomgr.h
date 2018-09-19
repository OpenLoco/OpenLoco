#pragma once

#include <cstdint>

namespace openloco
{
    class scenariomanager
    {
    public:
        void load_index(uint8_t al);
    };

    extern scenariomanager g_scenariomgr;
}
