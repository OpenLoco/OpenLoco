#include "EntityManager.h"
#include "../Console.h"
#include "../Core/LocoFixedVector.hpp"
#include "../Entities/Misc.h"
#include "../Game.h"
#include "../GameCommands/GameCommands.h"
#include "../GameState.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/Tile.h"
#include "../OpenLoco.h"
#include "../Vehicles/Vehicle.h"
#include "EntityTweener.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::EntityManager
{
    constexpr size_t kSpatialEntityMapSize = (Map::map_pitch * Map::map_pitch) + 1;
    constexpr size_t kEntitySpatialIndexNull = kSpatialEntityMapSize - 1;

    static_assert(kSpatialEntityMapSize == 0x40001);
    static_assert(kEntitySpatialIndexNull == 0x40000);

    loco_global<EntityId[kSpatialEntityMapSize], 0x01025A8C> _entitySpatialIndex;
    loco_global<uint32_t, 0x01025A88> _entitySpatialCount;

    static auto& rawEntities() { return getGameState().entities; }
    static auto entities() { return FixedVector(rawEntities()); }
    static auto& rawListHeads() { return getGameState().entityListHeads; }
    static auto& rawListCounts() { return getGameState().entityListCounts; }

    // 0x0046FDFD
    void reset()
    {
        // Reset all entities to 0
        std::fill(std::begin(rawEntities()), std::end(rawEntities()), Entity{});
        // Reset all entity lists
        std::fill(std::begin(rawListCounts()), std::end(rawListCounts()), 0);
        std::fill(std::begin(rawListHeads()), std::end(rawListHeads()), EntityId::null);

        // Remake null entities (size maxNormalEntities)
        EntityBase* previous = nullptr;
        uint16_t id = 0;
        for (; id < Limits::maxNormalEntities; ++id)
        {
            auto& ent = rawEntities()[id];
            ent.base_type = EntityBaseType::null;
            ent.id = EntityId(id);
            ent.next_thing_id = EntityId::null;
            ent.linkedListOffset = static_cast<uint8_t>(EntityListType::null) * 2;
            if (previous == nullptr)
            {
                ent.llPreviousId = EntityId::null;
                rawListHeads()[static_cast<uint8_t>(EntityListType::null)] = EntityId(id);
            }
            else
            {
                ent.llPreviousId = previous->id;
                previous->next_thing_id = EntityId(id);
            }
            previous = &ent;
        }
        rawListCounts()[static_cast<uint8_t>(EntityListType::null)] = Limits::maxNormalEntities;

        // Remake null money entities (size kMaxMoneyEntities)
        previous = nullptr;
        for (; id < Limits::kMaxEntities; ++id)
        {
            auto& ent = rawEntities()[id];
            ent.base_type = EntityBaseType::null;
            ent.id = EntityId(id);
            ent.next_thing_id = EntityId::null;
            ent.linkedListOffset = static_cast<uint8_t>(EntityListType::nullMoney) * 2;
            if (previous == nullptr)
            {
                ent.llPreviousId = EntityId::null;
                rawListHeads()[static_cast<uint8_t>(EntityListType::nullMoney)] = EntityId(id);
            }
            else
            {
                ent.llPreviousId = previous->id;
                previous->next_thing_id = EntityId(id);
            }
            previous = &ent;
        }
        rawListCounts()[static_cast<uint8_t>(EntityListType::nullMoney)] = Limits::kMaxMoneyEntities;

        resetSpatialIndex();
        EntityTweener::get().reset();
    }

    EntityId firstId(EntityListType list)
    {
        return rawListHeads()[(size_t)list];
    }

    uint16_t getListCount(const EntityListType list)
    {
        return rawListCounts()[static_cast<size_t>(list)];
    }

    template<>
    Vehicles::VehicleHead* first()
    {
        return get<Vehicles::VehicleHead>(firstId(EntityListType::vehicleHead));
    }

    template<>
    EntityBase* get(EntityId id)
    {
        EntityBase* result = nullptr;
        if (enumValue(id) < Limits::kMaxEntities)
        {
            return &rawEntities()[enumValue(id)];
        }
        return result;
    }

    constexpr size_t getSpatialIndexOffset(const Map::Pos2& loc)
    {
        if (loc.x == Location::null)
            return kEntitySpatialIndexNull;

        const auto tileX = std::abs(loc.x) / Map::tile_size;
        const auto tileY = std::abs(loc.y) / Map::tile_size;

        if (tileX >= Map::map_pitch || tileY >= Map::map_pitch)
            return kEntitySpatialIndexNull;

        return (Map::map_pitch * tileX) + tileY;
    }

    EntityId firstQuadrantId(const Map::Pos2& loc)
    {
        auto index = getSpatialIndexOffset(loc);
        return _entitySpatialIndex[index];
    }

    static void insertToSpatialIndex(EntityBase& entity, const size_t newIndex)
    {
        entity.nextQuadrantId = _entitySpatialIndex[newIndex];
        _entitySpatialIndex[newIndex] = entity.id;
    }

    static void insertToSpatialIndex(EntityBase& entity)
    {
        const auto index = getSpatialIndexOffset(entity.position);
        insertToSpatialIndex(entity, index);
    }

    // 0x0046FF54
    void resetSpatialIndex()
    {
        // Clear existing array
        std::fill(std::begin(_entitySpatialIndex), std::end(_entitySpatialIndex), EntityId::null);

        // Original filled an unreferenced array at 0x010A5A8E as well then overwrote part of it???

        // Refill the index
        for (auto& ent : entities())
        {
            insertToSpatialIndex(ent);
        }
    }

    // 0x0046FC57
    void updateSpatialIndex()
    {
        for (auto& ent : entities())
        {
            ent.moveTo(ent.position);
        }
    }

    static bool removeFromSpatialIndex(EntityBase& entity, const size_t index)
    {
        auto* quadId = &_entitySpatialIndex[index];
        _entitySpatialCount = 0;
        while (enumValue(*quadId) < Limits::kMaxEntities)
        {
            auto* quadEnt = get<EntityBase>(*quadId);
            if (quadEnt == &entity)
            {
                *quadId = entity.nextQuadrantId;
                return true;
            }
            _entitySpatialCount++;
            if (_entitySpatialCount > Limits::kMaxEntities)
            {
                break;
            }
            quadId = &quadEnt->nextQuadrantId;
        }
        return false;
    }

    static bool removeFromSpatialIndex(EntityBase& entity)
    {
        const auto index = getSpatialIndexOffset(entity.position);
        return removeFromSpatialIndex(entity, index);
    }

    void moveSpatialEntry(EntityBase& entity, const Map::Pos3& loc)
    {
        const auto newIndex = getSpatialIndexOffset(loc);
        const auto oldIndex = getSpatialIndexOffset(entity.position);
        if (newIndex != oldIndex)
        {
            if (!removeFromSpatialIndex(entity, oldIndex))
            {
                Console::log("Invalid quadrant ids... Reseting spatial index.");
                resetSpatialIndex();
                moveSpatialEntry(entity, loc);
                return;
            }
            insertToSpatialIndex(entity, newIndex);
        }
        entity.position = loc;
    }

    static EntityBase* createEntity(EntityId id, EntityListType list)
    {
        auto* newEntity = get<EntityBase>(id);
        if (newEntity == nullptr)
        {
            Console::error("Tried to create invalid entity!");
            return nullptr;
        }
        moveEntityToList(newEntity, list);

        newEntity->position = { Location::null, Location::null, 0 };
        insertToSpatialIndex(*newEntity);

        newEntity->name = StringIds::empty_pop;
        newEntity->var_14 = 16;
        newEntity->var_09 = 20;
        newEntity->var_15 = 8;
        newEntity->var_0C = 0;
        newEntity->sprite_left = Location::null;

        return newEntity;
    }

    // 0x004700A5
    EntityBase* createEntityMisc()
    {
        if (getListCount(EntityListType::misc) >= Limits::kMaxMiscEntities)
        {
            return nullptr;
        }
        if (getListCount(EntityListType::null) <= 0)
        {
            return nullptr;
        }

        auto newId = rawListHeads()[static_cast<uint8_t>(EntityListType::null)];
        return createEntity(newId, EntityListType::misc);
    }

    // 0x0047011C
    EntityBase* createEntityMoney()
    {
        if (getListCount(EntityListType::nullMoney) <= 0)
        {
            return nullptr;
        }

        auto newId = rawListHeads()[static_cast<uint8_t>(EntityListType::nullMoney)];
        return createEntity(newId, EntityListType::misc);
    }

    // 0x00470039
    EntityBase* createEntityVehicle()
    {
        if (getListCount(EntityListType::null) <= 0)
        {
            return nullptr;
        }

        auto newId = rawListHeads()[static_cast<uint8_t>(EntityListType::null)];
        return createEntity(newId, EntityListType::vehicle);
    }

    // 0x0047024A
    void freeEntity(EntityBase* const entity)
    {
        EntityTweener::get().removeEntity(entity);

        auto list = enumValue(entity->id) < 19800 ? EntityListType::null : EntityListType::nullMoney;
        moveEntityToList(entity, list);
        StringManager::emptyUserString(entity->name);
        entity->base_type = EntityBaseType::null;

        if (!removeFromSpatialIndex(*entity))
        {
            Console::log("Invalid quadrant ids... Reseting spatial index.");
            resetSpatialIndex();
        }
    }

    // 0x004A8826
    void updateVehicles()
    {
        if (Game::hasFlags(1u << 0) && !isEditorMode())
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
        if (getGameState().flags & (1u << 0))
        {
            for (auto* misc : EntityList<EntityListIterator<MiscBase>, EntityListType::misc>())
            {
                misc->update();
            }
        }
    }

    // 0x004B94CF
    void updateDaily()
    {
        call(0x004B94CF);
    }

    // 0x004C3C54
    void updateMonthly()
    {
        for (auto v : VehicleList())
        {
            v->updateMonthly();
        }
    }

    // 0x0047019F
    void moveEntityToList(EntityBase* const entity, const EntityListType list)
    {
        auto newListOffset = static_cast<uint8_t>(list) * 2;
        if (entity->linkedListOffset == newListOffset)
        {
            return;
        }

        auto curList = entity->linkedListOffset / 2;
        auto nextId = entity->next_thing_id;
        auto previousId = entity->llPreviousId;

        // Unlink previous entity from this entity
        if (previousId == EntityId::null)
        {
            rawListHeads()[curList] = nextId;
        }
        else
        {
            auto* previousEntity = get<EntityBase>(previousId);
            if (previousEntity == nullptr)
            {
                Console::error("Invalid previous entity id. Entity linked list corrupted?");
            }
            else
            {
                previousEntity->next_thing_id = nextId;
            }
        }
        // Unlink next entity from this entity
        if (nextId != EntityId::null)
        {
            auto* nextEntity = get<EntityBase>(nextId);
            if (nextEntity == nullptr)
            {
                Console::error("Invalid next entity id. Entity linked list corrupted?");
            }
            else
            {
                nextEntity->llPreviousId = previousId;
            }
        }

        entity->llPreviousId = EntityId::null;
        entity->linkedListOffset = newListOffset;
        entity->next_thing_id = rawListHeads()[static_cast<uint8_t>(list)];
        rawListHeads()[static_cast<uint8_t>(list)] = entity->id;
        // Link next entity to this entity
        if (entity->next_thing_id != EntityId::null)
        {
            auto* nextEntity = get<EntityBase>(entity->next_thing_id);
            if (nextEntity == nullptr)
            {
                Console::error("Invalid next entity id. Entity linked list corrupted?");
            }
            else
            {
                nextEntity->llPreviousId = entity->id;
            }
        }

        rawListCounts()[curList]--;
        rawListCounts()[static_cast<uint8_t>(list)]++;
    }

    // 0x00470188
    bool checkNumFreeEntities(const size_t numNewEntities)
    {
        if (EntityManager::getListCount(EntityManager::EntityListType::null) <= numNewEntities)
        {
            GameCommands::setErrorText(StringIds::too_many_objects_in_game);
            return false;
        }
        return true;
    }

    static void zeroEntity(EntityBase* ent)
    {
        auto next = ent->next_thing_id;
        auto previous = ent->llPreviousId;
        auto id = ent->id;
        auto llOffset = ent->linkedListOffset;
        std::fill_n(reinterpret_cast<uint8_t*>(ent), sizeof(Entity), 0);
        ent->base_type = EntityBaseType::null;
        ent->next_thing_id = next;
        ent->llPreviousId = previous;
        ent->id = id;
        ent->linkedListOffset = llOffset;
    }

    // 0x0046FED5
    void zeroUnused()
    {
        for (auto* ent : EntityList<EntityListIterator<EntityBase>, EntityListType::null>())
        {
            zeroEntity(ent);
        }
        for (auto* ent : EntityList<EntityListIterator<EntityBase>, EntityListType::nullMoney>())
        {
            zeroEntity(ent);
        }
    }
}
