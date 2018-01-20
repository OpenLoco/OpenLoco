#pragma once

#include <cstdint>
#include "localisation/stringmgr.h"

namespace openloco::progressbar
{
    void begin(string_id stringId, int32_t edx);
    void set_progress(int32_t value);
    void end();
}
