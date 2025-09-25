#include "AiCreateTrackAndStation.h"
#include "GameCommands/Track/CreateTrack.h"
#include "GameCommands/Track/CreateTrainStation.h"
#include "Map/BuildingElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"

namespace OpenLoco::GameCommands
{
    static loco_global<uint8_t, 0x01136073> _byte_1136073;

    // 0x004A7328
    static World::TileClearance::ClearFuncResult clearNearbyArea(World::TileElement& el)
    {
        if (el.type() == World::ElementType::tree)
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

    static currency32_t aiCreateTrackAndStation(const AiTrackAndStationPlacementArgs& args, uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction);
        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return GameCommands::FAILURE;
        }
        currency32_t totalCost = 0;
        int32_t numTilesTrackOrRoadUnderneath = 0;
        auto trackPos = args.pos;
        for (auto i = 0U; i < args.stationLength; ++i, trackPos += World::Pos3(World::kRotationOffset[args.rotation], 0))
        {
            if (!(flags & Flags::apply))
            {
                if (World::TileManager::validCoords(trackPos))
                {
                    auto tile = World::TileManager::get(trackPos);
                    bool passedSurface = false;
                    for (const auto& el : tile)
                    {
                        const auto type = el.type();
                        if (type == World::ElementType::surface)
                        {
                            passedSurface = true;
                            continue;
                        }
                        if (!passedSurface)
                        {
                            continue;
                        }
                        if (trackPos.z <= el.baseHeight())
                        {
                            break;
                        }
                        if ((type == World::ElementType::road) || (type == World::ElementType::track))
                        {
                            ++numTilesTrackOrRoadUnderneath;
                            break;
                        }
                    }
                }
            }
            {
                auto trackArgs = TrackPlacementArgs{};
                trackArgs.pos = trackPos;
                trackArgs.rotation = args.rotation;
                trackArgs.trackObjectId = args.trackObjectId;
                trackArgs.trackId = 0; // Always straight track
                trackArgs.mods = args.mods;
                trackArgs.bridge = args.bridge;
                trackArgs.unkFlags = 0;
                trackArgs.unk = false;

                auto trackRegs = static_cast<registers>(trackArgs);
                trackRegs.bl = flags;
                createTrack(trackRegs);
                const auto trackRes = static_cast<currency32_t>(trackRegs.ebx);
                if (!(flags & GameCommands::Flags::apply))
                {
                    if (static_cast<uint32_t>(trackRes) == GameCommands::FAILURE)
                    {
                        return GameCommands::FAILURE;
                    }

                    // There is a level crossing or track overlay so we can't place a station
                    if (_byte_1136073 & ((1U << 2) | (1U << 3)))
                    {
                        return GameCommands::FAILURE;
                    }
                }
                totalCost += trackRes;
            }
            {
                auto stationArgs = TrainStationPlacementArgs{};
                stationArgs.pos = trackPos;
                stationArgs.rotation = args.rotation;
                stationArgs.type = args.stationObjectId;
                stationArgs.trackObjectId = args.trackObjectId;
                stationArgs.trackId = 0; // Always straight track
                stationArgs.index = 0;   // Always index 0 for straight track

                auto stationRegs = static_cast<registers>(stationArgs);
                stationRegs.bl = flags;
                createTrainStation(stationRegs);
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
        }

        // Ensure there is enough space for the station track to exit
        if (args.unk1 & (1U << 1))
        {
            auto pos = World::Pos2{ args.pos } - World::kRotationOffset[args.rotation];
            for (auto i = 0U; i < 2; ++i)
            {
                const auto baseZ = args.pos.z / World::kSmallZStep;
                if (!World::TileClearance::applyClearAtStandardHeight(pos, baseZ, baseZ + 12, World::QuarterTile{ 0xF, 0 }, clearNearbyArea))
                {
                    return GameCommands::FAILURE;
                }
                pos -= World::kRotationOffset[args.rotation];
            }
        }

        if (args.unk1 & (1U << 0))
        {
            auto pos = World::Pos2{ args.pos } + World::kRotationOffset[args.rotation] * args.stationLength;
            for (auto i = 0U; i < 2; ++i)
            {
                const auto baseZ = args.pos.z / World::kSmallZStep;
                if (!World::TileClearance::applyClearAtStandardHeight(pos, baseZ, baseZ + 12, World::QuarterTile{ 0xF, 0 }, clearNearbyArea))
                {
                    return GameCommands::FAILURE;
                }
                pos += World::kRotationOffset[args.rotation];
            }
        }
        if (!(flags & Flags::apply))
        {
            if (numTilesTrackOrRoadUnderneath >= args.stationLength - 2)
            {
                return GameCommands::FAILURE;
            }
        }
        return totalCost;
    }

    // 0x004A6FDC
    void aiCreateTrackAndStation(registers& regs)
    {
        regs.ebx = aiCreateTrackAndStation(AiTrackAndStationPlacementArgs(regs), regs.bl);
    }
}
