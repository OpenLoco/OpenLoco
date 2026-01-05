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
#include "Scenario/ScenarioOptions.h"
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

    struct OverlapRoads
    {
        World::RoadElement* _begin = nullptr;
        World::RoadElement* _end = nullptr;

        OverlapRoads(World::Pos3 pos)
        {
            auto tile = World::TileManager::get(pos.x, pos.y);
            for (auto& el : tile)
            {
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    if (_begin != nullptr)
                    {
                        break;
                    }
                    continue;
                }
                if (elRoad->baseZ() != pos.z / World::kSmallZStep)
                {
                    if (_begin != nullptr)
                    {
                        break;
                    }
                    continue;
                }
                _end = elRoad + 1;
                if (_begin == nullptr)
                {
                    _begin = elRoad;
                }
            }
        }

        World::RoadElement* begin() const { return _begin; }
        World::RoadElement* end() const { return _end; }
    };

    static World::RoadElement* getRoadElement(const World::Pos3 pos, const RoadRemovalArgs& args, uint8_t sequenceIndex, uint8_t flags)
    {
        auto tile = World::TileManager::get(pos);
        const auto baseZ = pos.z / World::kSmallZStep;
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

        auto* roadElPiece = getRoadElement(roadLoc, args, roadPiece0.index, flags);
        if (roadElPiece == nullptr)
        {
            return 0;
        }

        currency32_t totalRemovalCost = 0;

        const auto* roadObj = ObjectManager::get<RoadObject>(roadElPiece->roadObjectId());
        if (roadElPiece->owner() != CompanyId::neutral)
        {
            const auto trackBaseCost = Economy::getInflationAdjustedCost(roadObj->sellCostFactor, roadObj->costIndex, 10);
            const auto cost = (trackBaseCost * World::TrackData::getRoadMiscData(args.roadId).costFactor) / 256;
            totalRemovalCost += cost;
        }

        // Check mod removal costs
        if (!roadObj->hasFlags(RoadObjectFlags::anyRoadTypeCompatible))
        {
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
        }
        return totalRemovalCost;
    }

    // 0x004775A5
    static uint32_t removeRoad(const RoadRemovalArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3{ 16, 16, 0 });

        // 0x0047762D
        auto* roadEl = getRoadElement(args.pos, args, args.sequenceIndex, flags);
        if (roadEl == nullptr)
        {
            return FAILURE;
        }

        const CompanyId roadOwner = roadEl->owner();

        if (!sub_431E6A(roadOwner, reinterpret_cast<const World::TileElement*>(roadEl)))
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

        currency32_t totalRemovalCost = 0;

        // 0x004776E3

        if (roadEl->hasStationElement())
        {
            // We only want to remove the road station if the target road element is the only
            // user of the road station.
            const auto overlaps = OverlapRoads(args.pos);
            const auto hasOtherRoadStationUsers = std::ranges::any_of(overlaps, [roadEl](const World::RoadElement& el) {
                if (&el == roadEl)
                {
                    return false;
                }
                return el.hasStationElement();
            });

            if (!hasOtherRoadStationUsers)
            {
                auto tile = World::TileManager::get(args.pos);
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
        // Road element pointer may be invalid after road station removal.
        roadEl = nullptr;

        // 0x004777EB
        bool removeRoadBridge = false; // 0x0112C2CD
        int8_t roadBridgeId = -1;      // 0x0112C2D0

        const auto& roadPieces = World::TrackData::getRoadPiece(args.roadId);
        const auto& currentPart = roadPieces[args.sequenceIndex];
        const auto roadStart = args.pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ currentPart.x, currentPart.y }, args.rotation), currentPart.z };

        // NB: moved out of the loop below (was at 0x00477A10)
        const currency32_t pieceRemovalCost = roadRemoveCost(args, roadPieces[0], roadStart, flags);

        for (auto& piece : roadPieces)
        {
            const auto roadLoc = roadStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

            if (shouldInvalidateTile(flags))
            {
                World::TileManager::mapInvalidateTileFull(roadLoc);
            }

            // 0x00477934
            auto* roadElPiece = getRoadElement(roadLoc, args, piece.index, flags);
            if (roadElPiece == nullptr)
            {
                continue;
            }

            // 0x004779B2
            if (roadElPiece->hasBridge())
            {
                const auto overlaps = OverlapRoads(args.pos);
                const auto numOverlappingRoads = std::ranges::distance(overlaps);

                // Bridge only removed if this is the only road piece
                removeRoadBridge = numOverlappingRoads == 1;
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

        // Add on bridge refund if applicable (Absent from vanilla)
        if (removeRoadBridge && roadOwner != CompanyId::neutral)
        {
            const auto* bridgeObj = ObjectManager::get<BridgeObject>(roadBridgeId);
            const auto bridgeBaseCost = Economy::getInflationAdjustedCost(bridgeObj->sellCostFactor, bridgeObj->costIndex, 10);
            totalRemovalCost += (bridgeBaseCost * World::TrackData::getRoadMiscData(args.roadId).costFactor) / 256;
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
