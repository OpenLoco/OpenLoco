#pragma once

#include <cstdint>

namespace openloco::ui
{
    #pragma pack(push, 1)

    struct widget
    {
        uint8_t pad_00[0x10];
    };

    struct window
    {
        union
        {
            uint8_t pad_all[0x88E];
            struct
            {
                uint8_t pad_00[0x0C];
                uint32_t enabled_widgets;   // 0x0C
                uint8_t pad_10[0x2C - 0x10];
                widget * widgets;           // 0x2C
                uint8_t pad_30[0x83E - 0x30];
                uint16_t var_83E;
                uint8_t pad_840[0x85A - 0x840];
                uint16_t var_85A;
                uint8_t pad_85C[0x886 - 0x85C];
                uint8_t var_886;
                uint8_t var_887;
            };
        };

        void sub_4CA17F();
    };

    #pragma pack(pop)
}
