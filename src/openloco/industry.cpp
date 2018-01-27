#include "industry.h"
#include "interop/interop.hpp"
#include "map/tilemgr.h"
#include "objects/objectmgr.h"

using namespace openloco::interop;
using namespace openloco::map;

namespace openloco
{
    industry_id_t industry::id() const
    {
        auto first = (industry*)0x005C455C;
        return (industry_id_t)(this - first);
    }

    industry_object* industry::object() const
    {
        return objectmgr::get<industry_object>(object_id);
    }

    bool industry::empty() const
    {
        return name == string_ids::null;
    }

    // 0x00453275
    void industry::update()
    {
        // registers regs;
        // regs.esi = (int32_t)this;
        // regs.edx = id();
        // call(0x00453275, regs);

        if (!(flags & industry_flags::flag_01))
        {
            auto obj = object();
            if (obj->var_11 == 0xFF)
            {
                for (int i = 0; i < 100; i++)
                {
                    auto tile = tilemgr::get(x, y);
                    auto surface = tile.surface();
                    if (surface != nullptr)
                    {
                        if (surface->data()[0] & 0x80)
                        {
                            if (surface->data()[7] == 0)
                            {
                                // WIP
                            }
                        }
                    }
                }
            }
        }
    }
}
