#pragma once

#include "EntityManager.h"
#include <OpenLoco/Engine/World.hpp>
#include <vector>

namespace OpenLoco
{
    class EntityTweener
    {
        std::vector<EntityBase*> _entities;
        std::vector<World::Pos3> _prePos;
        std::vector<World::Pos3> _postPos;

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
