#include "Industry.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "Objects/CargoObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "Utility/Numeric.hpp"
#include <algorithm>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco
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
        return name == StringIds::null;
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

    static bool findTree(surface_element* surface)
    {
        auto element = surface;
        while (!element->isLast())
        {
            element++;
            if (element->type() == element_type::tree)
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
            ptr = StringManager::formatString(ptr, StringIds::industry_closing_down);
            return;
        }

        // Under Construction
        if (under_construction != 0xFF)
        {
            ptr = StringManager::formatString(ptr, StringIds::industry_under_construction);
            return;
        }

        // Produced Cargo Only
        if (!canReceiveCargo())
        {
            if (!canProduceCargo())
                return;

            ptr = StringManager::formatString(ptr, StringIds::industry_producing);

            ptr = industryObj->getProducedCargoString(ptr);

            return;
        }

        // Required Cargo
        ptr = StringManager::formatString(ptr, StringIds::industry_requires);

        ptr = industryObj->getRequiredCargoString(ptr);

        if (!canProduceCargo())
            return;

        // Production and Received Cargo
        ptr = StringManager::formatString(ptr, StringIds::cargo_to_produce);

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

                // loc_453318
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
        const auto& surface = TileManager::get(pos).surface();
        if (surface != nullptr)
        {
            if (surface->hasHighTypeFlag())
            {
                if (surface->industryId() == id())
                {
                    uint8_t bl = surface->var_6_SLR5();
                    auto obj = object();
                    if (bl == 0 || bl != obj->var_EA)
                    {
                        // loc_4532E5
                        var_DB++;
                        if ((!(obj->flags & industry_object_flags::flag_28) && surface->var_4_E0() != 0) || findTree(surface))
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
        // 0x00453366
        int16_t tmp_a = var_DB / 16;
        int16_t tmp_b = std::max(0, var_DD - tmp_a);
        int16_t tmp_c = var_DB - tmp_b;
        int16_t tmp_d = std::min(tmp_c / 25, 255);

        auto obj = object();
        if (tmp_d < obj->var_EB)
        {
            var_DF = ((tmp_d * 256) / obj->var_EB) & 0xFF;
        }
        else
        {
            // 0x0045335F; moved here.
            var_DF = 255;
        }

        // 0x004533B2
        var_DB = 0;
        var_DD = 0;
        if (var_DF < 224)
        {
            if (produced_cargo_quantity[0] / 8 <= produced_cargo_max[0] || produced_cargo_quantity[1] / 8 <= produced_cargo_max[1])
            {
                if (prng.randBool())
                {
                    Map::map_pos randTile{ static_cast<coord_t>(x + (prng.randNext(-15, 16) * 32)), static_cast<coord_t>(y + (prng.randNext(-15, 16) * 32)) };
                    uint8_t bl = obj->var_ED;
                    uint8_t bh = obj->var_EE;
                    if (obj->var_EF != 0xFF && prng.randBool())
                    {
                        bl = obj->var_EF;
                        bh = obj->var_F0;
                    }
                    uint8_t dl = prng.randNext(7) * 32;
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
