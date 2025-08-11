#include "RemoveRoad.h"
#include "Audio/Audio.h"
#include "Economy/Economy.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Objects/BridgeObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Random.h"
#include "RemoveRoadStation.h"
#include "ScenarioOptions.h"
#include "SceneManager.h"
#include "World/TownManager.h"

namespace OpenLoco::GameCommands
{
    using namespace World::TileManager;

    // Copy of `playTrackRemovalSound`
    static void playRoadRemovalSound(const World::Pos3 pos)
    {
        const auto frequency = gPrng2().randNext(17955, 26146);
        Audio::playSound(Audio::SoundId::demolish, pos, 0, frequency);
    }

    static World::RoadElement* getRoadElement(const World::Tile& tile, const RoadRemovalArgs& args, uint8_t sequenceIndex, uint8_t flags)
    {
        const auto baseZ = args.pos.z / World::kSmallZStep;
        const auto companyId = SceneManager::isEditorMode() ? CompanyId::neutral : getUpdatingCompanyId();

        for (auto& element : tile)
        {
            auto* elRoad = element.as<World::RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }
            if (elRoad->baseZ() != baseZ)
            {
                continue;
            }
            if (elRoad->rotation() != args.rotation)
            {
                continue;
            }
            if (elRoad->sequenceIndex() != sequenceIndex)
            {
                continue;
            }
            if (elRoad->roadObjectId() != args.objectId)
            {
                continue;
            }
            if (elRoad->roadId() != args.roadId)
            {
                continue;
            }
            if (elRoad->isGhost() != ((flags & Flags::ghost) != 0))
            {
                continue;
            }
            if (elRoad->isAiAllocated() != ((flags & Flags::aiAllocated) != 0))
            {
                continue;
            }
            // Ghost only as this is checked elsewhere for non-ghost so that
            // neutral company is always allowed
            if (((flags & Flags::ghost) != 0) && elRoad->owner() != companyId)
            {
                return nullptr;
            }

            return elRoad;
        }

