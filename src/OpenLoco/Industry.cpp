#include "Industry.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "Map/AnimationManager.h"
#include "Map/TileManager.h"
#include "Objects/CargoObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "StationManager.h"
#include "Utility/Numeric.hpp"
#include <algorithm>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco
{
    const std::array<Unk4F9274, 1> word_4F9274 = {
        Unk4F9274{ { 0, 0 }, 0 },
    };
    const std::array<Unk4F9274, 4> word_4F927C = {
        Unk4F9274{ { 0, 0 }, 0 },
        Unk4F9274{ { 0, 32 }, 1 },
        Unk4F9274{ { 32, 32 }, 2 },
        Unk4F9274{ { 32, 0 }, 3 },
    };
    const stdx::span<const Unk4F9274> getUnk4F9274(bool type)
    {
        if (type)
            return word_4F927C;
        return word_4F9274;
    }

    const IndustryObject* Industry::getObject() const
    {
        return ObjectManager::get<IndustryObject>(object_id);
    }

    bool Industry::empty() const
    {
        return name == StringIds::null;
    }

    bool Industry::canReceiveCargo() const
    {
        auto receiveCargoState = false;
        for (const auto& receivedCargo : ObjectManager::get<IndustryObject>(object_id)->required_cargo_type)
        {
            if (receivedCargo != 0xff)
                receiveCargoState = true;
        }
        return receiveCargoState;
    }

    bool Industry::canProduceCargo() const
    {
        auto produceCargoState = false;
        for (const auto& producedCargo : ObjectManager::get<IndustryObject>(object_id)->produced_cargo_type)
        {
            if (producedCargo != 0xff)
                produceCargoState = true;
        }
        return produceCargoState;
    }

    static bool findTree(SurfaceElement* surface)
    {
        auto element = surface;
        while (!element->isLast())
        {
            element++;
            if (element->type() == ElementType::tree)
            {
                return true;
            }
        }
        return false;
    }

    // 0x0045935F
    void Industry::getStatusString(const char* buffer)
    {
        char* ptr = (char*)buffer;
        *ptr = '\0';
        const auto* industryObj = getObject();

        // Closing Down
        if (flags & IndustryFlags::closingDown)
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
    void Industry::update()
    {
        if (!(flags & IndustryFlags::flag_01) && under_construction == 0xFF)
        {
            // Run tile loop for 100 iterations
            for (int i = 0; i < 100; i++)
            {
                sub_45329B(tile_loop.current());

                // loc_453318
                if (tile_loop.next() == Pos2())
                {
                    sub_453354();
                    break;
                }
            }
        }
    }

    // 0x0045329B
    void Industry::sub_45329B(const Pos2& pos)
    {
        const auto& surface = TileManager::get(pos).surface();
        if (surface != nullptr)
        {
            if (surface->hasHighTypeFlag())
            {
                if (surface->industryId() == id())
                {
                    uint8_t bl = surface->var_6_SLR5();
                    const auto* obj = getObject();
                    if (bl == 0 || bl != obj->var_EA)
                    {
                        // loc_4532E5
                        var_DB++;
                        if ((!(obj->flags & IndustryObjectFlags::flag_28) && surface->var_4_E0() != 0) || findTree(surface))
                        {
                            var_DD++;
                        }
                    }
                }
            }
        }
    }

    void Industry::sub_453354()
    {
        // 0x00453366
        int16_t tmp_a = var_DB / 16;
        int16_t tmp_b = std::max(0, var_DD - tmp_a);
        int16_t tmp_c = var_DB - tmp_b;
        int16_t tmp_d = std::min(tmp_c / 25, 255);

        const auto* obj = getObject();
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
                    Map::Pos2 randTile{ static_cast<coord_t>(x + (prng.randNext(-15, 16) * 32)), static_cast<coord_t>(y + (prng.randNext(-15, 16) * 32)) };
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

    void Industry::sub_454A43(const Pos2& pos, uint8_t bl, uint8_t bh, uint8_t dl)
    {
        registers regs;
        regs.bl = bl;
        regs.bh = bh;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.dl = dl;
        regs.dh = enumValue(id());
        call(0x00454A43, regs);
    }

    // 0x00459D43
    void Industry::createMapAnimations()
    {
        for (size_t i = 0; i < numTiles; i++)
        {
            auto& tilePos = tiles[i];
            auto baseZ = (tilePos.z & ~Location::null) / 4;
            auto tile = TileManager::get(tilePos);

            for (auto& el : tile)
            {
                auto* industryEl = el.as<IndustryElement>();
                if (industryEl == nullptr)
                    continue;

                if (industryEl->baseZ() != baseZ)
                    continue;

                auto tileIndustry = industryEl->industry();
                if (tileIndustry != nullptr)
                {
                    const auto* industryObject = tileIndustry->getObject();
                    if (industryObject != nullptr)
                    {
                        auto animOffsets = getUnk4F9274(industryObject->var_C6 & (1 << industryEl->var_6_1F()));
                        for (auto animOffset : animOffsets)
                        {
                            AnimationManager::createAnimation(3, animOffset.pos + tilePos, baseZ);
                        }
                    }
                }
            }
        }
    }

    // 0x004574F7
    void Industry::updateProducedCargoStats()
    {
        const auto* industryObj = getObject();

        for (auto cargoNum = 0; cargoNum < 2; ++cargoNum)
        {
            auto& indStatsStation = producedCargoStatsStation[cargoNum];
            auto& indStatsRating = producedCargoStatsRating[cargoNum];
            std::fill(std::begin(indStatsStation), std::end(indStatsStation), StationId::null);
            const auto cargoType = industryObj->produced_cargo_type[cargoNum];
            if (cargoType == 0xFF)
            {
                continue;
            }

            for (auto dword = 0; dword < 32; ++dword)
            {
                auto bits = var_E1[dword];
                for (auto bit = Utility::bitScanForward(bits); bit != -1; bit = Utility::bitScanForward(bits))
                {
                    bits &= ~(1 << bit);
                    const auto stationId = static_cast<StationId>((dword << 5) | bit);
                    const auto* station = StationManager::get(stationId);
                    if (station->empty())
                    {
                        continue;
                    }

                    const auto& cargoStats = station->cargoStats[cargoType];
                    if (!(cargoStats.flags & (1 << 1)))
                    {
                        continue;
                    }

                    const auto rating = cargoStats.rating;
                    for (auto index = 0; index < 4; ++index)
                    {
                        if (indStatsStation[index] == StationId::null || indStatsRating[index] <= rating)
                        {
                            // This is an insertion sort.
                            // Rotate so that we overwrite the last entry
                            std::rotate(std::begin(indStatsStation) + index, std::end(indStatsStation) - 1, std::end(indStatsStation));
                            std::rotate(std::begin(indStatsRating) + index, std::end(indStatsRating) - 1, std::end(indStatsRating));
                            indStatsStation[index] = stationId;
                            indStatsRating[index] = rating;
                            break;
                        }
                    }
                }
            }

            uint8_t ratingFraction = 0xFF;
            for (auto& rating : indStatsRating)
            {
                rating = (rating * ratingFraction) / 256;
                ratingFraction = -rating;
            }
        }
        std::fill(std::begin(var_E1), std::end(var_E1), 0);
    }
}
