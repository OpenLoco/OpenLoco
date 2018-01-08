#pragma once

#include <cstdint>

namespace openloco::ui::windowmgr
{
    #pragma pack(push, 1)

    struct window
    {
        uint8_t pad_00[0x88E];
    };

    #pragma pack(pop)
}
