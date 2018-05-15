#include "misc.h"
#include "../map/tilemgr.h"
#include "../objects/objectmgr.h"
#include "../objects/steam_object.h"
#include "thingmgr.h"

using namespace openloco;
using namespace openloco::objectmgr;

steam_object* openloco::exhaust::object() const
{
    return objectmgr::get<steam_object>(object_id & 0x7F);
}

// 0x0044080C
exhaust* openloco::exhaust::create(loc16 loc, uint8_t type)
{
    if ((uint16_t)loc.x > 12287 || (uint16_t)loc.y > 12287)
        return nullptr;
    auto surface = openloco::map::tilemgr::get(loc.x & 0xFFE0, loc.y & 0xFFE0).surface();

    if (surface == nullptr)
        return nullptr;

    if (loc.z <= surface->base_z() * 4)
        return nullptr;

    auto _exhaust = static_cast<exhaust*>(thingmgr::create_thing());

    if (_exhaust != nullptr)
    {
        _exhaust->base_type = thing_base_type::misc;
        _exhaust->move_to(loc);
        _exhaust->object_id = type;
        auto obj = _exhaust->object();
        _exhaust->var_14 = obj->var_05;
        _exhaust->var_09 = obj->var_06;
        _exhaust->var_15 = obj->var_07;
        _exhaust->type = misc_thing_type::exhaust;
        _exhaust->var_26 = 0;
        _exhaust->var_28 = 0;
        _exhaust->var_32 = 0;
        _exhaust->var_34 = 0;
        _exhaust->var_36 = 0;
    }
    return _exhaust;
}

// 0x00440BEB
smoke* openloco::smoke::create(loc16 loc)
{
    auto t = static_cast<smoke*>(thingmgr::create_thing());
    if (t != nullptr)
    {
        t->var_14 = 44;
        t->var_09 = 32;
        t->var_15 = 34;
        t->base_type = thing_base_type::misc;
        t->move_to(loc);
        t->type = misc_thing_type::smoke;
        t->var_28 = 0;
    }
    return t;
}
