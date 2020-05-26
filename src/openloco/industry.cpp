#include "industry.h"
#include "interop/interop.hpp"
#include "localisation/string_ids.h"
#include "map/tilemgr.h"
#include "objects/cargo_object.h"
#include "objects/industry_object.h"
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

    bool industry::canReceiveCargo() const
    {
        auto receiveCargoState = false;
        for (const auto& receivedCargo : objectmgr::get<industry_object>(object_id)->required_cargo_type)
        {
            if (receivedCargo != 0xff)
                receiveCargoState = true;
        }
        return receiveCargoState;
    }

    bool industry::canProduceCargo() const
    {
        auto produceCargoState = false;
        for (const auto& producedCargo : objectmgr::get<industry_object>(object_id)->produced_cargo_type)
        {
            if (producedCargo != 0xff)
                produceCargoState = true;
        }
        return produceCargoState;
    }

    static bool find_5(surface_element* surface)
    {
        auto element = surface;
        while (!element->is_last())
        {
            element++;
            if (element->type() == element_type::industry)
            {
                return true;
            }
        }
        return false;
    }

    // 0x0045935F
    void industry::getStatusString(const char* buffer)
    {
        char* ptr = (char*)buffer;
        *ptr = '\0';
        auto industryObj = object();

        // Closing Down
        if (flags & industry_flags::closing_down)
        {
            ptr = stringmgr::format_string(ptr, string_ids::industry_closing_down);
            return;
        }

        // Under Construction
        if (under_construction != 0xFF)
        {
            ptr = stringmgr::format_string(ptr, string_ids::industry_under_construction);
            return;
        }

        // Produced Cargo Only
        if (!canReceiveCargo())
        {
            if (!canProduceCargo())
                return;

            ptr = stringmgr::format_string(ptr, string_ids::industry_producing);

            ptr = industryObj->getProducedCargoString(ptr);

            return;
        }

        // Required Cargo
        ptr = stringmgr::format_string(ptr, string_ids::industry_requires);

        ptr = industryObj->getRequiredCargoString(ptr);

        if (!canProduceCargo())
            return;

        // Production and Received Cargo
        ptr = stringmgr::format_string(ptr, string_ids::cargo_to_produce);

        ptr = industryObj->getProducedCargoString(ptr);
    }

    // 0x00453275
    void industry::update()
    {
        if (!(flags & industry_flags::flag_01) && under_construction == 0xFF)
        {
            // Run tile loop for 100 iterations
            for (int i = 0; i < 100; i++)
            {
                sub_45329B(tile_loop.current());
                if (tile_loop.next() == map_pos())
                {
                    sub_453354();
                    break;
                }
            }
        }
    }

    // 0x0045329B
    void industry::sub_45329B(const map_pos& pos)
    {
        const auto& surface = tilemgr::get(pos).surface();
        if (surface != nullptr)
        {
            if (surface->has_high_type_flag())
            {
                if (surface->industry_id() == id())
                {
                    uint8_t bl = surface->var_6_SLR5();
                    auto obj = object();
                    if (bl == 0 || bl != obj->var_EA)
                    {
                        var_DB++;
                        if ((!(obj->flags & industry_object_flags::flag_28) && surface->var_4_E0() != 0) || find_5(surface))
                        {
                            var_DD++;
                        }
                    }
                }
            }
        }
    }

    void industry::sub_453354()
    {
        auto obj = object();
        int16_t tmp_a = var_DB / 16;
        int16_t tmp_b = std::max(0, var_DD - tmp_a);
        int16_t tmp_c = var_DB - tmp_b;
        int16_t tmp_d = std::min(tmp_c / 25, 255);
        if (tmp_d < obj->var_EB)
        {
            var_DF = tmp_d / obj->var_EB;
        }
        else
        {
            var_DF = 255;
        }

        var_DB = 0;
        var_DD = 0;
        if (var_DF < 224)
        {
            if (produced_cargo_quantity[0] / 8 <= produced_cargo_max[0] || produced_cargo_quantity[1] / 8 <= produced_cargo_max[1])
            {
                if (prng.rand_bool())
                {
                    map::map_pos randTile{ static_cast<coord_t>(x + (prng.rand_next(-15, 16) * 32)), static_cast<coord_t>(y + (prng.rand_next(-15, 16) * 32)) };
                    uint8_t bl = obj->var_ED;
                    uint8_t bh = obj->var_EE;
                    if (obj->var_EF != 0xFF && prng.rand_bool())
                    {
                        bl = obj->var_EF;
                        bh = obj->var_F0;
                    }
                    uint8_t dl = prng.rand_next(7) * 32;
                    sub_454A43(randTile, bl, bh, dl);
                }
            }
        }
    }

    void industry::sub_454A43(map_pos pos, uint8_t bl, uint8_t bh, uint8_t dl)
    {
        registers regs;
        regs.bl = bl;
        regs.bh = bh;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.dl = dl;
        regs.dh = id();
        call(0x00454A43, regs);
    }
}
