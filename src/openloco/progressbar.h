#pragma once

#include <cstdint>

namespace openloco::progressbar
{
    void begin(int32_t maximum, int32_t edx);
    void increment(int32_t value);
    void end();
}
