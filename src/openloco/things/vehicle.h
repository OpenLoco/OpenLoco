#pragma once

#include "thing.h"

namespace openloco
{
#pragma pack(push, 1)
    struct vehicle : thing
    {
        vehicle * next_vehicle();
        vehicle * next_car();

        void update_head();

    private:
        bool update();
    };
#pragma pack(pop)
}
