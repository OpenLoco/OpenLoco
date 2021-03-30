#include "EntityManager.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../OpenLoco.h"
#include "../Vehicles/Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::EntityManager
{
    loco_global<thing_id_t[numEntityLists], 0x00525E40> _heads;
    loco_global<uint16_t[numEntityLists], 0x00525E4C> _listCounts;
    loco_global<Entity[maxEntities], 0x006DB6DC> _entities;
    loco_global<thing_id_t[0x40001], 0x01025A8C> _entitySpatialIndex;
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;
    constexpr size_t _entitySpatialIndexNull = 0x40000;

    // 0x0046FDFD
    void reset()
    {
        call(0x0046FDFD);
    }

    thing_id_t firstId(EntityListType list)
    {
        return _heads[(size_t)list];
    }

    uint16_t getListCount(const EntityListType list)
    {
        return _listCounts[static_cast<size_t>(list)];
    }

    template<>
    Vehicles::VehicleHead* first()
    {
        return get<Vehicles::VehicleHead>(firstId(EntityListType::vehicleHead));
    }

    template<>
    EntityBase* get(thing_id_t id)
    {
        EntityBase* result = nullptr;
        if (id < maxEntities)
        {
            return &_entities.get()[id];
        }
        return result;
    }

    constexpr size_t getSpatialIndexOffset(const Map::map_pos& loc)
    {
        size_t index = _entitySpatialIndexNull;
        if (loc.x != Location::null)
        {
            uint16_t x = loc.x & 0x3FE0;
            uint16_t y = loc.y & 0x3FE0;

            index = (x << 4) | (y >> 5);
        }

        if (index >= 0x40001)
        {
            return _entitySpatialIndexNull;
        }
        return index;
    }

    thing_id_t firstQuadrantId(const Map::map_pos& loc)
    {
        auto index = getSpatialIndexOffset(loc);
        return _entitySpatialIndex[index];
    }

    // 0x004700A5
    EntityBase* createEntity()
    {
        registers regs;
        call(0x004700A5, regs);
        return (EntityBase*)regs.esi;
    }

    // 0x0047024A
    void freeEntity(EntityBase* const entity)
    {
        registers regs;
        regs.esi = reinterpret_cast<uint32_t>(entity);
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
    void updateMiscEntities()
    {
        call(0x004402F4);
    }

    // 0x0047019F
    void moveEntityToList(EntityBase* const entity, const EntityListType list)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(entity);
        regs.ecx = (static_cast<int8_t>(list) + 1) * 2; // Loco function expects to use this to access an array of words
        call(0x0047019F, regs);
    }

    // 0x00470188
    bool checkNumFreeEntities(const size_t numNewEntities)
    {
        if (EntityManager::getListCount(EntityManager::EntityListType::null) <= numNewEntities)
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
