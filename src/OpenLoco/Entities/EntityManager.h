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
    enum class EntityListType
    {
        null,      // Used for vehicles and other misc entities (not money)
        nullMoney, // For some reason money effects have their own pool of entities to use
        vehicleHead,
        misc = 4,
        vehicle = 6,
    };

    void reset();

    template<typename T>
    T* get(EntityId id);

    template<>
    EntityBase* get(EntityId id);

    template<typename T>
    T* get(EntityId id)
    {
        auto* base = get<EntityBase>(id);
        return base != nullptr ? base->asBase<T>() : nullptr;
    }

    EntityId firstId(EntityListType list);

    template<typename T>
    T* first();

    EntityId firstQuadrantId(const Map::Pos2& loc);
    void resetSpatialIndex();
    void updateSpatialIndex();
    void moveSpatialEntry(EntityBase& entity, const Map::Pos3& loc);

    EntityBase* createEntityMisc();
    EntityBase* createEntityMoney();
    EntityBase* createEntityVehicle();
    void freeEntity(EntityBase* const entity);

    void updateVehicles();
    void updateMiscEntities();
    void updateDaily();
    void updateMonthly();

    uint16_t getListCount(const EntityListType list);
    void moveEntityToList(EntityBase* const entity, const EntityListType list);
    bool checkNumFreeEntities(const size_t numNewEntities);
    void zeroUnused();

    template<typename TEntityType, EntityId EntityBase::*nextList>
    class ListIterator
    {
    private:
        TEntityType* entity = nullptr;
        EntityId nextEntityId = EntityId::null;

    public:
        ListIterator(const EntityId _headId)
            : nextEntityId(_headId)
        {
            ++(*this);
        }

        ListIterator& operator++()
        {
            entity = get<TEntityType>(nextEntityId);

            if (entity)
            {
                nextEntityId = entity->*nextList;
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
            return entity == other.entity;
        }
        bool operator!=(ListIterator& other) const
        {
            return !(*this == other);
        }
        TEntityType* operator*()
        {
            if (entity == nullptr)
            {
                throw "Bad Entity List!";
            }
            return entity;
        }
        // iterator traits
        using difference_type = std::ptrdiff_t;
        using value_type = TEntityType;
        using pointer = TEntityType*;
        using reference = TEntityType&;
        using iterator_category = std::forward_iterator_tag;
    };

    template<typename T>
    using EntityListIterator = ListIterator<T, &EntityBase::next_thing_id>;

    template<typename T, EntityListType list>
    class EntityList
    {
    private:
        EntityId firstId = EntityId::null;

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
            return T(EntityId::null);
        }
    };

    using VehicleList = EntityList<EntityListIterator<Vehicles::VehicleHead>, EntityListType::vehicleHead>;

    class EntityTileList
    {
    private:
        EntityId firstId = EntityId::null;
        using Iterator = ListIterator<EntityBase, &EntityBase::nextQuadrantId>;

    public:
        EntityTileList(const Map::Pos2& loc)
        {
            firstId = EntityManager::firstQuadrantId(loc);
        }

        Iterator begin()
        {
            return Iterator(firstId);
        }
        Iterator end()
        {
            return Iterator(EntityId::null);
        }
    };
}
