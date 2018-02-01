#include "../objects/objectmgr.h"
#include "../objects/steam_object.h"
#include "../map/tilemgr.h"
#include "thingmgr.h"
#include "misc.h"

using namespace openloco;
using namespace openloco::objectmgr;

steam_object * openloco::exahust::object() const
{
    return objectmgr::get<steam_object>(object_id & 0x7F);
}

exahust* openloco::exahust::create_new_exahust(loc16 loc, uint8_t type)
{
    if (loc.x > 12287 || loc.y > 12287)
        return nullptr;
    auto surface = openloco::map::tilemgr::get(loc.x & 0xFFE0, loc.y & 0xFFE0).surface();

    if (loc.z <= surface->base_z() * 4)
        return nullptr;

    auto _exahust = static_cast<exahust *>(thingmgr::create_thing());

    if (_exahust != nullptr)
    {
        _exahust->var_00 = 1;
        _exahust->move_to(loc);
        _exahust->object_id = type;
        auto obj = _exahust->object();
        _exahust->var_14 = obj->var_05;
        _exahust->var_09 = obj->var_06;
        _exahust->var_15 = obj->var_07;
        _exahust->type = 0;
        _exahust->var_26 = 0;
        _exahust->var_28 = 0;
        _exahust->var_32 = 0;
        _exahust->var_34 = 0;
        _exahust->var_36 = 0;
    }
    return _exahust;
}


// 0x00440BEB
smoke* openloco::smoke::create_black_smoke(loc16 loc)
{
    auto t = static_cast<smoke *>(thingmgr::create_thing());
    if (t != nullptr)
    {
        t->var_14 = 44;
        t->var_09 = 32;
        t->var_15 = 34;
        t->var_00 = 1;
        t->move_to(loc);
        t->type = 8;
        t->var_28 = 0;
    }
    return t;
}
