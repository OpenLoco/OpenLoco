#include "industry.h"
#include "interop/interop.hpp"
#include "map/tilemgr.h"
#include "objects/objectmgr.h"
#include "utility/numeric.hpp"
#include <algorithm>

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
                            break;
                        }
                    }
                }
                var_DF = 0xFF;
                int16_t tmp_a = var_DB >> 4;
                int16_t tmp_b = std::max(0, var_DD - tmp_a);
                int16_t tmp_c = var_DB - tmp_b;
                int16_t tmp_d = std::min(tmp_c / 25, 255);
                if (tmp_d < obj->var_EB)
                {
                    var_DF = tmp_d / obj->var_EB;
                }

                var_DB = 0;
                var_DD = 0;
                if (var_DF < 224)
                {
                    if (var_189 / 8 <= var_1A3 || var_18B / 8 <= var_1A5)
                    {
                        if (prng.rand_next(255) <= 128)
                        {
                            uint8_t x = var_02 + (prng.rand_next(-15, 16) * 32);
                            uint8_t y = var_04 + (prng.rand_next(-15, 16) * 32);
                            uint8_t bl = obj->var_ED;
                            uint8_t bh = obj->var_EE;
                            uint8_t dl = prng.rand_next(7) * 32;
                            if (prng.rand_bool() && obj->var_EF != 0xFF)
                            {
                                bl = obj->var_EF;
                                bh = obj->var_F0;
                            }
                            sub_454A43(x, y, bl, bh, dl);
                        }
                    }
                }
            }
        }
    }

    void industry::sub_454A43(coord_t x, coord_t y, uint8_t bl, uint8_t bh, uint8_t dl)
    {
        registers regs;
        regs.bl = bl;
        regs.bh = bh;
        regs.ax = x;
        regs.cx = y;
        regs.dl = dl;
        regs.dh = id();
        call(0x00454A43, regs);
    }
}
