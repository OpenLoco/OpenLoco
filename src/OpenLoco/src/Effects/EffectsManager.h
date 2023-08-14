#pragma once
#include "Effect.h"
#include "Entities/EntityManager.h"

namespace OpenLoco::EffectsManager
{
    using EffectsList = EntityManager::EntityList<EntityManager::EntityListIterator<MiscBase>, EntityManager::EntityListType::misc>;

    void update();
}
