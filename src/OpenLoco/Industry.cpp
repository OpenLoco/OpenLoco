#include "Industry.h"
#include "Date.h"
#include "GameCommands/GameCommands.h"
#include "IndustryManager.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "Map/AnimationManager.h"
#include "Map/TileManager.h"
#include "Math/Bound.hpp"
#include "MessageManager.h"
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

    // 0x004534BD
    void Industry::updateDaily()
    {
        if (flags & IndustryFlags::flag_01)
        {
            return;
        }

        auto* indObj = getObject();

        uint16_t production = 0;
        if (indObj->flags & IndustryObjectFlags::requires_all_cargo)
        {
            production = std::numeric_limits<uint16_t>::max();
            for (auto i = 0; i < 3; ++i)
            {
                if (indObj->required_cargo_type[i] != 0xFF)
                {
                    production = std::min(production, var_199[i]);
                }
            }
            if (production != 0)
            {
                for (auto i = 0; i < 3; ++i)
                {
                    if (indObj->required_cargo_type[i] != 0xFF)
                    {
                        var_199[i] -= production;
                    }
                }
            }
        }
        else
        {
            for (auto i = 0; i < 3; ++i)
            {
                if (indObj->required_cargo_type[i] != 0xFF)
                {
                    production = Math::Bound::add(production, var_199[i]);
                    var_199[i] = 0;
                }
            }
        }
        if (production != 0)
        {
            for (auto i = 0; i < 2; ++i)
            {
                if (indObj->produced_cargo_type[i] != 0xFF)
                {
                    var_181[i] = Math::Bound::add(var_181[i], production);
                }
            }
        }

        for (auto i = 0; i < 3; ++i)
        {
            // Lose a 16th of required cargo if requires_all_cargo and not equally satisfied
            var_199[i] -= Math::Bound::add(var_199[i], 15) / 16;
        }

        for (auto i = 0; i < 2; ++i)
        {
            if (indObj->produced_cargo_type[i] == 0xFF)
            {
                continue;
            }

            uint16_t ax = (var_179[i] * var_DF) / 256;
            if (ax < var_17D[i])
            {
                var_17D[i]--;
            }
            else if (ax > var_17D[i])
            {
                var_17D[i]++;
            }

            var_181[i] = Math::Bound::add(var_17D[i], var_181[i]);

            if (var_181[i] >= 8)
            {
                const auto max = std::min<uint16_t>(var_181[i], 255);
                var_181[i] -= max;
                var_185[i] = Math::Bound::add(var_185[i], max);

                std::vector<StationId> stations;
                for (auto stationId : producedCargoStatsStation[i])
                {
                    if (stationId != StationId::null)
                    {
                        stations.push_back(stationId);
                    }
                }

                const auto remainingCargo = StationManager::deliverCargoToStations(stations, indObj->produced_cargo_type[i], max);
                var_19F[i] = Math::Bound::add(remainingCargo, var_19F[i]);
            }
        }
    }

    // 0x00453868
    void Industry::updateMonthly()
    {
        if (flags & IndustryFlags::flag_01)
        {
            return;
        }

        if (flags & IndustryFlags::closingDown && var_17D[0] == 0 && var_17D[1] == 0)
        {
            GameCommands::do_48(GameCommands::Flags::apply, id());
            return;
        }
        bool hasEvent = false;
        const auto* indObj = getObject();
        if (under_construction == 0xFF
            && !(flags & IndustryFlags::closingDown)
            && indObj->required_cargo_type[0] == 0xFF)
        {
            if (indObj->flags & IndustryObjectFlags::unk18
                && produced_cargo_transported[0] > 70
                && gPrng().randNext(31) == 0
                && var_179[0] < 100
                && var_179[1] < 100)
            {
                var_179[0] = std::min(100, var_179[0] * 2);
                var_179[1] = std::min(100, var_179[1] * 2);
                MessageManager::post(MessageType::industryProductionUp, CompanyId::null, enumValue(id()), 0xFFFF);
                hasEvent = true;
            }
            else if (indObj->flags & IndustryObjectFlags::unk19 && ((produced_cargo_transported[0] > 50 && var_179[0] > 20 && gPrng().randNext(31) == 0) || (produced_cargo_transported[0] <= 50 && var_179[0] > 10 && gPrng().randNext(15) == 0)))
            {
                var_179[0] /= 2;
                var_179[1] /= 2;
                MessageManager::post(MessageType::industryProductionDown, CompanyId::null, enumValue(id()), 0xFFFF);
                hasEvent = true;
            }
        }
        if (!hasEvent
            && !(IndustryManager::getFlags() & IndustryManager::Flags::disallowIndustriesCloseDown)
            && under_construction == 0xFF
            && !(flags & IndustryFlags::closingDown))
        {
            if ((getCurrentYear() > indObj->obsoleteYear
                 && prng.randNext(0xFFFF) < 102)
                || (indObj->var_F3 != 0
                    && indObj->var_F3 > prng.randNext(0xFFFF)))
            {
                flags |= IndustryFlags::closingDown;
                var_179[0] = 0;
                var_179[1] = 0;
                MessageManager::post(MessageType::industryClosingDown, CompanyId::null, enumValue(id()), 0xFFFF);
            }
        }

        if (historySize[0] == std::size(history_1))
        {
            std::rotate(std::begin(history_1), std::begin(history_1) + 1, std::end(history_1));
        }
        else
        {
            historySize[0]++;
        }
        const auto newValue = std::min<uint32_t>(var_185[0], 12750u) / 50;
        history_1[historySize[0] - 1] = newValue;

        if (historySize[1] == std::size(history_2))
        {
            std::rotate(std::begin(history_2), std::begin(history_2) + 1, std::end(history_2));
        }
        else
        {
            historySize[1]++;
        }
        const auto newValue2 = std::min<uint32_t>(var_185[1], 12750u) / 50;
        history_2[historySize[1] - 1] = newValue2;

        produced_cargo_quantity[0] = var_185[0];
        var_185[0] = 0;
        produced_cargo_max[0] = var_19F[0];
        var_19F[0] = 0;
        auto transported = std::min(produced_cargo_quantity[0], produced_cargo_max[0]);
        if (produced_cargo_quantity[0] != 0)
        {
            transported = (transported * 100) / produced_cargo_quantity[0];
        }
        produced_cargo_transported[0] = transported;

        produced_cargo_quantity[1] = var_185[1];
        var_185[1] = 0;
        produced_cargo_max[1] = var_19F[1];
        var_19F[1] = 0;
        auto transported2 = std::min(produced_cargo_quantity[1], produced_cargo_max[1]);
        if (produced_cargo_quantity[1] != 0)
        {
            transported2 = (transported2 * 100) / produced_cargo_quantity[1];
        }
        produced_cargo_transported[1] = transported2;

        required_cargo_quantity[0] = var_18D[0];
        required_cargo_quantity[1] = var_18D[1];
        required_cargo_quantity[2] = var_18D[2];
        var_18D[0] = 0;
        var_18D[1] = 0;
        var_18D[2] = 0;
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

            auto id = 0;
            for (const auto& hasBit : var_E1)
            {
                const auto stationId = static_cast<StationId>(id);
                id++;
                if (!hasBit)
                {
                    continue;
                }
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

            uint8_t ratingFraction = 0xFF;
            for (auto& rating : indStatsRating)
            {
                rating = (rating * ratingFraction) / 256;
                ratingFraction = -rating;
            }
        }
        var_E1.reset();
    }
}
