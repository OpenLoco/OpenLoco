#pragma once

#include "../objects/vehicle_object.h"
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
        vehicle_object* object() const;

        void update_head();
        void sub_4BA8D4();

    private:
        bool update();
        void sub_4BAA76();
        int32_t sub_4AA1D0();
        void sub_4AAC4E();
        void sub_4AAB0B();
        void sub_4AC255(vehicle * back_bogie, vehicle * front_bogie);
        uint16_t sub_4BE368(uint32_t distance);
        uint8_t sub_4BF4DA(uint16_t xy_offset, int16_t z_offset);
    };
#pragma pack(pop)
}
