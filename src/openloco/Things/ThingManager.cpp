#include "ThingManager.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"

using namespace openloco::interop;

namespace openloco::thingmgr
{
    loco_global<thing_id_t[num_thing_lists], 0x00525E40> _heads;
    loco_global<uint16_t[num_thing_lists], 0x00525E4C> _listCounts;
    loco_global<Thing[max_things], 0x006DB6DC> _things;
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;

    thing_id_t firstId(thing_list list)
    {
        return _heads[(size_t)list];
    }

    uint16_t getListCount(const thing_list list)
    {
        return _listCounts[static_cast<size_t>(list)];
    }

    template<>
    vehicle_head* first()
    {
        return get<vehicle_head>(firstId(thing_list::vehicle_head));
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
    thing_base* createThing()
    {
        registers regs;
        call(0x004700A5, regs);
        return (thing_base*)regs.esi;
    }

    // 0x0047024A
    void freeThing(thing_base* const thing)
    {
        registers regs;
        regs.esi = reinterpret_cast<uint32_t>(thing);
        call(0x0047024A, regs);
    }

    // 0x004A8826
    void updateVehicles()
    {
        if ((addr<0x00525E28, uint32_t>() & 1) && !isEditorMode())
        {
            for (auto v : VehicleList())
            {
                v->updateVehicle();
            }
        }
    }

    // 0x004402F4
    void updateMiscThings()
    {
        call(0x004402F4);
    }

    // 0x0047019F
    void moveSpriteToList(thing_base* const thing, const thing_list list)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(thing);
        regs.ecx = (static_cast<int8_t>(list) + 1) * 2; // Loco function expects to use this to access an array of words
        call(0x0047019F, regs);
    }

    // 0x00470188
    bool checkNumFreeThings(const size_t numNewThings)
    {
        if (thingmgr::getListCount(thingmgr::thing_list::null) <= numNewThings)
        {
            gGameCommandErrorText = string_ids::too_many_objects_in_game;
            return false;
        }
        return true;
    }
}
