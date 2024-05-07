#include "RemoveBuilding.h"
#include "Economy/Economy.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/BuildingElement.h"
#include "Map/TileManager.h"
#include "Objects/BuildingObject.h"
#include "S5/S5.h"
#include "SceneManager.h"
#include "World/CompanyManager.h"
#include "World/Industry.h"
#include "World/TownManager.h"

namespace OpenLoco::GameCommands
{
    // 0x00497D8D
    static std::optional<int16_t> getCompanyRating(World::Pos2& pos)
    {
        auto companyId = CompanyManager::getUpdatingCompanyId();
        if (companyId != CompanyId::neutral)
        {
            auto res = TownManager::getClosestTownAndDensity(pos);
            if (res.has_value())
            {
                auto townId = res->first;
                auto town = TownManager::get(townId);
                if (town != nullptr)
                {
                    return town->companyRatings[enumValue(companyId)];
                }
            }
        }
        return std::nullopt;
    }

    // 0x0042D74E
    static uint32_t removeBuilding(World::Pos3& pos, uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction);
        GameCommands::setPosition(pos);

        auto tile = World::TileManager::get(pos);

        for (auto& el : tile)
        {
            auto* elBuilding = el.as<World::BuildingElement>();
            if (elBuilding == nullptr)
                continue;

            if (elBuilding->baseZ() != pos.z / World::kSmallZStep)
                continue;

            const auto* buildingObj = elBuilding->getObject();
            if (!isEditorMode() && !isSandboxMode())
            {
                if ((flags & (GameCommands::Flags::ghost | GameCommands::Flags::flag_7)) == 0)
                {
                    if (buildingObj->hasFlags(BuildingObjectFlags::undestructible))
                    {
                        GameCommands::setErrorText(StringIds::demolition_not_allowed);
                        return GameCommands::FAILURE;
                    }

                    if (!buildingObj->hasFlags(BuildingObjectFlags::miscBuilding))
                    {
                        auto rating = getCompanyRating(pos);
                        if (rating.has_value() && *rating < 0)
                        {
                            auto res = TownManager::getClosestTownAndDensity(pos);
                            auto* town = TownManager::get(res->first);
                            auto formatArgs = FormatArguments::common();
                            formatArgs.push(town->name);
                            GameCommands::setErrorText(StringIds::local_authority_refuses_permission);
                            return GameCommands::FAILURE;
                        }
                    }
                }
            }
            auto animOffsets = getBuildingTileOffsets(buildingObj->hasFlags(BuildingObjectFlags::largeTile));
            for (auto animOffset : animOffsets)
            {
                const auto subTilePos = animOffset.pos + pos;

                auto subTile = World::TileManager::get(subTilePos);
                for (auto& element : subTile)
                {
                    auto* subElBuilding = element.as<World::BuildingElement>();
                    if (subElBuilding == nullptr)
                        continue;

                    if (subElBuilding->baseZ() != pos.z / World::kSmallZStep)
                        continue;

                    if (flags & GameCommands::Flags::apply)
                    {
                        World::TileManager::removeBuildingElement(subElBuilding->get<World::BuildingElement>(), subTilePos);
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
