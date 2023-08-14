#pragma once
#include "Effect.h"
#include "Entities/EntityManager.h"

namespace OpenLoco::EffectsManager
{
    using EffectsList = EntityManager::EntityList<EntityManager::EntityListIterator<EffectEntity>, EntityManager::EntityListType::misc>;

    void update();
}
