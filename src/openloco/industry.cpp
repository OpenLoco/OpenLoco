#include "industry.h"
#include "interop/interop.hpp"
#include "localisation/string_ids.h"
#include "map/tilemgr.h"
#include "objects/cargo_object.h"
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
        for (const auto& receivedCargo : objectmgr::get<industry_object>(object_id)->received_cargo_type)
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
        if (var_11 == -1)
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
            auto producedCargo = 0;

            if (industryObj->produced_cargo_type[0] != 0xFF)
            {
                producedCargo++;
                auto cargoObj = objectmgr::get<cargo_object>(industryObj->produced_cargo_type[0]);
                ptr = stringmgr::format_string(ptr, cargoObj->name);
            }

            if (industryObj->produced_cargo_type[1] != 0xFF)
            {
                producedCargo++;

                if (producedCargo > 1)
                    ptr = stringmgr::format_string(ptr, string_ids::cargo_and);

                auto cargoObj = objectmgr::get<cargo_object>(industryObj->produced_cargo_type[1]);
                ptr = stringmgr::format_string(ptr, cargoObj->name);
            }
            return;
        }

        // Required Cargo
        ptr = stringmgr::format_string(ptr, string_ids::industry_requires);
        auto requiredCargo = 0;

        if (industryObj->received_cargo_type[0] != 0xFF)
        {
            requiredCargo++;

            auto cargoObj = objectmgr::get<cargo_object>(industryObj->received_cargo_type[0]);
            ptr = stringmgr::format_string(ptr, cargoObj->name);
        }

        if (industryObj->received_cargo_type[1] != 0xFF)
        {
            requiredCargo++;
            if (requiredCargo > 1)
            {
                if ((industryObj->var_E4 & (1 << 17)) == 0)
                    ptr = stringmgr::format_string(ptr, string_ids::cargo_and);
                else
                    ptr = stringmgr::format_string(ptr, string_ids::cargo_or);
            }

            auto cargoObj = objectmgr::get<cargo_object>(industryObj->received_cargo_type[1]);
            ptr = stringmgr::format_string(ptr, cargoObj->name);
        }

        if (industryObj->received_cargo_type[2] != 0xFF)
        {
            requiredCargo++;
            if (requiredCargo > 1)
            {
                if ((industryObj->var_E4 & (1 << 17)) == 0)
                    ptr = stringmgr::format_string(ptr, string_ids::cargo_and);
                else
                    ptr = stringmgr::format_string(ptr, string_ids::cargo_or);
            }

            auto cargoObj = objectmgr::get<cargo_object>(industryObj->received_cargo_type[2]);
            ptr = stringmgr::format_string(ptr, cargoObj->name);
        }

        if (!canProduceCargo())
            return;

        // Production and Received Cargo
        ptr = stringmgr::format_string(ptr, string_ids::cargo_to_produce);
        auto producedCargo = 0;

        if (industryObj->produced_cargo_type[0] != 0xFF)
        {
            producedCargo++;
            auto cargoObj = objectmgr::get<cargo_object>(industryObj->produced_cargo_type[0]);
            ptr = stringmgr::format_string(ptr, cargoObj->name);
        }

        if (industryObj->produced_cargo_type[1] != 0xFF)
        {
            producedCargo++;

            if (producedCargo > 1)
                ptr = stringmgr::format_string(ptr, string_ids::cargo_and);

            auto cargoObj = objectmgr::get<cargo_object>(industryObj->produced_cargo_type[1]);
            ptr = stringmgr::format_string(ptr, cargoObj->name);
        }
    }

    // 0x00453275
    void industry::update()
    {
        if (!(flags & industry_flags::flag_01) && var_11 == 0xFF)
        {
            // Run tile loop for 100 iterations
            auto obj = object();
            for (int i = 0; i < 100; i++)
            {
                const auto& surface = tilemgr::get(tile_loop.current()).surface();
                if (surface != nullptr)
                {
                    if (surface->data()[0] & 0x80)
                    {
                        if (surface->industry_id() == id())
                        {
                            uint8_t bl = surface->data()[6] >> 5;
                            if (bl == 0 || bl != obj->var_EA)
                            {
                                var_DB++;
                                if ((!(obj->var_E4 & 0x10000000) && (surface->data()[4] & 0xE0) == 0) || find_5(surface))
                                {
                                    var_DD++;
                                }
                            }
                        }
                    }
                }
                if (tile_loop.next() == map_pos())
                {
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
                    break;
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