        return nullptr;
    }

    // 0x00477A10
    static currency32_t roadRemoveCost(const RoadRemovalArgs& args, const World::TrackData::PreviewTrack roadPiece0, const World::Pos3 roadStart, const uint8_t flags)
    {
        const auto roadLoc = roadStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ roadPiece0.x, roadPiece0.y }, args.rotation), roadPiece0.z };

        auto tile = World::TileManager::get(roadLoc.x, roadLoc.y);
        auto* roadElPiece = getRoadElement(tile, args, roadPiece0.index, flags);
        if (roadElPiece == nullptr)
        {
            return 0;
        }

        currency32_t totalRemovalCost = 0;

        const auto* roadObj = ObjectManager::get<RoadObject>(roadElPiece->roadObjectId());
        {
            const auto trackBaseCost = Economy::getInflationAdjustedCost(roadObj->sellCostFactor, roadObj->costIndex, 10);
            const auto cost = (trackBaseCost * World::TrackData::getRoadMiscData(args.roadId).costFactor) / 256;
            totalRemovalCost += cost;
        }

        // Check mod removal costs
        for (auto i = 0U; i < 2; i++)
        {
            if (roadElPiece->hasMod(i))
            {
                const auto* trackExtraObj = ObjectManager::get<RoadExtraObject>(roadObj->mods[i]);
                const auto trackExtraBaseCost = Economy::getInflationAdjustedCost(trackExtraObj->sellCostFactor, trackExtraObj->costIndex, 10);
                const auto cost = (trackExtraBaseCost * World::TrackData::getRoadMiscData(args.roadId).costFactor) / 256;
                totalRemovalCost += cost;
            }
        }

        return totalRemovalCost;
    }

    // 0x004775A5
    static uint32_t removeRoad(const RoadRemovalArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3{ 16, 16, 0 });

        // 0x0047762D
        auto tile = World::TileManager::get(args.pos.x, args.pos.y);

        if (auto* roadEl = getRoadElement(tile, args, args.sequenceIndex, flags); roadEl != nullptr)
        {
            const auto companyId = SceneManager::isEditorMode() ? CompanyId::neutral : getUpdatingCompanyId();
            if (!sub_431E6A(companyId, reinterpret_cast<const World::TileElement*>(roadEl)))
            {
                return FAILURE;
            }

            /*
            // Remove check for is road in use when removing roads. It is
            // quite annoying when it's sometimes only the player's own
            // vehicles that are using it.
            // TODO: turn this into a setting?
            if (companyId != CompanyId::neutral && (roadEl->hasUnk7_40() || roadEl->hasUnk7_80()))
            {
                setErrorText(StringIds::empty);

                auto nearest = TownManager::getClosestTownAndDensity(args.pos);
                if (nearest.has_value())
                {
                    auto* town = TownManager::get(nearest->first);
                    FormatArguments::common(town->name);
                    setErrorText(StringIds::stringid_local_authority_wont_allow_removal_in_use);
                }
                return FAILURE;
            }
            */
        }

        currency32_t totalRemovalCost = 0;

        // 0x004776E3
        if (auto* roadEl = getRoadElement(tile, args, args.sequenceIndex, flags); roadEl != nullptr)
        {
            if (roadEl->hasStationElement())
            {
                // Count the number of road station users as we only want to
                // remove the road station if the target road element is the only
                // user of the road station.
                const auto numRoadStationUsers = [targetElRoad = roadEl, tile, &args, flags]() {
                    const auto baseZ = args.pos.z / World::kSmallZStep;
                    bool foundFirst = false;
                    size_t count = 0U;
                    for (auto& element : tile)
                    {
                        auto* elRoad = element.as<World::RoadElement>();
                        if (elRoad == nullptr)
                        {
                            if (foundFirst)
                            {
                                break;
                            }
                            continue;
                        }
                        if (elRoad->baseZ() != baseZ)
                        {
                            if (foundFirst)
                            {
                                break;
                            }
                            continue;
                        }
                        foundFirst = true;
                        if (elRoad->hasStationElement())
                        {
                            count++;
                        }
                    }
                    return count;
                }();

                if (numRoadStationUsers == 1)
                {
                    auto* elStation = tile.roadStation(roadEl->roadId(), roadEl->rotation(), roadEl->baseZ());
                    if (elStation != nullptr && !elStation->isGhost())
                    {
                        RoadStationRemovalArgs srArgs = {};
                        srArgs.pos = args.pos;
                        srArgs.rotation = args.rotation;
                        srArgs.roadId = args.roadId;
                        srArgs.index = args.sequenceIndex;
                        srArgs.roadObjectId = args.objectId;

                        auto stationRemovalRes = GameCommands::doCommand(srArgs, flags);
                        if (stationRemovalRes == FAILURE)
                        {
                            return FAILURE;
                        }

                        totalRemovalCost += stationRemovalRes;
                    }
                }
            }
        }

        // 0x004777EB
        if (auto* roadEl = getRoadElement(tile, args, args.sequenceIndex, flags); roadEl != nullptr)
        {
            bool removeRoadBridge = false; // 0x0112C2CD
            int8_t roadBridgeId = -1;   // 0x0112C2D0

            const auto& roadPieces = World::TrackData::getRoadPiece(roadEl->roadId());
            const auto& currentPart = roadPieces[roadEl->sequenceIndex()];
            const auto roadStart = args.pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ currentPart.x, currentPart.y }, args.rotation), currentPart.z };

            // NB: moved out of the loop below (was at 0x00477A10)
            const currency32_t pieceRemovalCost = roadRemoveCost(args, roadPieces[0], roadStart, flags);

            for (auto& piece : roadPieces)
            {
                const auto roadLoc = roadStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

                if (!(flags & Flags::aiAllocated))
                {
                    World::TileManager::mapInvalidateTileFull(roadLoc);
                }

                // 0x00477934
                auto pieceTile = World::TileManager::get(roadLoc.x, roadLoc.y);
                auto* roadElPiece = getRoadElement(pieceTile, args, piece.index, flags);
                if (roadElPiece == nullptr)
                {
                    continue;
                }

                // 0x004779B2
                if (roadElPiece->hasBridge())
                {
                    const auto numOverlappingElRoads = [targetElRoad = roadElPiece, pos = roadLoc, tile = pieceTile]() {
                        const auto baseZ = pos.z / World::kSmallZStep;
                        bool foundFirst = false;
                        size_t count = 0U;
                        for (auto& element : tile)
                        {
                            auto* elRoad = element.as<World::RoadElement>();
                            if (elRoad == nullptr)
                            {
                                if (foundFirst)
                                {
                                    break;
                                }
                                continue;
                            }
                            if (elRoad->baseZ() != baseZ)
                            {
                                if (foundFirst)
                                {
                                    break;
                                }
                                continue;
                            }
                            foundFirst = true;
                            count++;
                        }
                        return count;
                    }();
                    // Bridge only removed if this is the only road piece
                    removeRoadBridge = numOverlappingElRoads == 1;
                    roadBridgeId = roadElPiece->bridge();
                }

                if (!(flags & Flags::apply))
                {
                    continue;
                }

                World::TileManager::removeElement(*reinterpret_cast<World::TileElement*>(roadElPiece));
                Scenario::getOptions().madeAnyChanges = 1;
                World::TileManager::setLevelCrossingFlags(roadLoc);
            }

            // 0x00477A10
            totalRemovalCost += pieceRemovalCost;

            // Seems to have been forgotten in vanilla
            if (removeRoadBridge)
            {
                const auto* bridgeObj = ObjectManager::get<BridgeObject>(roadBridgeId);
                const auto bridgeBaseCost = Economy::getInflationAdjustedCost(bridgeObj->sellCostFactor, bridgeObj->costIndex, 10);
                totalRemovalCost += (bridgeBaseCost * World::TrackData::getRoadMiscData(args.roadId).costFactor) / 256;
            }
        }

        // 0x00477B39
        if (flags & Flags::apply)
        {
            if (!(flags & (Flags::aiAllocated | Flags::ghost)))
            {
                if (getUpdatingCompanyId() != CompanyId::neutral)
                {
                    playRoadRemovalSound(args.pos);
                }
            }
        }

        return totalRemovalCost;
    }

    void removeRoad(registers& regs)
    {
        regs.ebx = removeRoad(RoadRemovalArgs(regs), regs.bl);
    }
}
