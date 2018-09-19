#pragma once

#include "thing.h"
#include "vehicle.h"
#include <cstdio>

namespace openloco
{
    constexpr size_t num_thing_lists = 6;
    constexpr size_t max_things = 20000;

    class thingmanager
    {
    public:
        enum class thing_list
        {
            null,
            vehicle,
            misc = 3
        };

        template<typename T>
        T* get(thing_id_t id);

        template<>
        thing_base* get(thing_id_t id);

        thing_id_t first_id(thing_list list);

        template<typename T>
        T* first();

        thing_base* create_thing();

        void update_vehicles();
        void update_misc_things();
    };

    extern thingmanager g_thingmgr;
}
