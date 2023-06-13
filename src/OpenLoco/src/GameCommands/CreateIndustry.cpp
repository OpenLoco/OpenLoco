#include "Date.h"
#include "GameCommands.h"
#include "Graphics/Colour.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "SceneManager.h"
#include "World/IndustryManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Utility/Numeric.hpp>

namespace OpenLoco::GameCommands
{
    static IndustryId sub_454C91(uint8_t type, const World::Pos2& pos, const Core::Prng& prng)
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
                return IndustryId::null;
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
                return IndustryId::null;
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
            return id;
        }

        GameCommands::setErrorText(StringIds::too_many_industries);
        return IndustryId::null;
    }

    // 0x0045436B
    currency32_t createIndustry(const IndustryPlacementArgs& args, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);
        {
            const auto centrePos = World::Pos2{ args.pos.x + 16, args.pos.y + 16 };
            const auto centreHeight = World::TileManager::getHeight(centrePos);
            GameCommands::setPosition({ args.pos.x, args.pos.y, centreHeight.landHeight });
        }

        Core::Prng prng{ args.srand0, args.srand1 };
        const auto newIndustryId = sub_454C91(args.type, args.pos, prng);
        if (newIndustryId == IndustryId::null)
        {
            return FAILURE;
        }
        //_industryLastPlacedId = newIndustryId;
        auto* newIndustry = IndustryManager::get(newIndustryId);
        auto* indObj = newIndustry->getObject();
        if (args.buildImmediately)
        {
            newIndustry->under_construction = 0xFF;
        }
        if (flags & Flags::flag_6)
        {
            newIndustry->flags |= IndustryFlags::isGhost;
        }
        if (!isEditorMode() && getUpdatingCompanyId() != CompanyId::neutral)
        {
            newIndustry->flags |= IndustryFlags::flag_04;
            newIndustry->owner = getUpdatingCompanyId();
        }

        // Note: Could use a fixed size vector as max size is Colour::max
        std::vector<Colour> availableColours;
        auto colourBitSet = indObj->availableColours;
        for (auto colourI32 = Utility::bitScanForward(colourBitSet); colourI32 != -1; colourI32 = Utility::bitScanForward(colourBitSet))
        {
            colourBitSet &= ~(1ULL << colourI32);
            availableColours.push_back(static_cast<Colour>(colourI32));
        }

        // 0x00E0C3D3
        const auto randColour = [&availableColours, &newIndustry]() {
            if (availableColours.empty())
            {
                return Colour::black;
            }
            return availableColours[newIndustry->prng.randNext(availableColours.size())];
        }();

        // 0x00E0C3BE - C0
        const auto posUnk = World::Pos2{ newIndustry->x, newIndustry->y };

        // used also for 0x00454552 break up into two when function allowed to diverge
        const auto randVal = newIndustry->prng.randNext() & 0xFF;

        auto prodRateRand = randVal;
        for (auto i = 0; i < 2; ++i)
        {
            newIndustry->var_17D[i] = 0;
            newIndustry->productionRate[i] = (((indObj->initialProductionRate[i].max - indObj->initialProductionRate[i].min) * randVal) / 256) + indObj->initialProductionRate[i].min;
            if (isEditorMode())
            {
                newIndustry->var_17D[i] = newIndustry->productionRate[i];
                newIndustry->producedCargoQuantityPreviousMonth[i] = newIndustry->var_17D[i] * 30;
            }
            // This is odd but follows vanilla
            prodRateRand = newIndustry->productionRate[i] & 0xFF;
        }

        // 0x00454552
        const auto numBuildings = (((indObj->maxNumBuildings - indObj->minNumBuildings + 1) * randVal) / 256) + indObj->minNumBuildings;
        for (auto i = 0U; i < numBuildings; ++i)
        {
            const auto building = indObj->buildings[i];
            // 0x00454563
        }
    }

    void createIndustry(registers& regs)
    {
        IndustryPlacementArgs args(regs);
        regs.ebx = createIndustry(args, regs.bl);
    }
}
