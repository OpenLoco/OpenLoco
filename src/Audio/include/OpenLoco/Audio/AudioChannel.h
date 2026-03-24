#pragma once

#include <cstdint>

namespace OpenLoco::Audio
{
    enum class ChannelId : uint8_t
    {
        master = 0,
        music,
        effects,
        vehicles,
        ui,
        ambient,
        count
    };
}
