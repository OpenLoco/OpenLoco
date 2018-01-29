#include "townmgr.h"
#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::townmgr
{
    static loco_global<town[80], 0x005B825C> _towns;

    std::array<town, max_towns> towns()
    {
        auto arr = (std::array<town, max_towns>*)_towns.get();
        return *arr;
    }

    town* get(town_id_t id)
    {
        if (id >= _towns.size())
        {
            return nullptr;
        }
        return &_towns[id];
    }
}
