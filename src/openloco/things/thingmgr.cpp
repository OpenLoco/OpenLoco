#include "thingmgr.h"
#include "../interop/interop.hpp"
#include "../openloco.h"

using namespace openloco::interop;

namespace openloco::thingmgr
{
    loco_global<thing_id_t[num_thing_lists], 0x00525E40> _heads;
    loco_global<uint16_t[num_thing_lists], 0x00525E4C> _listCounts;
    loco_global<Thing[max_things], 0x006DB6DC> _things;

    thing_id_t first_id(thing_list list)
    {
        return _heads[(size_t)list];
    }

    uint16_t getListCount(const thing_list list)
    {
        return _listCounts[static_cast<size_t>(list)];
    }

    template<>
    vehicle* first()
    {
        return get<vehicle>(first_id(thing_list::vehicle));
    }

    template<>
    thing_base* get(thing_id_t id)
    {
        thing_base* result = nullptr;
        if (id < max_things)
        {
            return &_things.get()[id];
        }
        return result;
    }

    // 0x004700A5
    thing_base* create_thing()
    {
        registers regs;
        call(0x004700A5, regs);
        return (thing_base*)regs.esi;
    }

    // 0x004A8826
    void update_vehicles()
    {
        if ((addr<0x00525E28, uint32_t>() & 1) && !is_editor_mode())
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
        call(0x004402F4);
    }

    // 0x0047019F
    void moveSpriteToList(thing_base* const thing, const thing_list list)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(thing);
        regs.cl = static_cast<int8_t>(list);
        call(0x0047019F, regs);
    }
}
