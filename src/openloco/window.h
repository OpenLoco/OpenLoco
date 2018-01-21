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

    struct viewport
    {
        int16_t width;                   // 0x00
        int16_t height;                  // 0x02
        int16_t x;                       // 0x04
        int16_t y;                       // 0x06
        int16_t view_x;                  // 0x08
        int16_t view_y;                  // 0x0A
        int16_t view_width;              // 0x0C
        int16_t view_height;             // 0x0E
        uint8_t zoom;                    // 0x10
        uint8_t pad_11;
        uint16_t var_12;                 // 0x12, maybe flags
    };
    #pragma pack(pop)
}
