#pragma once

namespace openloco
{
    struct interface_skin_object
    {
        uint8_t pad_00[6];
        uint8_t colours_06[2];
        uint8_t pad_08[0x12 - 8];
        uint8_t colours_12[6];
    };
}
