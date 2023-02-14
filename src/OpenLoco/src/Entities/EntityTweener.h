#pragma once

#include "EntityManager.h"
#include <OpenLoco/Engine/Map.hpp>
#include <vector>

namespace OpenLoco
{
    class EntityTweener
    {
        std::vector<EntityBase*> _entities;
        std::vector<Map::Pos3> _prePos;
        std::vector<Map::Pos3> _postPos;

    public:
        static EntityTweener& get();

        void preTick();
        void postTick();
        void removeEntity(const EntityBase* entity);
        void tween(float alpha);
        void restore();
        void reset();
    };
}
