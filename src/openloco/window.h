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
                uint32_t enabled_widgets; // 0x0C
                uint8_t pad_10[0x2C - 0x10];
                widget* widgets; // 0x2C
                uint8_t pad_30[0x40 - 0x30];
                uint16_t var_40;
                uint32_t var_42;
                uint8_t pad_46[0x83E - 0x46];
                uint16_t var_83E;
                uint8_t pad_840[0x85A - 0x840];
                uint16_t var_85A;
                uint8_t pad_85C[0x870 - 0x85C];
                uint16_t var_870;
                uint8_t pad_872[0x882 - 0x872];
                uint8_t type; // 0x882
                uint8_t pad_883[0x886 - 0x883];
                uint8_t colours[2]; // 0x886
            };
        };

        void invalidate();
        void sub_4CA17F();
    };

#pragma pack(pop)
}
