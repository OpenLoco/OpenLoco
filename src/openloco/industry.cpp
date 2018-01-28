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

    static bool find_20(surface_element* surface)
    {
        auto element = surface;
        while (!element->is_last())
        {
            element++;
            if (element->type() == element_type::unk_20)
            {
                return true;
            }
        }
        return false;
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
                // Run tile loop for 100 iterations
                for (int i = 0; i < 100; i++)
                {
                    auto surface = tilemgr::get(tile_loop_x, tile_loop_y).surface();
                    if (surface != nullptr)
                    {
                        if (surface->data()[0] & 0x80)
                        {
                            if (surface->data()[7] == 0)
                            {
                                uint8_t bl = surface->data()[6] >> 5;
                                if (bl == 0 || bl != obj->var_EA)
                                {
                                    var_DB++;
                                    if ((!(obj->var_E4 & 0x10000000) && (surface->data()[4] & 0xE0) == 0) || find_20(surface))
                                    {
                                        var_DD++;
                                    }
                                }
                            }
                        }
                    }

                    tile_loop_x += tile_size;
                    if (tile_loop_x >= map_size - 1)
                    {
                        tile_loop_x = 0;
                        tile_loop_y += tile_size;
                        if (tile_loop_y >= map_size - 1)
                        {
                            tile_loop_y = 0;
                        }
                    }
                }
            }
        }
    }
}
