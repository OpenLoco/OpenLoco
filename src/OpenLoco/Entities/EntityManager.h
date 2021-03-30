#pragma once

#include "../Map/Map.hpp"
#include "Entity.h"
#include <cstdio>
#include <iterator>

namespace OpenLoco::Vehicles
{
    struct VehicleHead;
}

namespace OpenLoco::EntityManager
{
    constexpr size_t numEntityLists = 6;
    constexpr size_t maxEntities = 20000;

    enum class EntityListType
    {
        null,
        vehicle_head,
        misc = 3,
    };

    void reset();

    template<typename T>
    T* get(thing_id_t id);

    template<>
    EntityBase* get(thing_id_t id);

    template<typename T>
    T* get(thing_id_t id)
    {
        return static_cast<T*>(get<EntityBase>(id));
    }

    thing_id_t firstId(EntityListType list);

    template<typename T>
    T* first();

    thing_id_t firstQuadrantId(const Map::map_pos& loc);

    EntityBase* createThing();
    void freeThing(EntityBase* const thing);

    void updateVehicles();
    void updateMiscThings();

    uint16_t getListCount(const EntityListType list);
    void moveSpriteToList(EntityBase* const thing, const EntityListType list);
    bool checkNumFreeThings(const size_t numNewThings);
    void zeroUnused();

    template<typename ThingType, thing_id_t EntityBase::*nextList>
    class ListIterator
    {
    private:
        ThingType* thing = nullptr;
        thing_id_t nextThingId = ThingId::null;

    public:
        ListIterator(const uint16_t _headId)
            : nextThingId(_headId)
        {
            ++(*this);
        }

        ListIterator& operator++()
        {
            thing = get<ThingType>(nextThingId);

            if (thing)
            {
                nextThingId = thing->*nextList;
            }
            return *this;
        }

        ListIterator operator++(int)
        {
            ListIterator retval = *this;
            ++(*this);
            return retval;
        }
        bool operator==(ListIterator& other) const
        {
            return thing == other.thing;
        }
        bool operator!=(ListIterator& other) const
        {
            return !(*this == other);
        }
        ThingType* operator*()
        {
            return thing;
        }
        // iterator traits
        using difference_type = std::ptrdiff_t;
        using value_type = ThingType;
        using pointer = ThingType*;
        using reference = ThingType&;
        using iterator_category = std::forward_iterator_tag;
    };

    template<typename T, EntityListType list>
    class EntityList
    {
    private:
        uint16_t firstId = ThingId::null;
        using thingListIterator = T;

    public:
        EntityList()
        {
            firstId = EntityManager::firstId(list);
        }

        T begin()
        {
            return T(firstId);
        }
        T end()
        {
            return T(ThingId::null);
        }
    };

    using VehicleList = EntityList<ListIterator<Vehicles::VehicleHead, &EntityBase::next_thing_id>, EntityListType::vehicle_head>;

    class EntityTileList
    {
    private:
        uint16_t firstId = ThingId::null;
        using ThingTileListIterator = ListIterator<EntityBase, &EntityBase::nextQuadrantId>;

    public:
        EntityTileList(const Map::map_pos& loc)
        {
            firstId = EntityManager::firstQuadrantId(loc);
        }

        ThingTileListIterator begin()
        {
            return ThingTileListIterator(firstId);
        }
        ThingTileListIterator end()
        {
            return ThingTileListIterator(ThingId::null);
        }
    };
}
