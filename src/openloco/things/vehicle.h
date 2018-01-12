#pragma once

#include "thing.h"

namespace openloco
{
    namespace flags_5f
    {
        constexpr uint8_t can_breakdown = 1 << 1;
        constexpr uint8_t flag_2 = 1 << 2;
    }

#pragma pack(push, 1)
    struct vehicle : thing
    {
        vehicle * next_vehicle();
        vehicle * next_car();

        void update_head();
        void sub_4BA8D4();

    private:
        bool update();
        void sub_4BAA76();
    };
#pragma pack(pop)
}
