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
        vehicleHead,
        misc = 3,
    };

    void reset();

    template<typename T>
    T* get(EntityId_t id);

    template<>
    EntityBase* get(EntityId_t id);

    template<typename T>
    T* get(EntityId_t id)
    {
        return static_cast<T*>(get<EntityBase>(id));
    }

    EntityId_t firstId(EntityListType list);

    template<typename T>
    T* first();

    EntityId_t firstQuadrantId(const Map::map_pos& loc);

    EntityBase* createEntity();
    void freeEntity(EntityBase* const entity);

    void updateVehicles();
    void updateMiscEntities();

    uint16_t getListCount(const EntityListType list);
    void moveEntityToList(EntityBase* const entity, const EntityListType list);
    bool checkNumFreeEntities(const size_t numNewEntities);
    void zeroUnused();

    template<typename TEntityType, EntityId_t EntityBase::*nextList>
    class ListIterator
    {
    private:
        TEntityType* entity = nullptr;
        EntityId_t nextEntityId = EntityId::null;

    public:
        ListIterator(const uint16_t _headId)
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
            return entity;
        }
        // iterator traits
        using difference_type = std::ptrdiff_t;
        using value_type = TEntityType;
        using pointer = TEntityType*;
        using reference = TEntityType&;
        using iterator_category = std::forward_iterator_tag;
    };

    template<typename T, EntityListType list>
    class EntityList
    {
    private:
        uint16_t firstId = EntityId::null;

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

    using VehicleList = EntityList<ListIterator<Vehicles::VehicleHead, &EntityBase::next_thing_id>, EntityListType::vehicleHead>;

    class EntityTileList
    {
    private:
        uint16_t firstId = EntityId::null;
        using Iterator = ListIterator<EntityBase, &EntityBase::nextQuadrantId>;

    public:
        EntityTileList(const Map::map_pos& loc)
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
