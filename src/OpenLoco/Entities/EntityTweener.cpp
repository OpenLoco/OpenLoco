#include "EntityTweener.h"
#include <cmath>
#include <iostream>

namespace OpenLoco
{
    using EntityListType = EntityManager::EntityListType;
    using EntityListIterator = EntityManager::ListIterator<EntityBase, &EntityBase::next_thing_id>;

    template<EntityListType id>
    void PopulateEntities(std::vector<EntityBase*>& list, std::vector<Map::map_pos3>& posList)
    {
        auto entsView = EntityManager::EntityList<EntityListIterator, id>();
        for (auto* ent : entsView)
        {
            list.push_back(ent);
            posList.emplace_back(ent->x, ent->y, ent->z);
        }
    }

    static EntityTweener _tweener;

    EntityTweener& EntityTweener::Get()
    {
        return _tweener;
    }

    void EntityTweener::PreTick()
    {
        Restore();
        Reset();
        PopulateEntities<EntityListType::misc>(_entities, _prePos);
        PopulateEntities<EntityListType::vehicleHead>(_entities, _prePos);
        PopulateEntities<EntityListType::vehicle>(_entities, _prePos);
    }

    void EntityTweener::PostTick()
    {
        for (auto* ent : _entities)
        {
            if (ent == nullptr || ent->id == EntityId::null)
            {
                // Sprite was removed, add a dummy position to keep the index aligned.
                _postPos.emplace_back(0, 0, 0);
            }
            else
            {
                _postPos.emplace_back(ent->x, ent->y, ent->z);
            }
        }
    }

    void EntityTweener::RemoveEntity(const EntityBase* entity)
    {
        auto it = std::find(_entities.begin(), _entities.end(), entity);
        if (it != _entities.end())
        {
            *it = nullptr;
        }
    }

    void EntityTweener::Tween(float alpha)
    {
        const float inv = (1.0f - alpha);
        for (size_t i = 0; i < _entities.size(); ++i)
        {
            auto* ent = _entities[i];
            if (ent == nullptr)
                continue;

            auto& posA = _prePos[i];
            auto& posB = _postPos[i];

            if (posA == posB)
                continue;

            auto newPos = Map::map_pos3{ static_cast<int16_t>(std::round(posB.x * alpha + posA.x * inv)),
                                         static_cast<int16_t>(std::round(posB.y * alpha + posA.y * inv)),
                                         static_cast<int16_t>(std::round(posB.z * alpha + posA.z * inv)) };

            if (newPos.x == ent->x && newPos.y == ent->y && newPos.z == ent->z)
                continue;

            ent->moveTo(newPos);
            ent->invalidateSprite();
        }
    }

    void EntityTweener::Restore()
    {
        for (size_t i = 0; i < _entities.size(); ++i)
        {
            auto* ent = _entities[i];
            if (ent == nullptr)
                continue;

            auto& newPos = _postPos[i];

            if (newPos.x == ent->x && newPos.y == ent->y && newPos.z == ent->z)
                continue;

            ent->moveTo(newPos);
            ent->invalidateSprite();
        }
    }

    void EntityTweener::Reset()
    {
        _entities.clear();
        _prePos.clear();
        _postPos.clear();
    }

}
