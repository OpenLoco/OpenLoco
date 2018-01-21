#pragma once

#include "thing.h"

namespace openloco
{
    namespace flags_5f
    {
        constexpr uint8_t breakdown_pending = 1 << 1;
        constexpr uint8_t broken_down = 1 << 2;
    }

#pragma pack(push, 1)
    struct vehicle : thing
    {
        vehicle* next_vehicle();
        vehicle* next_car();

        void update_head();
        void sub_4BA8D4();

    private:
        bool update();
        void sub_4BAA76();
        int32_t sub_4AA1D0();
        void sub_4AAC4E();
        void invalidate_sprite();
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct unk_113D758
    {
        uint16_t var_00;
        uint16_t var_02;
        int16_t var_04;
        int16_t var_06;
        int16_t x;         // 0x08
        int16_t y;         // 0x0A
        int16_t width;     // 0x0C
        int16_t height;    // 0x0E
        uint8_t zoom_level;
        uint8_t pad_11;
        uint16_t var_12;
    };
#pragma pack(pop)
}
