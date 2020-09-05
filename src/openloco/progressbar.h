#pragma once

#include "localisation/stringmgr.h"
#include <cstdint>

namespace openloco::progressbar
{
    void begin(string_id stringId, int32_t edx);
    void setProgress(int32_t value);
    void end();
}
