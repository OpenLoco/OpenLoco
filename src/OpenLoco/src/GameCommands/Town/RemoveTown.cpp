#include "Audio/Audio.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Map/BuildingElement.h"
#include "Map/RoadElement.h"
#include "Map/TileElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "MessageManager.h"
#include "Objects/ObjectManager.h"
#include "S5/S5.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "World/IndustryManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    // 0x0049711F
    static uint32_t removeTown(const TownRemovalArgs& args, const uint8_t flags)
    {
        for (auto& station : StationManager::stations())
        {
            if (station.town == args.townId)
            {
                setErrorText(StringIds::all_stations_near_this_town_must_be_removed_first);
                return FAILURE;
            }
        }

        if (!(flags & Flags::apply))
        {
            return 0;
        }

        // NB: vanilla did not set an expenditute type
        GameCommands::setExpenditureType(ExpenditureType::Construction);

        // Iterate over the entire map to find town tiles
        // TODO: can't we do better than vanilla for this? e.g. a radius around the town centre?
        for (auto& tilePos : getWorldRange())
        {
            auto tile = TileManager::get(tilePos);

            bool resetTileLoop = true;
            while (resetTileLoop)
            {
                resetTileLoop = false;
                for (auto& element : tile)
                {
                    if (element.isGhost())
                    {
                        continue;
                    }

                    auto* buildingEl = element.as<BuildingElement>();
                    if (buildingEl != nullptr)
                    {
                        if (buildingEl->has_40())
                        {
                            continue;
                        }

                        if (buildingEl->multiTileIndex() != 0)
                        {
                            continue;
                        }

                        auto worldPos = toWorldSpace(tilePos);
                        auto maybeTown = TownManager::getClosestTownAndDensity(worldPos);
                        if (maybeTown)
                        {
                            TownId nearestTown = maybeTown->first;
                            if (nearestTown != args.townId)
                            {
                                continue;
                            }
                        }

                        BuildingRemovalArgs rmArgs{};
                        rmArgs.pos = Pos3(worldPos.x, worldPos.y, buildingEl->baseHeight());
                        if (doCommand(rmArgs, flags) != FAILURE)
                        {
                            resetTileLoop = true;
                            break;
                        }

                        continue;
                    }

                    auto* roadEl = element.as<RoadElement>();
                    if (roadEl != nullptr)
                    {
                        if (roadEl->owner() != CompanyId::neutral)
                        {
                            continue;
                        }

                        auto worldPos = toWorldSpace(tilePos);
                        auto maybeTown = TownManager::getClosestTownAndDensity(worldPos);
                        if (maybeTown)
                        {
                            TownId nearestTown = maybeTown->first;
                            if (nearestTown != args.townId)
                            {
                                continue;
                            }
                        }

                        RoadRemovalArgs rmArgs{};
                        rmArgs.pos = Pos3(worldPos.x, worldPos.y, roadEl->baseHeight());
                        rmArgs.unkDirection = roadEl->unkDirection();
                        rmArgs.roadId = roadEl->roadId();
                        rmArgs.sequenceIndex = roadEl->sequenceIndex();
                        rmArgs.objectId = roadEl->roadObjectId();
                        if (doCommand(rmArgs, flags) != FAILURE)
                        {
                            resetTileLoop = true;
                            break;
                        }
                    }
                }
            }
        }

        Ui::WindowManager::close(Ui::WindowType::town, enumValue(args.townId));

        auto* town = TownManager::get(args.townId);
        auto oldTownCentre = Pos2(town->x, town->y);

        StringManager::emptyUserString(town->name);
        town->name = StringIds::null;

        Ui::Windows::TownList::removeTown(args.townId);

        MessageManager::removeAllSubjectRefs(enumValue(args.townId), MessageItemArgumentType::town);

        for (auto& industry : IndustryManager::industries())
        {
            if (industry.town == args.townId)
            {
                IndustryRemovalArgs rmArgs{};
                rmArgs.industryId = industry.id();
                doCommand(rmArgs, flags);
            }
        }

        auto tileHeight = TileManager::getHeight(oldTownCentre);
        setPosition({ oldTownCentre.x, oldTownCentre.y, tileHeight.landHeight });

        TownManager::resetBuildingsInfluence();

        auto& options = S5::getOptions();
        options.madeAnyChanges = 1;

        return 0;
    }

    void removeTown(registers& regs)
    {
        TownRemovalArgs args(regs);
        regs.ebx = removeTown(args, regs.bl);
    }
}
