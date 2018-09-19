#include "thingmgr.h"
#include "../interop/interop.hpp"
#include "../openloco.h"

using namespace openloco;
using namespace openloco::interop;

thingmanager openloco::g_thingmgr;

static loco_global<thing_id_t[num_thing_lists], 0x00525E40> _heads;
static loco_global<thing[max_things], 0x006DB6DC> _things;

thing_id_t thingmanager::first_id(thing_list list)
{
    return _heads[(size_t)list];
}

template<>
vehicle* thingmanager::first()
{
    return get<vehicle>(first_id(thing_list::vehicle));
}

template<>
thing_base* thingmanager::get(thing_id_t id)
{
    thing_base* result = nullptr;
    if (id < max_things)
    {
        return &_things.get()[id];
    }
    return result;
}

template<typename T>
T* thingmanager::get(thing_id_t id)
{
    return static_cast<T*>(get<thing_base>(id));
}

// 0x004700A5
thing_base* thingmanager::create_thing()
{
    registers regs;
    call(0x004700A5, regs);
    return (thing_base*)regs.esi;
}

// 0x004A8826
void thingmanager::update_vehicles(objectmanager& objectmgr)
{
    if ((addr<0x00525E28, uint32_t>() & 1) && !is_editor_mode())
    {
        auto v = first<vehicle>();
        while (v != nullptr)
        {
            auto next = v->next_vehicle(*this);
            v->update_head(objectmgr, *this);
            v = next;
        }
    }
}

// 0x004402F4
void thingmanager::update_misc_things()
{
    call(0x004402F4);
}
