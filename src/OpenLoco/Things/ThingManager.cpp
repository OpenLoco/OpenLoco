#include "ThingManager.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../OpenLoco.h"
#include "../Ptr.h"
#include "../Vehicles/Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::ThingManager
{
    loco_global<thing_id_t[num_thing_lists], 0x00525E40> _heads;
    loco_global<uint16_t[num_thing_lists], 0x00525E4C> _listCounts;
    loco_global<Thing[max_things], 0x006DB6DC> _things;
    loco_global<thing_id_t[0x40001], 0x01025A8C> _thingSpatialIndex;
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;
    constexpr size_t _thingSpatialIndexNull = 0x40000;

    // 0x0046FDFD
    void reset()
    {
        call(0x0046FDFD);
    }

    thing_id_t firstId(thing_list list)
    {
        return _heads[(size_t)list];
    }

    uint16_t getListCount(const thing_list list)
    {
        return _listCounts[static_cast<size_t>(list)];
    }

    template<>
    Vehicles::VehicleHead* first()
    {
        return get<Vehicles::VehicleHead>(firstId(thing_list::vehicle_head));
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

    constexpr size_t getSpatialIndexOffset(const Map::map_pos& loc)
    {
        size_t index = _thingSpatialIndexNull;
        if (loc.x != Location::null)
        {
            uint16_t x = loc.x & 0x3FE0;
            uint16_t y = loc.y & 0x3FE0;

            index = (x << 4) | (y >> 5);
        }

        if (index >= 0x40001)
        {
            return _thingSpatialIndexNull;
        }
        return index;
    }

    thing_id_t firstQuadrantId(const Map::map_pos& loc)
    {
        auto index = getSpatialIndexOffset(loc);
        return _thingSpatialIndex[index];
    }

    // 0x004700A5
    thing_base* createThing()
    {
        registers regs;
        call(0x004700A5, regs);
        return ToPtr(thing_base, regs.esi);
    }

    // 0x0047024A
    void freeThing(thing_base* const thing)
    {
        registers regs;
        regs.esi = ToInt(thing);
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
        regs.esi = ToInt(thing);
        regs.ecx = (static_cast<int8_t>(list) + 1) * 2; // Loco function expects to use this to access an array of words
        call(0x0047019F, regs);
    }

    // 0x00470188
    bool checkNumFreeThings(const size_t numNewThings)
    {
        if (ThingManager::getListCount(ThingManager::thing_list::null) <= numNewThings)
        {
            gGameCommandErrorText = StringIds::too_many_objects_in_game;
            return false;
        }
        return true;
    }

    // 0x0046FED5
    void zeroUnused()
    {
        call(0x0046FED5);
    }
}
