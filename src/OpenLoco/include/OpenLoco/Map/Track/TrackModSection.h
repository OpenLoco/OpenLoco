#pragma once

#include <cstdint>

namespace OpenLoco::World::Track
{
    enum class ModSection : uint8_t
    {
        single = 0,
        block = 1,
        allConnected = 2,
    };
}
