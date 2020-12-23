#include "Misc.h"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "ThingManager.h"

using namespace OpenLoco;
using namespace OpenLoco::ObjectManager;

steam_object* OpenLoco::exhaust::object() const
{
    return ObjectManager::get<steam_object>(object_id & 0x7F);
}

// 0x0044080C
exhaust* OpenLoco::exhaust::create(loc16 loc, uint8_t type)
{
    if ((uint16_t)loc.x > 12287 || (uint16_t)loc.y > 12287)
        return nullptr;
    auto surface = OpenLoco::Map::TileManager::get(loc.x & 0xFFE0, loc.y & 0xFFE0).surface();

    if (surface == nullptr)
        return nullptr;

    if (loc.z <= surface->baseZ() * 4)
        return nullptr;

    auto _exhaust = static_cast<exhaust*>(ThingManager::createThing());

    if (_exhaust != nullptr)
    {
        _exhaust->base_type = thing_base_type::misc;
        _exhaust->moveTo(loc);
        _exhaust->object_id = type;
        auto obj = _exhaust->object();
        _exhaust->var_14 = obj->var_05;
        _exhaust->var_09 = obj->var_06;
        _exhaust->var_15 = obj->var_07;
        _exhaust->mType = MiscThingType::exhaust;
        _exhaust->var_26 = 0;
        _exhaust->var_28 = 0;
        _exhaust->var_32 = 0;
        _exhaust->var_34 = 0;
        _exhaust->var_36 = 0;
    }
    return _exhaust;
}

// 0x00440BEB
smoke* OpenLoco::smoke::create(loc16 loc)
{
    auto t = static_cast<smoke*>(ThingManager::createThing());
    if (t != nullptr)
    {
        t->var_14 = 44;
        t->var_09 = 32;
        t->var_15 = 34;
        t->base_type = thing_base_type::misc;
        t->moveTo(loc);
        t->mType = MiscThingType::smoke;
        t->var_28 = 0;
    }
    return t;
}
