#include "EntityTweener.h"
#include "../OpenLoco.h"
#include "../Vehicles/Vehicle.h"
#include "Entity.h"
#include <cmath>
#include <iostream>

namespace OpenLoco
{
    using EntityListType = EntityManager::EntityListType;
    using EntityListIterator = EntityManager::ListIterator<EntityBase, &EntityBase::next_thing_id>;

    template<EntityListType id, typename Pred>
    void PopulateEntities(std::vector<EntityBase*>& list, std::vector<Map::Pos3>& posList, const Pred& pred)
    {
        auto entsView = EntityManager::EntityList<EntityListIterator, id>();
        for (auto* ent : entsView)
        {
            if (!pred(ent))
                continue;

            list.push_back(ent);
            posList.emplace_back(ent->position);
        }
    }

    static EntityTweener _tweener;

    EntityTweener& EntityTweener::get()
    {
        return _tweener;
    }

    void EntityTweener::preTick()
    {
        restore();
        reset();
        PopulateEntities<EntityListType::misc>(_entities, _prePos, [](auto* ent) { return true; });
        PopulateEntities<EntityListType::vehicle>(_entities, _prePos, [](auto* ent) {
            const auto* vehicle = ent->template asBase<Vehicles::VehicleBase>();
            if (vehicle == nullptr)
            {
                // This can be never null but makes the compiler happy.
                return false;
            }
            return vehicle->isVehicleBody() || vehicle->isVehicleBogie();
        });
    }

    void EntityTweener::postTick()
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
                _postPos.emplace_back(ent->position);
            }
        }
    }

    void EntityTweener::removeEntity(const EntityBase* entity)
    {
        auto it = std::find(_entities.begin(), _entities.end(), entity);
        if (it != _entities.end())
        {
            *it = nullptr;
        }
    }

    void EntityTweener::tween(float alpha)
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

            auto newPos = Map::Pos3{ static_cast<int16_t>(std::round(posB.x * alpha + posA.x * inv)),
                                     static_cast<int16_t>(std::round(posB.y * alpha + posA.y * inv)),
                                     static_cast<int16_t>(std::round(posB.z * alpha + posA.z * inv)) };

            if (ent->position == newPos)
                continue;

            ent->moveTo(newPos);
            ent->invalidateSprite();
        }
    }

    void EntityTweener::restore()
    {
        for (size_t i = 0; i < _entities.size(); ++i)
        {
            auto* ent = _entities[i];
            if (ent == nullptr)
                continue;

            auto& newPos = _postPos[i];

            if (ent->position == newPos)
                continue;

            ent->moveTo(newPos);
            ent->invalidateSprite();
        }
    }

    void EntityTweener::reset()
    {
        _entities.clear();
        _prePos.clear();
        _postPos.clear();
    }

}
