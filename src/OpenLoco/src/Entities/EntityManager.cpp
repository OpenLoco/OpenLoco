#include "EntityManager.h"
#include "EntityTweener.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include <OpenLoco/Core/LocoFixedVector.hpp>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::EntityManager
{
    constexpr size_t kSpatialEntityMapSize = (World::TileManager::kMapPitch * World::TileManager::kMapPitch) + 1;
    constexpr size_t kEntitySpatialIndexNull = kSpatialEntityMapSize - 1;

    static_assert(kSpatialEntityMapSize == 0x40001);
    static_assert(kEntitySpatialIndexNull == 0x40000);

    loco_global<EntityId[kSpatialEntityMapSize], 0x01025A8C> _entitySpatialIndex;
    loco_global<uint32_t, 0x01025A88> _entitySpatialCount;

    static auto& rawEntities() { return getGameState().entities; }
    static auto entities() { return FixedVector(rawEntities()); }
    static auto& rawListHeads() { return getGameState().entityListHeads; }
    static auto& rawListCounts() { return getGameState().entityListCounts; }

    constexpr uint8_t getLinkedListOffset(EntityListType list)
    {
        return enumValue(list) * sizeof(uint16_t);
    }

    constexpr size_t getLinkedListIndex(uint8_t offset)
    {
        return offset / sizeof(uint16_t);
    }

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
            ent.baseType = EntityBaseType::null;
            ent.id = EntityId(id);
            ent.nextEntityId = EntityId::null;
            ent.linkedListOffset = getLinkedListOffset(EntityListType::null);
            if (previous == nullptr)
            {
                ent.llPreviousId = EntityId::null;
                rawListHeads()[enumValue(EntityListType::null)] = EntityId(id);
            }
            else
            {
                ent.llPreviousId = previous->id;
                previous->nextEntityId = EntityId(id);
            }
            previous = &ent;
        }
        rawListCounts()[enumValue(EntityListType::null)] = Limits::maxNormalEntities;

        // Remake null money entities (size kMaxMoneyEntities)
        previous = nullptr;
        for (; id < Limits::kMaxEntities; ++id)
        {
            auto& ent = rawEntities()[id];
            ent.baseType = EntityBaseType::null;
            ent.id = EntityId(id);
            ent.nextEntityId = EntityId::null;
            ent.linkedListOffset = getLinkedListOffset(EntityListType::nullMoney);
            if (previous == nullptr)
            {
                ent.llPreviousId = EntityId::null;
                rawListHeads()[enumValue(EntityListType::nullMoney)] = EntityId(id);
            }
            else
            {
                ent.llPreviousId = previous->id;
                previous->nextEntityId = EntityId(id);
            }
            previous = &ent;
        }
        rawListCounts()[enumValue(EntityListType::nullMoney)] = Limits::kMaxMoneyEntities;

        resetSpatialIndex();
        EntityTweener::get().reset();
    }

    void freeUserStrings()
    {
        for (auto& entity : rawEntities())
        {
            if (entity.baseType == EntityBaseType::null)
            {
                continue;
            }

            StringManager::emptyUserString(entity.name);
        }
    }

    EntityId firstId(EntityListType list)
    {
        return rawListHeads()[enumValue(list)];
    }

    uint16_t getListCount(const EntityListType list)
    {
        return rawListCounts()[enumValue(list)];
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

    constexpr size_t getSpatialIndexOffset(const World::Pos2& loc)
    {
        if (loc.x == Location::null)
        {
            return kEntitySpatialIndexNull;
        }

        const auto tileX = std::abs(loc.x) / World::kTileSize;
        const auto tileY = std::abs(loc.y) / World::kTileSize;

        if (tileX >= World::TileManager::kMapPitch || tileY >= World::TileManager::kMapPitch)
        {
            return kEntitySpatialIndexNull;
        }

        return (World::TileManager::kMapPitch * tileX) + tileY;
    }

    EntityId firstQuadrantId(const World::Pos2& loc)
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

    void moveSpatialEntry(EntityBase& entity, const World::Pos3& loc)
    {
        const auto newIndex = getSpatialIndexOffset(loc);
        const auto oldIndex = getSpatialIndexOffset(entity.position);
        if (newIndex != oldIndex)
        {
            if (!removeFromSpatialIndex(entity, oldIndex))
            {
                Logging::info("Invalid quadrant ids... Resetting spatial index.");
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
            Logging::error("Tried to create invalid entity! id: {}, list: {}", enumValue(id), enumValue(list));
            return nullptr;
        }
        moveEntityToList(newEntity, list);

        newEntity->position = { Location::null, Location::null, 0 };
        insertToSpatialIndex(*newEntity);

        newEntity->name = StringIds::empty_pop;
        newEntity->spriteWidth = 16;
        newEntity->spriteHeightNegative = 20;
        newEntity->spriteHeightPositive = 8;
        newEntity->vehicleFlags = VehicleFlags::none;
        newEntity->spriteLeft = Location::null;

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

        auto newId = rawListHeads()[enumValue(EntityListType::null)];
        return createEntity(newId, EntityListType::misc);
    }

    // 0x0047011C
    EntityBase* createEntityMoney()
    {
        if (getListCount(EntityListType::nullMoney) <= 0)
        {
            return nullptr;
        }

        const auto newId = rawListHeads()[enumValue(EntityListType::nullMoney)];
        return createEntity(newId, EntityListType::misc);
    }

    // 0x00470039
    EntityBase* createEntityVehicle()
    {
        if (getListCount(EntityListType::null) <= 0)
        {
            return nullptr;
        }

        const auto newId = rawListHeads()[enumValue(EntityListType::null)];
        return createEntity(newId, EntityListType::vehicle);
    }

    // 0x0047024A
    void freeEntity(EntityBase* const entity)
    {
        EntityTweener::get().removeEntity(entity);

        auto list = enumValue(entity->id) < 19800 ? EntityListType::null : EntityListType::nullMoney;
        moveEntityToList(entity, list);
        StringManager::emptyUserString(entity->name);
        entity->baseType = EntityBaseType::null;

        if (!removeFromSpatialIndex(*entity))
        {
            Logging::info("Invalid quadrant ids... Resetting spatial index.");
            resetSpatialIndex();
        }
    }

    // 0x0047019F
    void moveEntityToList(EntityBase* const entity, const EntityListType list)
    {
        const auto newListOffset = getLinkedListOffset(list);
        if (entity->linkedListOffset == newListOffset)
        {
            return;
        }

        const auto newListIndex = enumValue(list);
        const auto oldListIndex = getLinkedListIndex(entity->linkedListOffset);

        const auto nextId = entity->nextEntityId;
        const auto previousId = entity->llPreviousId;

        // Unlink previous entity from this entity
        if (previousId == EntityId::null)
        {
            rawListHeads()[oldListIndex] = nextId;
        }
        else
        {
            auto* previousEntity = get<EntityBase>(previousId);
            if (previousEntity == nullptr)
            {
                Logging::error("Invalid previous entity id. Entity linked list corrupted? Id: {}", enumValue(previousId));
            }
            else
            {
                previousEntity->nextEntityId = nextId;
            }
        }
        // Unlink next entity from this entity
        if (nextId != EntityId::null)
        {
            auto* nextEntity = get<EntityBase>(nextId);
            if (nextEntity == nullptr)
            {
                Logging::error("Invalid next entity id. Entity linked list corrupted? Id: {}", enumValue(nextId));
            }
            else
            {
                nextEntity->llPreviousId = previousId;
            }
        }

        entity->llPreviousId = EntityId::null;
        entity->linkedListOffset = newListOffset;
        entity->nextEntityId = rawListHeads()[newListIndex];
        rawListHeads()[newListIndex] = entity->id;
        // Link next entity to this entity
        if (entity->nextEntityId != EntityId::null)
        {
            auto* nextEntity = get<EntityBase>(entity->nextEntityId);
            if (nextEntity == nullptr)
            {
                Logging::error("Invalid next entity id. Entity linked list corrupted? Id: {}", enumValue(entity->nextEntityId));
            }
            else
            {
                nextEntity->llPreviousId = entity->id;
            }
        }

        rawListCounts()[oldListIndex]--;
        rawListCounts()[newListIndex]++;
    }

    // 0x00470188
    bool checkNumFreeEntities(const size_t numNewEntities)
    {
        if (getListCount(EntityListType::null) <= numNewEntities)
        {
            GameCommands::setErrorText(StringIds::too_many_objects_in_game);
            return false;
        }
        return true;
    }

    static void zeroEntity(EntityBase* ent)
    {
        auto next = ent->nextEntityId;
        auto previous = ent->llPreviousId;
        auto id = ent->id;
        auto llOffset = ent->linkedListOffset;
        std::fill_n(reinterpret_cast<uint8_t*>(ent), sizeof(Entity), 0);
        ent->baseType = EntityBaseType::null;
        ent->nextEntityId = next;
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
