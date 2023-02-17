#pragma once
#include "Entities/EntityManager.h"
#include "Entities/Misc.h"

namespace OpenLoco::EffectsManager
{
    using EffectsList = EntityManager::EntityList<EntityManager::EntityListIterator<MiscBase>, EntityManager::EntityListType::misc>;

    void update();
}
