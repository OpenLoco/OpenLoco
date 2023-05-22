#include "Date.h"
#include "GameCommands.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "World/IndustryManager.h"
#include "World/TownManager.h"

namespace OpenLoco::GameCommands
{
    static bool sub_454C91(uint8_t type, const World::Pos2& pos, const Core::Prng& prng)
    {
        const auto* indObj = ObjectManager::get<IndustryObject>(type);

        // TODO: MAKE THIS A FUNCTION
        // 0x00E0C38C
        uint32_t producedCargoTypes = 0;
        for (auto& cargoType : indObj->producedCargoType)
        {
            if (cargoType != 0xFFU)
            {
                producedCargoTypes |= (1U << cargoType);
            }
        }

        // 0x00E0C390
        uint32_t requiredCargoTypes = 0;
        for (auto& cargoType : indObj->requiredCargoType)
        {
            if (cargoType != 0xFFU)
            {
                requiredCargoTypes |= (1U << cargoType);
            }
        }

        for (const auto& ind : IndustryManager::industries())
        {
            const auto distance = Math::Vector::manhattanDistance(World::Pos2{ ind.x, ind.y }, pos);

            const auto* indObj2 = ind.getObject();
            uint32_t producedCargoTypes2 = 0;
            for (auto& cargoType : indObj2->producedCargoType)
            {
                if (cargoType != 0xFFU)
                {
                    producedCargoTypes2 |= (1U << cargoType);
                }
            }

            uint32_t requiredCargoTypes2 = 0;
            for (auto& cargoType : indObj2->requiredCargoType)
            {
                if (cargoType != 0xFFU)
                {
                    requiredCargoTypes2 |= (1U << cargoType);
                }
            }

            const auto hasAtLeast1SameCargo = ((producedCargoTypes & producedCargoTypes2) != 0)
                || ((requiredCargoTypes & requiredCargoTypes2) != 0);

            const auto tooClose = 32 * (hasAtLeast1SameCargo ? 24 : 9);

            if (distance < tooClose)
            {
                GameCommands::setErrorText(StringIds::too_close_to_another_industry);
                return false;
            }
        }

        // Find free industry slot (MOVE TO INDUSTRY MANAGER)
        for (IndustryId id = static_cast<IndustryId>(0); enumValue(id) < Limits::kMaxIndustries; id = static_cast<IndustryId>(enumValue(id) + 1))
        {
            auto* industry = IndustryManager::get(id);
            if (!industry->empty())
            {
                continue;
            }

            industry->prng = prng;
            industry->flags = IndustryFlags::none;
            industry->objectId = type;
            industry->x = pos.x;
            industry->y = pos.y;
            industry->numTiles = 0;
            industry->under_construction = 0;
            industry->tileLoop = World::TileLoop{};
            industry->var_DB = 0;
            industry->var_DD = 0;
            industry->var_DF = 0;
            industry->foundingYear = getCurrentYear();
            industry->var_E1 = {};
            for (auto& stats : industry->producedCargoStatsStation)
            {
                std::fill(std::begin(stats), std::end(stats), StationId::null);
            }
            std::fill(std::begin(industry->var_17D), std::end(industry->var_17D), 0);
            std::fill(std::begin(industry->var_181), std::end(industry->var_181), 0);
            std::fill(std::begin(industry->producedCargoQuantityMonthlyTotal), std::end(industry->producedCargoQuantityMonthlyTotal), 0);
            std::fill(std::begin(industry->producedCargoQuantityPreviousMonth), std::end(industry->producedCargoQuantityPreviousMonth), 0);
            std::fill(std::begin(industry->receivedCargoQuantityMonthlyTotal), std::end(industry->receivedCargoQuantityMonthlyTotal), 0);
            std::fill(std::begin(industry->receivedCargoQuantityPreviousMonth), std::end(industry->receivedCargoQuantityPreviousMonth), 0);
            std::fill(std::begin(industry->receivedCargoQuantityDailyTotal), std::end(industry->receivedCargoQuantityDailyTotal), 0);
            std::fill(std::begin(industry->producedCargoQuantityDeliveredMonthlyTotal), std::end(industry->producedCargoQuantityDeliveredMonthlyTotal), 0);
            std::fill(std::begin(industry->producedCargoQuantityDeliveredPreviousMonth), std::end(industry->producedCargoQuantityDeliveredPreviousMonth), 0);
            std::fill(std::begin(industry->producedCargoPercentTransportedPreviousMonth), std::end(industry->producedCargoPercentTransportedPreviousMonth), 0);
            std::fill(std::begin(industry->producedCargoMonthlyHistorySize), std::end(industry->producedCargoMonthlyHistorySize), 1);
            // Note: vanilla just set to 0 first entry
            std::fill(std::begin(industry->producedCargoMonthlyHistory1), std::end(industry->producedCargoMonthlyHistory1), 0);
            std::fill(std::begin(industry->producedCargoMonthlyHistory2), std::end(industry->producedCargoMonthlyHistory2), 0);
            std::fill(std::begin(industry->history_min_production), std::end(industry->history_min_production), 0);

            const auto res = TownManager::getClosestTownAndDensity(pos);
            if (!res.has_value())
            {
                GameCommands::setErrorText(StringIds::town_must_be_built_nearby_first);
                return false;
            }
            industry->town = res->first;
            industry->name = indObj->var_02;

            for (auto& innerInd : IndustryManager::industries())
            {
                if (innerInd.name != industry->name)
                {
                    continue;
                }
                if (&innerInd == industry)
                {
                    continue;
                }
                if (innerInd.town != industry->town)
                {
                    continue;
                }

                for (auto unique = 1; unique < 0xFFF; ++unique)
                {
                    FormatArguments args{};
                    args.push<uint16_t>(unique);
                    char buffer[512]{};
                    StringManager::formatString(buffer, indObj->var_02 + 1, &args);
                    const auto newName = StringManager::userStringAllocate(buffer, 0);
                    if (newName == StringIds::empty)
                    {
                        continue;
                    }
                    industry->name = newName;
                    break;
                }
            }
            return true;
        }

        GameCommands::setErrorText(StringIds::too_many_industries);
        return false;
    }
}
