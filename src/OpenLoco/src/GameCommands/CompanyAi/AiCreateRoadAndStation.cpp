#include "AiCreateRoadAndStation.h"
#include "GameCommands/Road/CreateRoad.h"
#include "GameCommands/Road/CreateRoadStation.h"
#include "Map/BuildingElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"

namespace OpenLoco::GameCommands
{
    // 0x0047B0DC
    static World::TileClearance::ClearFuncResult clearNearbyArea(World::TileElement& el)
    {
        if (el.type() == World::ElementType::tree)
        {
            return World::TileClearance::ClearFuncResult::noCollision;
        }
        if (el.type() == World::ElementType::road)
        {
            return World::TileClearance::ClearFuncResult::noCollision;
        }
        if (el.type() == World::ElementType::building)
        {
            auto* elBuilding = el.as<World::BuildingElement>();
            if (elBuilding == nullptr)
            {
                return World::TileClearance::ClearFuncResult::noCollision;
            }
            auto* buildingObj = ObjectManager::get<BuildingObject>(elBuilding->objectId());
            if (buildingObj->hasFlags(BuildingObjectFlags::isHeadquarters | BuildingObjectFlags::indestructible))
            {
                return World::TileClearance::ClearFuncResult::collision;
            }
            return World::TileClearance::ClearFuncResult::noCollision;
        }
        return World::TileClearance::ClearFuncResult::collision;
    }

    static currency32_t aiCreateRoadAndStationCost(const AiRoadAndStationPlacementArgs& args, uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction);
        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return GameCommands::FAILURE;
        }
        currency32_t totalCost = 0;
        {
            auto roadArgs = RoadPlacementArgs{};
            roadArgs.pos = args.pos;
            roadArgs.rotation = args.rotation;
            roadArgs.roadObjectId = args.roadObjectId;
            roadArgs.roadId = 0; // Always straight road
            roadArgs.mods = args.mods;
            roadArgs.bridge = args.bridge;
            roadArgs.unkFlags = 0;

            auto roadRegs = static_cast<registers>(roadArgs);
            roadRegs.bl = flags;
            createRoad(roadRegs);
            const auto roadRes = static_cast<currency32_t>(roadRegs.ebx);
            if (!(flags & GameCommands::Flags::apply))
            {
                if (static_cast<uint32_t>(roadRes) == GameCommands::FAILURE)
                {
                    return GameCommands::FAILURE;
                }

                // There is a level crossing so we can't place a station
                if (getLegacyReturnState().flags_1136073 & (1U << 2))
                {
                    return GameCommands::FAILURE;
                }
            }
            totalCost += roadRes;
        }
        {
            auto stationArgs = RoadStationPlacementArgs{};
            stationArgs.pos = args.pos;
            stationArgs.rotation = args.rotation;
            stationArgs.type = args.stationObjectId;
            stationArgs.roadObjectId = args.roadObjectId;
            stationArgs.roadId = 0; // Always straight road
            stationArgs.index = 0;  // Always index 0 for straight road

            auto stationRegs = static_cast<registers>(stationArgs);
            stationRegs.bl = flags;
            createRoadStation(stationRegs);
            const auto stationRes = static_cast<currency32_t>(stationRegs.ebx);
            if (!(flags & GameCommands::Flags::apply))
            {
                if (static_cast<uint32_t>(stationRes) == GameCommands::FAILURE)
                {
                    return GameCommands::FAILURE;
                }
            }
            totalCost += stationRes;
        }

        if (args.unk1 & (1U << 1))
        {
            const auto pos = World::Pos2{ args.pos } - World::kRotationOffset[args.rotation];
            const auto baseZ = args.pos.z / World::kSmallZStep;
            if (!World::TileClearance::applyClearAtStandardHeight(pos, baseZ, baseZ + 12, World::QuarterTile{ 0xF, 0 }, clearNearbyArea))
            {
                return GameCommands::FAILURE;
            }
        }

        if (args.unk1 & (1U << 0))
        {
            const auto pos = World::Pos2{ args.pos } + World::kRotationOffset[args.rotation];
            const auto baseZ = args.pos.z / World::kSmallZStep;
            if (!World::TileClearance::applyClearAtStandardHeight(pos, baseZ, baseZ + 12, World::QuarterTile{ 0xF, 0 }, clearNearbyArea))
            {
                return GameCommands::FAILURE;
            }
        }
        return totalCost;
    }

    // 0x0047AF0B
    void aiCreateRoadAndStation(registers& regs)
    {
        regs.ebx = aiCreateRoadAndStationCost(AiRoadAndStationPlacementArgs(regs), regs.bl);
    }
}
