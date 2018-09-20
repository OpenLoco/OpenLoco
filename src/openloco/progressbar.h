#pragma once

#include "localisation/stringmgr.h"
#include <cstdint>

namespace openloco
{
    class progressbar
    {
    public:
        void begin(string_id stringId, int32_t edx);
        void set_progress(int32_t value);
        void end();
    };
}
