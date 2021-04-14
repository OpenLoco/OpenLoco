#pragma once

#include "../Map/Map.hpp"
#include "EntityManager.h"
#include <vector>

namespace OpenLoco
{
    class EntityTweener
    {
        std::vector<EntityBase*> _entities;
        std::vector<Map::map_pos3> _prePos;
        std::vector<Map::map_pos3> _postPos;

    public:
        static EntityTweener& Get();

        void PreTick();
        void PostTick();
        void RemoveEntity(const EntityBase* entity);
        void Tween(float alpha);
        void Restore();
        void Reset();
    };
}
