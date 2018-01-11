#include "../interop/interop.hpp"
#include "../openloco.h"
#include "thingmgr.h"

namespace openloco::thingmgr
{
    loco_global_array<thing_id_t, num_thing_lists, 0x00525E40> _heads;
    loco_global_array<thing, max_things, 0x006DB6DC> _things;

    thing_id_t first_id(thing_list list)
    {
        return _heads[(size_t)list];
    }

    template<>
    vehicle * first()
    {
        return get<vehicle>(first_id(thing_list::vehicle));
    }

    template<>
    thing * get(thing_id_t id)
    {
        thing * result = nullptr;
        if (id < max_things)
        {
            return &_things.get()[id];
        }
        return result;
    }

    // 0x004A8826
    void update_vehicles()
    {
        if ((LOCO_GLOBAL(0x00525E28, uint32_t) & 1) && !is_editor_mode())
        {
            auto v = first<vehicle>();
            while (v != nullptr)
            {
                auto next = v->next_vehicle();
                v->update_head();
                v = next;
            }
        }
    }

    // 0x004402F4
    void update_misc_things()
    {
        LOCO_CALLPROC_X(0x004402F4);
    }
}
