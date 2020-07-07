#pragma once

#include "thing.h"
#include "vehicle.h"
#include <cstdio>

namespace openloco::thingmgr
{
    constexpr size_t num_thing_lists = 6;
    constexpr size_t max_things = 20000;

    enum class thing_list
    {
        null,
        vehicle,
        misc = 3,
    };

    template<typename T>
    T* get(thing_id_t id);

    template<>
    thing_base* get(thing_id_t id);

    template<typename T>
    T* get(thing_id_t id)
    {
        return static_cast<T*>(get<thing_base>(id));
    }

    thing_id_t first_id(thing_list list);

    template<typename T>
    T* first();

    thing_base* create_thing();
    void freeThing(thing_base* const thing);

    void update_vehicles();
    void update_misc_things();

    uint16_t getListCount(const thing_list list);
    void moveSpriteToList(thing_base* const thing, const thing_list list);
    bool checkNumFreeThings(const size_t numNewThings);
}
