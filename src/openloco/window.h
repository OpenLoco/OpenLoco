#pragma once

#include "localisation/stringmgr.h"
#include <cstdint>

namespace openloco::ui
{
#pragma pack(push, 1)

    struct widget
    {
        uint8_t type;   // 0x00
        uint8_t colour; // 0x01
        int16_t left;   // 0x02
        int16_t right;  // 0x04
        int16_t top;    // 0x06
        int16_t bottom; // 0x08
        union
        {
            uint32_t image;
            string_id text;
            uint32_t content;
        };
        string_id tooltip; // 0x0E
    };

    namespace window_flags
    {
        constexpr uint16_t flag_0 = 1 << 0;
        constexpr uint16_t flag_1 = 1 << 1;
        constexpr uint16_t flag_4 = 1 << 4;
        constexpr uint16_t flag_5 = 1 << 5;
        constexpr uint16_t flag_6 = 1 << 6;
        constexpr uint16_t flag_9 = 1 << 9;
        constexpr uint16_t flag_12 = 1 << 12;
    }

    struct window
    {
        union
        {
            uint8_t pad_all[0x88E];
            struct
            {
                void* event_handlers;     // 0x00
                void* viewport;           // 0x04
                uint8_t pad_08[0x04];     // 0x08
                uint32_t enabled_widgets; // 0x0C
                uint8_t pad_10[0x2C - 0x10];
                widget* widgets;     // 0x2C
                uint16_t x;          // 0x30
                uint16_t y;          // 0x32
                uint16_t width;      // 0x34
                uint16_t height;     // 0x36
                uint16_t min_width;  // 0x38
                uint16_t max_width;  // 0x3a
                uint16_t min_height; // 0x3c
                uint16_t max_height; // 0x3e
                uint16_t var_40;
                uint32_t flags;
                uint8_t pad_46[0x83E - 0x46];
                uint16_t var_83E;
                uint8_t pad_840[0x846 - 0x840];
                uint16_t var_846;
                uint8_t pad_848[0x854 - 0x848];
                uint16_t var_854;
                uint16_t var_856;
                uint8_t pad_858[0x85A - 0x858];
                uint16_t var_85A;
                uint8_t pad_85C[0x870 - 0x85C];
                uint16_t var_870;
                uint16_t var_872;
                uint8_t pad_874[0x876 - 0x874];
                uint16_t var_876;
                uint16_t var_878;
                uint16_t var_87A;
                uint8_t pad_87C[0x882 - 0x87C];
                uint8_t type; // 0x882
                uint8_t pad_883[0x886 - 0x883];
                uint8_t colours[4]; // 0x886
            };
        };

        void invalidate();
        void sub_4CA17F();
    };

    struct viewport
    {
        int16_t width;       // 0x00
        int16_t height;      // 0x02
        int16_t x;           // 0x04
        int16_t y;           // 0x06
        int16_t view_x;      // 0x08
        int16_t view_y;      // 0x0A
        int16_t view_width;  // 0x0C
        int16_t view_height; // 0x0E
        uint8_t zoom;        // 0x10
        uint8_t pad_11;
        uint16_t var_12; // 0x12, maybe flags
    };
#pragma pack(pop)
}
