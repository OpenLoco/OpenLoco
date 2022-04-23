#include "../CompanyManager.h"
#include "../Economy/Economy.h"
#include "../Industry.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/TileManager.h"
#include "../Objects/BuildingObject.h"
#include "../S5/S5.h"
#include "../TownManager.h"
#include "GameCommands.h"

namespace OpenLoco::GameCommands
{
    // 0x00497D8D
    static std::optional<int16_t> getCompanyRating(Map::Pos2& pos)
    {
        auto companyId = CompanyManager::getUpdatingCompanyId();
        if (companyId != CompanyId::neutral)
        {
            auto res = TownManager::getClosestTownAndUnk(pos);
            if (res.has_value())
            {
                auto townId = res->first;
                auto town = TownManager::get(townId);
                if (town != nullptr)
                {
                    return town->company_ratings[enumValue(companyId)];
                }
            }
        }
        return std::nullopt;
    }

    // 0x0042D74E
    static uint32_t removeBuilding(Map::Pos3& pos, uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction);
        GameCommands::setPosition(pos);

        auto tile = Map::TileManager::get(pos);

        for (auto& el : tile)
        {
            auto* elBuilding = el.as<Map::BuildingElement>();
            if (elBuilding == nullptr)
                continue;

            if (elBuilding->baseZ() != pos.z / Map::kSmallZStep)
                continue;

            const auto* buildingObj = elBuilding->getObject();
            if (!isEditorMode())
            {
                if ((flags & (GameCommands::Flags::flag_6 | GameCommands::Flags::flag_7)) == 0)
                {
                    if ((buildingObj->flags & BuildingObjectFlags::undestructible) != 0)
                    {
                        GameCommands::setErrorText(StringIds::demolition_not_allowed);
                        return GameCommands::FAILURE;
                    }

                    if ((buildingObj->flags & BuildingObjectFlags::misc_building) == 0)
                    {
                        auto rating = getCompanyRating(pos);
                        if (rating.has_value() && *rating < 0)
                        {
                            auto res = TownManager::getClosestTownAndUnk(pos);
                            auto* town = TownManager::get(res->first);
                            auto formatArgs = FormatArguments::common();
                            formatArgs.push(town->name);
                            GameCommands::setErrorText(StringIds::local_authority_refuses_permission);
                            return GameCommands::FAILURE;
                        }
                    }
                }
            }
            auto animOffsets = getUnk4F9274(buildingObj->flags & BuildingObjectFlags::large_tile);
            for (auto animOffset : animOffsets)
            {
                const auto subTilePos = animOffset.pos + pos;

                auto subTile = Map::TileManager::get(subTilePos);
                for (auto& element : subTile)
                {
                    auto* subElBuilding = element.as<Map::BuildingElement>();
                    if (subElBuilding == nullptr)
                        continue;

                    if (subElBuilding->baseZ() != pos.z / Map::kSmallZStep)
                        continue;

                    if (flags & GameCommands::Flags::apply)
                    {
                        Map::TileManager::removeBuildingElement(subElBuilding->get<Map::BuildingElement>(), subTilePos);
                        auto& options = S5::getOptions();
                        options.madeAnyChanges = 1;
                    }
                }
            }
            return Economy::getInflationAdjustedCost(buildingObj->clearCostFactor, buildingObj->clearCostIndex, 8);
        }
        return GameCommands::FAILURE;
    }

    void removeBuilding(registers& regs)
    {
        BuildingRemovalArgs args(regs);
        regs.ebx = removeBuilding(args.pos, regs.bl);
    }
}
