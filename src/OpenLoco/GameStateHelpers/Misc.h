#pragma once

#include <cstdint>

namespace OpenLoco
{
    // 0x00525E28
    uint32_t& gameStateFlags();

    // 0x00525FB4
    uint8_t& gameStateCurrentSnowLine();

    // 0x0052622E
    uint16_t& gameStateVar416();
}
