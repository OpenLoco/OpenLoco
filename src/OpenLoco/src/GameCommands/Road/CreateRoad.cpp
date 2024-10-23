#include "CreateRoad.h"
// #include "Audio/Audio.h"
#include "Economy/Economy.h"
#include "GameState.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/BuildingElement.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "Objects/BridgeObject.h"
#include "Objects/LevelCrossingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/RoadStationObject.h"
#include "Objects/TrackObject.h"
#include "S5/S5.h"
#include "SceneManager.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco::GameCommands
{
    using namespace World::TileManager;
    using namespace World::Track;

    static loco_global<uint8_t, 0x0112C2E9> _alternateTrackObjectId; // used by ui
    static loco_global<ElementPositionFlags, 0x01136072> _byte_1136072;
    static loco_global<uint8_t, 0x01136073> _byte_1136073;
    static loco_global<World::MicroZ, 0x01136074> _byte_1136074;
    static loco_global<uint8_t, 0x01136075> _byte_1136075; // bridgeType of any overlapping track

    // TODO: Identical to createTrack function
    static bool isBridgeRequired(const World::SmallZ baseZ, const World::SurfaceElement& elSurface, const World::TrackData::PreviewTrack& piece, const uint8_t unk)
    {
        if (baseZ > elSurface.baseZ())
        {
            return true;
        }
        else if (baseZ == elSurface.baseZ())
        {
            if (!piece.hasFlags(World::TrackData::PreviewTrackFlags::unk4))
            {
                if (unk == 0 || unk == elSurface.slopeCorners())
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    struct ClearFunctionArgs
    {
        World::Pos3 pos;
        uint8_t rotation;
        uint8_t roadId;
        uint8_t bridgeId;
        uint8_t roadObjectId;
        uint8_t index;
        bool isLastIndex;
        uint8_t flags;
        uint8_t unkFlags;
    };

    struct RoadClearFunctionResult
    {
        explicit RoadClearFunctionResult(World::TileClearance::ClearFuncResult _res)
            : res(_res)
        {
        }

        World::TileClearance::ClearFuncResult res;
        bool hasLevelCrossing = false;
        uint8_t levelCrossingObjId = 0xFFU;
        bool hasStation = false;
        StationId stationId = StationId::null;
        std::array<uint8_t, 16> roadIdUnk = {};
    };

    // 0x00476EC4
    static RoadClearFunctionResult clearTrack(World::TrackElement& elTrack, const ClearFunctionArgs& args)
    {
        if (elTrack.hasBridge())
        {
            _byte_1136075 = elTrack.bridge();
            auto* bridgeObj = ObjectManager::get<BridgeObject>(elTrack.bridge());
            if ((bridgeObj->disabledTrackCfg & CommonTraitFlags::junction) != CommonTraitFlags::none)
            {
                setErrorText(StringIds::bridge_not_suitable_for_junction);
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
            }
        }

        auto* trackObj = ObjectManager::get<TrackObject>(elTrack.trackObjectId());
        auto* roadObj = ObjectManager::get<RoadObject>(args.roadObjectId);

        if (!(trackObj->compatibleRoads & (1U << args.roadObjectId))
            && !(roadObj->compatibleTracks & (1U << elTrack.trackObjectId())))
        {
            FormatArguments::common(trackObj->name);
            setErrorText(StringIds::unable_to_cross_or_create_junction_with_string);
            return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
        }

        if (elTrack.hasSignal())
        {
            setErrorText(StringIds::signal_in_the_way);
            return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
        }

        if (elTrack.hasStationElement())
        {
            setErrorText(StringIds::station_in_the_way);
            return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
        }

        if (elTrack.hasBridge())
        {
            if (elTrack.bridge() != args.bridgeId)
            {
                setErrorText(StringIds::bridge_types_must_match);
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
            }
        }

        if (elTrack.trackId() != 0
            || args.roadId != 0
            || !((elTrack.rotation() - args.rotation) & 0b1)
            || elTrack.baseHeight() != args.pos.z)
        {
            setErrorText(StringIds::level_crossing_only_possible_with_straight_road_and_track_at_same_level);
            return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
        }

        auto res = RoadClearFunctionResult(World::TileClearance::ClearFuncResult::noCollision);
        res.hasLevelCrossing = true;
        _byte_1136073 = _byte_1136073 | (1U << 2);

        if (!(args.flags & (Flags::aiAllocated | Flags::ghost))
            && (args.flags & Flags::apply))
        {
            elTrack.setHasLevelCrossing(true);
        }
        return res;
    }

    // Checks to see if this is an overlay road
    // such as a tram on top of a road
    static bool isPotentialOverlayRoad(World::RoadElement& elRoad, const ClearFunctionArgs& args)
    {
        if (elRoad.roadId() == args.roadId
            && elRoad.rotation() == args.rotation
            && elRoad.sequenceIndex() == args.index)
        {
            return true;
        }
        else if (
            World::TrackData::getRoadMiscData(elRoad.roadId()).reverseRoadId == args.roadId
            && (elRoad.rotation() ^ (1U << 1)) == args.rotation
            && (World::TrackData::getRoadMiscData(elRoad.roadId()).reverseLane - 1 - elRoad.sequenceIndex()) == args.index)
        {
            return true;
        }
        return false;
    }

    static World::StationElement* getRoadStationElement(const World::Pos3& pos)
    {
        auto tile = World::TileManager::get(pos.x, pos.y);
        const auto baseZ = pos.z / World::kSmallZStep;

        for (auto& element : tile)
        {
            auto* elStation = element.as<World::StationElement>();

            if (elStation == nullptr)
            {
                continue;
            }

            if (elStation->baseZ() != baseZ)
            {
                continue;
            }

            if (elStation->stationType() != StationType::roadStation)
            {
                continue;
            }

            return elStation;
        }
        return nullptr;
    }
    // 0x0047744C
    static bool roadTraitAndCompatbilityCheck(const World::RoadElement& elRoad, const ClearFunctionArgs& args, const RoadObject& targetRoadObj, const RoadObject& newRoadObj)
    {
        if (elRoad.roadObjectId() == args.roadObjectId)
        {
            if (!targetRoadObj.hasTraitFlags(RoadTraitFlags::junction))
            {
                setErrorText(StringIds::junctions_not_possible);
                return false;
            }
        }
        else
        {
            if (!(targetRoadObj.compatibleRoads & (1U << args.roadObjectId))
                && !(newRoadObj.compatibleRoads & (1U << elRoad.roadObjectId())))
            {
                FormatArguments::common(targetRoadObj.name);
                setErrorText(StringIds::unable_to_cross_or_create_junction_with_string);
                return false;
            }
        }

        return true;
    }

    // 0x004773FE
    static bool finalJunctionChecks(const World::RoadElement& elRoad, const ClearFunctionArgs& args, const RoadObject& targetRoadObj, const RoadObject& newRoadObj)
    {
        if (World::TrackData::getRoadPiece(args.roadId).size() > 1
            || World::TrackData::getRoadPiece(elRoad.roadId()).size() > 1)
        {
            setErrorText(StringIds::junction_not_possible);
            return false;
        }

        return roadTraitAndCompatbilityCheck(elRoad, args, targetRoadObj, newRoadObj);
    }

    static bool aiUnkCheck(const World::RoadElement& elRoad, const ClearFunctionArgs& args)
    {
        if (!(args.unkFlags & ((1U << 4) | (1U << 5))))
        {
            return false;
        }
        if (!(args.flags & Flags::aiAllocated))
        {
            return false;
        }
        if (elRoad.isAiAllocated())
        {
            return false;
        }
        return ((args.unkFlags & (1U << 4)) || elRoad.owner() == getUpdatingCompanyId());
    }

    // 0x00476FAB
    static RoadClearFunctionResult clearRoad(World::RoadElement& elRoad, const ClearFunctionArgs& args)
    {
        if (!elRoad.isGhost() && !elRoad.isAiAllocated())
        {
            _byte_1136073 = _byte_1136073 | (1U << 6);
        }

        if (elRoad.hasBridge())
        {
            _byte_1136075 = elRoad.bridge();
        }

        const auto& targetPiece = World::TrackData::getRoadPiece(elRoad.roadId())[elRoad.sequenceIndex()];
        const auto& newPiece = World::TrackData::getRoadPiece(args.roadId)[args.index];

        const auto targetConnectFlags = targetPiece.connectFlags[elRoad.rotation()];
        const auto newConnectFlags = newPiece.connectFlags[args.rotation];
        if (!(targetConnectFlags & newConnectFlags))
        {
            return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::noCollision);
        }

        // RoadObject of the road element we are clearing
        const auto& targetRoadObj = ObjectManager::get<RoadObject>(elRoad.roadObjectId());
        // RoadObject of the road we are placing as part of createRoad
        const auto& newRoadObj = ObjectManager::get<RoadObject>(args.roadObjectId);

        if ((args.flags & Flags::aiAllocated) && (args.unkFlags & (1U << 4)) && targetRoadObj->hasFlags(RoadObjectFlags::unk_00))
        {
            setErrorText(StringIds::junctions_not_possible);
            return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
        }

        const auto& gs = getGameState();
        if (!(gs.roadObjectIdIsFlag7 & (1U << elRoad.roadObjectId()))
            || !(gs.roadObjectIdIsFlag7 & (1U << args.roadObjectId)))
        {
            if (!sub_431E6A(elRoad.owner(), reinterpret_cast<const World::TileElement*>(&elRoad)))
            {
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
            }
        }

        bool confirmedOverlay = false;
        const bool aiUnk = aiUnkCheck(elRoad, args);
        if (elRoad.baseHeight() == args.pos.z)
        {
            if ((World::TrackData::getRoadMiscData(elRoad.roadId()).flags & (CommonTraitFlags::slope | CommonTraitFlags::steepSlope)) != CommonTraitFlags::none)
            {
                if (isPotentialOverlayRoad(elRoad, args))
                {
                    _byte_1136073 = _byte_1136073 | (1U << 5);
                    if (elRoad.roadObjectId() != args.roadObjectId)
                    {
                        confirmedOverlay = true;
                    }
                    if (aiUnk)
                    {
                        confirmedOverlay = true;
                    }
                }
            }
        }

        if (!confirmedOverlay)
        {
            // 0x477130
            if (((World::TrackData::getRoadMiscData(elRoad.roadId()).flags & (CommonTraitFlags::slope | CommonTraitFlags::steepSlope)) != CommonTraitFlags::none)
                || ((World::TrackData::getRoadMiscData(args.roadId).flags & (CommonTraitFlags::slope | CommonTraitFlags::steepSlope)) != CommonTraitFlags::none))
            {
                setErrorText(StringIds::junction_must_be_entirely_level);
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
            }

            if (elRoad.baseHeight() != args.pos.z)
            {
                FormatArguments::common(targetRoadObj->name);
                setErrorText(StringIds::string_id_in_the_way_wrong_height_for_junction);
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
            }
        }

        auto res = RoadClearFunctionResult(World::TileClearance::ClearFuncResult::noCollision);
        if (elRoad.hasLevelCrossing())
        {
            res.levelCrossingObjId = elRoad.levelCrossingObjectId();
        }

        _alternateTrackObjectId = elRoad.roadObjectId();

        if (elRoad.hasSignalElement())
        {
            setErrorText(StringIds::signal_in_the_way);
            return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
        }

        if (elRoad.hasStationElement())
        {
            if (args.roadId != 0
                || (args.rotation != elRoad.rotation() && (args.rotation != (elRoad.rotation() ^ (1U << 1)))))
            {
                setErrorText(StringIds::station_in_the_way);
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
            }

            auto* elStation = getRoadStationElement(args.pos);
            if (elStation != nullptr)
            {
                res.stationId = elStation->stationId();
                // Check if either the road object is compatible with the station object
                // or if the station object is compatible with the road object
                for (const auto compatStationObjId : newRoadObj->stations)
                {
                    if (compatStationObjId == elStation->objectId())
                    {
                        res.hasStation = true;
                        break;
                    }
                }
                if (!res.hasStation)
                {
                    auto* roadStationObj = ObjectManager::get<RoadStationObject>(elStation->objectId());
                    for (const auto compatRoadObjId : roadStationObj->mods)
                    {
                        if (compatRoadObjId == args.roadObjectId)
                        {
                            res.hasStation = true;
                            break;
                        }
                    }
                }
                if (!res.hasStation)
                {
                    setErrorText(StringIds::station_in_the_way);
                    return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
                }
            }
        }

        // Exact overlays are allowed only if objectIds are different
        // NOTE: Doesn't take into account sequence index
        const bool isExactOverlay = args.roadId == elRoad.roadId()
            && args.rotation == elRoad.rotation()
            && args.index == elRoad.sequenceIndex();
        // A reverse overlay is ultimately the same as an exact overlay
        // NOTE: Doesn't take into account sequence index
        const bool isReverseExactOverlay = !isExactOverlay
            && (World::TrackData::getRoadMiscData(elRoad.roadId()).reverseRoadId == args.roadId)
            && (((World::TrackData::getRoadMiscData(elRoad.roadId()).reverseRotation + elRoad.rotation()) & 0x3) == args.rotation)
            && (World::TrackData::getRoadMiscData(elRoad.roadId()).reverseLane == World::TrackData::getRoadMiscData(args.roadId).reverseLane);

        // Ends can connect to starts or middles can connect if exactly the same (tram/road curve overlay)
        // NOTE: Doesn't take into account size 1 pieces that have additional allowances (tram tight curve overlay on straight road)
        const bool isConnectableSequence = (elRoad.sequenceIndex() == 0 && args.isLastIndex)               // is Start Road && connect is End
            || (elRoad.sequenceIndex() != 0 && elRoad.isFlag6() && args.index == 0)                        // is End Road && connect is Start
            || (elRoad.sequenceIndex() != 0 && !elRoad.isFlag6() && elRoad.sequenceIndex() == args.index); // is Middle Road && connect same id

        if (elRoad.hasBridge())
        {
            if (elRoad.bridge() != args.bridgeId)
            {
                setErrorText(StringIds::bridge_types_must_match);
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
            }

            auto* bridgeObj = ObjectManager::get<BridgeObject>(args.bridgeId);
            if ((bridgeObj->disabledTrackCfg & CommonTraitFlags::junction) != CommonTraitFlags::none)
            {
                if (args.roadObjectId == elRoad.roadObjectId())
                {
                    setErrorText(StringIds::bridge_not_suitable_for_junction);
                    return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
                }
                if (!isExactOverlay)
                {
                    if (!isReverseExactOverlay || !isConnectableSequence)
                    {
                        setErrorText(StringIds::bridge_not_suitable_for_junction);
                        return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
                    }
                }
            }
        }
        // 0x004772F4

        if (isExactOverlay)
        {
            _byte_1136073 = _byte_1136073 | (1U << 5);
            if (elRoad.roadObjectId() == args.roadObjectId)
            {
                // 0x004773BC
                if (aiUnk)
                {
                    // 0x004773FE
                    if (!finalJunctionChecks(elRoad, args, *targetRoadObj, *newRoadObj))
                    {
                        return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
                    }
                    res.roadIdUnk[elRoad.roadId()] |= 1U << elRoad.rotation();
                    res.roadIdUnk[0] |= 1U << 7;

                    return res;
                }

                setErrorText(StringIds::already_built_here);
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
            }
            // 0x0047744C

            if (!roadTraitAndCompatbilityCheck(elRoad, args, *targetRoadObj, *newRoadObj))
            {
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
            }

            return res;
        }
        // 0x00477336
        if (isReverseExactOverlay)
        {
            if (!isConnectableSequence)
            {
                // 0x004773FE
                if (!finalJunctionChecks(elRoad, args, *targetRoadObj, *newRoadObj))
                {
                    return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
                }
                if (elRoad.roadObjectId() == args.roadObjectId)
                {
                    res.roadIdUnk[elRoad.roadId()] |= 1U << elRoad.rotation();
                    res.roadIdUnk[0] |= 1U << 7;
                }
                return res;
            }
            // 0x004773A5
            _byte_1136073 = _byte_1136073 | (1U << 5);
            if (elRoad.roadObjectId() == args.roadObjectId)
            {
                // 0x004773BC
                if (aiUnk)
                {
                    // 0x004773FE
                    if (!finalJunctionChecks(elRoad, args, *targetRoadObj, *newRoadObj))
                    {
                        return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
                    }

                    res.roadIdUnk[elRoad.roadId()] |= 1U << elRoad.rotation();
                    res.roadIdUnk[0] |= 1U << 7;

                    return res;
                }

                setErrorText(StringIds::already_built_here);
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
            }
            // 0x0047744C
            if (!roadTraitAndCompatbilityCheck(elRoad, args, *targetRoadObj, *newRoadObj))
            {
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
            }

            return res;
        }

        // 0x004773FE
        if (!finalJunctionChecks(elRoad, args, *targetRoadObj, *newRoadObj))
        {
            return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collisionErrorSet);
        }

        if (elRoad.roadObjectId() == args.roadObjectId)
        {
            res.roadIdUnk[elRoad.roadId()] |= 1U << elRoad.rotation();
            res.roadIdUnk[0] |= 1U << 7;
        }

        return res;
    }

    // 0x00476D40
    static RoadClearFunctionResult clearFunction(World::TileElement& el, currency32_t& totalCost, std::set<World::Pos3, World::LessThanPos3>& removedBuildings, const ClearFunctionArgs& args)
    {
        // stack
        // 0x0 = totalCost
        // 0xC = roadObjectId
        // 0xD = roadId
        // 0xE = bridge
        // 0x14 = flags
        // 0x15 = rotation
        // 0x0112C2E2 = hasLevelCrossing
        // 0x0112C2E3 = levelCrossingObjId
        // 0x0112C2EC = hasStation

        switch (el.type())
        {
            case World::ElementType::track:
            {
                auto* elTrack = el.as<World::TrackElement>();
                if (elTrack != nullptr)
                {
                    return clearTrack(*elTrack, args);
                }
                break;
            }
            case World::ElementType::station:
            {
                auto* elStation = el.as<World::StationElement>();
                if (elStation->stationType() == StationType::roadStation)
                {
                    return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::noCollision);
                }
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collision);
            }
            case World::ElementType::signal:
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::noCollision);
            case World::ElementType::building:
            {
                auto* elBuilding = el.as<World::BuildingElement>();
                if (elBuilding == nullptr)
                {
                    return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::noCollision);
                }
                _byte_1136073 = _byte_1136073 | (1U << 4);
                return RoadClearFunctionResult(World::TileClearance::clearBuildingCollision(*elBuilding, args.pos, removedBuildings, args.flags, totalCost));
            }
            case World::ElementType::tree:
            {
                auto* elTree = el.as<World::TreeElement>();
                if (elTree == nullptr)
                {
                    return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::noCollision);
                }
                return RoadClearFunctionResult(World::TileClearance::clearTreeCollision(*elTree, args.pos, args.flags, totalCost));
            }
            case World::ElementType::road:
            {
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad != nullptr)
                {
                    return clearRoad(*elRoad, args);
                }
                break;
            }
            case World::ElementType::surface:
            case World::ElementType::wall:
            case World::ElementType::industry:
                return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collision);
        }
        return RoadClearFunctionResult(World::TileClearance::ClearFuncResult::collision);
    }

    // 0x00475FBC
    static uint32_t createRoad(const RoadPlacementArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3{ 16, 16, 0 });
        _byte_1136072 = ElementPositionFlags::none;
        _byte_1136073 = 0; // Bridge related
        _byte_1136074 = 0;
        _byte_1136075 = 0xFFU;
        _alternateTrackObjectId = 0xFFU;

        const auto companyId = isEditorMode() ? CompanyId::neutral : getUpdatingCompanyId();

        if ((flags & Flags::apply) && !(flags & Flags::aiAllocated) && companyId != CompanyId::neutral)
        {
            companySetObservation(getUpdatingCompanyId(), ObservationStatus::buildingTrackRoad, args.pos, EntityId::null, args.roadObjectId | (1U << 7));
        }

        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        auto* roadObj = ObjectManager::get<RoadObject>(args.roadObjectId);
        const auto compatFlags = World::TrackData::getRoadMiscData(args.roadId).compatibleFlags;
        uint8_t validMods = args.mods;

        for (auto i = 0U; i < 2; ++i)
        {
            if (args.mods & (1U << i))
            {
                auto* extraObj = ObjectManager::get<RoadExtraObject>(roadObj->mods[i]);
                if ((compatFlags & extraObj->roadPieces) != compatFlags)
                {
                    validMods &= ~(1U << i);
                }
            }
        }

        if (args.pos.z & 0xF)
        {
            return FAILURE;
        }

        currency32_t totalCost = 0;

        {
            const auto roadBaseCost = Economy::getInflationAdjustedCost(roadObj->buildCostFactor, roadObj->costIndex, 10);
            const auto cost = (roadBaseCost * World::TrackData::getRoadMiscData(args.roadId).costFactor) / 256;
            totalCost += cost;
        }
        for (auto i = 0U; i < 2; ++i)
        {
            if (validMods & (1U << i))
            {
                auto* extraObj = ObjectManager::get<RoadExtraObject>(roadObj->mods[i]);
                const auto trackExtraBaseCost = Economy::getInflationAdjustedCost(extraObj->buildCostFactor, extraObj->costIndex, 10);
                const auto cost = (trackExtraBaseCost * World::TrackData::getRoadMiscData(args.roadId).costFactor) / 256;
                totalCost += cost;
            }
        }

        auto& roadPieces = World::TrackData::getRoadPiece(args.roadId);
        std::set<World::Pos3, World::LessThanPos3> removedBuildings;
        // 0x0113C2EC
        bool hasStation = false;
        // 0x0112C2EA
        StationId stationId = StationId::null;

        std::array<uint8_t, 16> roadIdUnk = {};
        roadIdUnk[args.roadId] |= 1U << args.rotation;

        for (auto& piece : roadPieces)
        {
            const auto roadLoc = args.pos + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };
            const auto quarterTile = piece.subTileClearance.rotate(args.rotation);
            const auto unk = Numerics::rotl4bit(enumValue(piece.flags) & 0xF, args.rotation);

            if (roadLoc.z < 16)
            {
                setErrorText(StringIds::error_too_low);
                return FAILURE;
            }

            const auto baseZ = roadLoc.z / World::kSmallZStep;
            auto clearZ = baseZ + (piece.clearZ + 32) / World::kSmallZStep;
            _byte_1136073 = _byte_1136073 & ~(1U << 1);

            // Why aren't we just failing invalid???
            if (World::validCoords(roadLoc))
            {
                const auto tile = World::TileManager::get(roadLoc);
                auto* elSurface = tile.surface();
                if (elSurface->water())
                {
                    _byte_1136073 = _byte_1136073 | (1U << 7);
                }

                const bool requiresBridge = isBridgeRequired(baseZ, *elSurface, piece, unk);
                if (requiresBridge)
                {
                    // 0x004762CD
                    _byte_1136073 = _byte_1136073 | (1U << 1) | (1U << 0);
                    World::MicroZ heightDiff = (baseZ - elSurface->baseZ()) / World::kMicroToSmallZStep;
                    if (args.bridge == 0xFFU)
                    {
                        setErrorText(StringIds::bridge_needed);
                        return FAILURE;
                    }
                    auto* bridgeObj = ObjectManager::get<BridgeObject>(args.bridge);
                    if (heightDiff > bridgeObj->maxHeight)
                    {
                        setErrorText(StringIds::too_far_above_ground_for_bridge_type);
                        return FAILURE;
                    }
                    _byte_1136074 = std::max(heightDiff, *_byte_1136074);
                    if ((bridgeObj->disabledTrackCfg & World::TrackData::getRoadMiscData(args.roadId).flags) != CommonTraitFlags::none)
                    {
                        setErrorText(StringIds::bridge_type_unsuitable_for_this_configuration);
                        return FAILURE;
                    }
                    clearZ += bridgeObj->clearHeight / World::kSmallZStep;
                }
            }

            // 0x0047632B
            if (clearZ > 236)
            {
                setErrorText(StringIds::error_too_high);
                return FAILURE;
            }

            // 0x0113C2E2
            bool hasLevelCrossing = false;
            // 0x0113C2E3
            uint8_t levelCrossingObjId = 0xFFU;

            ClearFunctionArgs clearArgs{};
            clearArgs.pos = roadLoc;
            clearArgs.rotation = args.rotation;
            clearArgs.roadId = args.roadId;
            clearArgs.bridgeId = args.bridge;
            clearArgs.roadObjectId = args.roadObjectId;
            clearArgs.index = piece.index;
            clearArgs.isLastIndex = piece.index == (roadPieces.size() - 1);
            clearArgs.flags = flags;
            clearArgs.unkFlags = args.unkFlags;

            auto clearFunc = [&totalCost, &hasLevelCrossing, &hasStation, &removedBuildings, &levelCrossingObjId, &stationId, &roadIdUnk, &clearArgs](World::TileElement& el) {
                const auto res = clearFunction(el, totalCost, removedBuildings, clearArgs);
                hasLevelCrossing |= res.hasLevelCrossing;
                hasStation |= res.hasStation;
                if (res.levelCrossingObjId != 0xFFU)
                {
                    levelCrossingObjId = res.levelCrossingObjId;
                }
                if (res.stationId != StationId::null)
                {
                    stationId = res.stationId;
                }
                for (auto i = 0U; i < std::size(roadIdUnk); ++i)
                {
                    roadIdUnk[i] |= res.roadIdUnk[i];
                }
                return res.res;
            };

            if (!World::TileClearance::applyClearAtStandardHeight(roadLoc, baseZ, clearZ, quarterTile, clearFunc))
            {
                return FAILURE;
            }

            if (levelCrossingObjId == 0xFFU)
            {
                levelCrossingObjId = getGameState().currentDefaultLevelCrossingType;
            }
            if (hasLevelCrossing)
            {
                auto* levelCrossObj = ObjectManager::get<LevelCrossingObject>(levelCrossingObjId);
                totalCost += Economy::getInflationAdjustedCost(levelCrossObj->costFactor, levelCrossObj->costIndex, 10);
            }

            if ((flags & Flags::apply) && !(flags & (Flags::ghost | Flags::aiAllocated)))
            {
                World::TileManager::removeAllWallsOnTileBelow(World::toTileSpace(roadLoc), baseZ);
            }

            // 0x00476408
            const auto posFlags = World::TileClearance::getPositionFlags();

            // Abridged flags for just above/underground
            const auto newGroundFlags = posFlags & (ElementPositionFlags::aboveGround | ElementPositionFlags::underground);
            if (_byte_1136072 != ElementPositionFlags::none && (*_byte_1136072 & newGroundFlags) == ElementPositionFlags::none)
            {
                setErrorText(StringIds::cant_build_partly_above_partly_below_ground);
                return FAILURE;
            }
            _byte_1136072 = newGroundFlags;

            if ((posFlags & ElementPositionFlags::partiallyUnderwater) != ElementPositionFlags::none)
            {
                setErrorText(StringIds::cant_build_this_underwater);
                return FAILURE;
            }
            if ((posFlags & ElementPositionFlags::underwater) != ElementPositionFlags::none)
            {
                setErrorText(StringIds::too_close_to_water_surface);
                return FAILURE;
            }

            if (!(flags & Flags::apply))
            {
                continue;
            }

            if (CompanyManager::isPlayerCompany(companyId))
            {
                companyEmotionEvent(getUpdatingCompanyId(), Emotion::thinking);
            }
            if (!(flags & (Flags::ghost | Flags::aiAllocated)))
            {
                World::TileManager::removeSurfaceIndustryAtHeight(roadLoc);
                World::TileManager::setTerrainStyleAsClearedAtHeight(roadLoc);
            }

            auto* newElRoad = World::TileManager::insertElementRoad(roadLoc, baseZ, quarterTile.getBaseQuarterOccupied());
            if (newElRoad == nullptr)
            {
                return FAILURE;
            }
            newElRoad->setClearZ(clearZ);
            newElRoad->setRotation(args.rotation);
            newElRoad->setRoadObjectId(args.roadObjectId);
            newElRoad->setSequenceIndex(piece.index);
            newElRoad->setRoadId(args.roadId);
            newElRoad->setOwner(companyId);
            for (auto i = 0U; i < 2; ++i)
            {
                if (validMods & (1U << i))
                {
                    newElRoad->setMod(i, true);
                }
            }
            if ((getGameState().roadObjectIdIsNotTram & (1U << args.roadObjectId)) && companyId != CompanyId::neutral)
            {
                newElRoad->setUnk7_40(true);
            }
            newElRoad->setBridgeObjectId(args.bridge);
            newElRoad->setHasBridge(_byte_1136073 & (1U << 1));
            if (hasLevelCrossing && !(flags & Flags::aiAllocated))
            {
                newElRoad->setHasLevelCrossing(true);
                newElRoad->setLevelCrossingObjectId(levelCrossingObjId);
            }
            else
            {
                newElRoad->setHasLevelCrossing(false);
                newElRoad->setLevelCrossingObjectId(0);
            }
            newElRoad->setHasStationElement(hasStation);
            newElRoad->setFlag6(piece.index == (roadPieces.size() - 1));
            newElRoad->setGhost(flags & Flags::ghost);
            newElRoad->setAiAllocated(flags & Flags::aiAllocated);
            if (!(flags & Flags::aiAllocated))
            {
                World::TileManager::mapInvalidateTileFull(roadLoc);
            }
        }

        // TODO: Verify if we remembered to do this for track
        auto& options = S5::getOptions();
        options.madeAnyChanges = 1;

        // 0x47656B TODO: special road code...
        if ((flags & Flags::apply)
            && !(flags & (Flags::aiAllocated | Flags::ghost))
            && (roadIdUnk[0] & (1U << 7))
            && roadObj->hasFlags(RoadObjectFlags::unk_02))
        {
            for (auto i = 0U; i < 4; ++i)
            {
                const auto rot0 = i;
                const auto rot1 = (i + 1) & 0x3;
                const auto rot2 = (i + 2) & 0x3;
                const auto rot3 = (i + 3) & 0x3;

                const auto rot0Flag = (1U << rot0);
                const auto rot1Flag = (1U << rot1);
                const auto rot2Flag = (1U << rot2);
                const auto rot3Flag = (1U << rot3);

                auto placeAddition = [&args, companyId, validMods, flags](uint8_t rotation, uint8_t roadId) {
                    World::TileManager::removeSurfaceIndustryAtHeight(args.pos);
                    World::TileManager::setTerrainStyleAsClearedAtHeight(args.pos);

                    auto* newElRoad = World::TileManager::insertElement<World::RoadElement>(args.pos, args.pos.z / World::kSmallZStep, 0xFU);
                    if (newElRoad == nullptr)
                    {
                        return;
                    }
                    newElRoad->setClearZ(args.pos.z / World::kSmallZStep + 8);
                    newElRoad->setRotation(rotation);
                    newElRoad->setRoadObjectId(args.roadObjectId);
                    newElRoad->setSequenceIndex(0);
                    newElRoad->setRoadId(roadId);
                    newElRoad->setOwner(companyId);
                    for (auto i = 0U; i < 2; ++i)
                    {
                        if (validMods & (1U << i))
                        {
                            newElRoad->setMod(i, true);
                        }
                    }
                    newElRoad->setBridgeObjectId(args.bridge);
                    newElRoad->setHasBridge(_byte_1136073 & (1U << 1));

                    newElRoad->setFlag6(true);
                    newElRoad->setGhost(flags & Flags::ghost);
                    newElRoad->setAiAllocated(flags & Flags::aiAllocated);
                };

                auto requiresAdditionalLeft = [&roadIdUnk, rot0Flag, rot1Flag, rot2Flag, rot3Flag]() {
                    if ((roadIdUnk[0] & rot0Flag)
                        && ((roadIdUnk[0] & rot3Flag)
                            || (roadIdUnk[0] & rot1Flag)
                            || (roadIdUnk[1] & rot1Flag)
                            || (roadIdUnk[2] & rot2Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[0] & rot2Flag) && (roadIdUnk[0] & rot3Flag))
                    {
                        return true;
                    }
                    if ((roadIdUnk[0] & rot3Flag) && ((roadIdUnk[2] & rot0Flag) || (roadIdUnk[1] & rot3Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[2] & rot0Flag) && ((roadIdUnk[1] & rot1Flag) || (roadIdUnk[2] & rot2Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[1] & rot3Flag) && ((roadIdUnk[1] & rot1Flag) || (roadIdUnk[2] & rot2Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[0] & rot1Flag) && (roadIdUnk[2] & rot0Flag))
                    {
                        return true;
                    }
                    return false;
                };
                if (requiresAdditionalLeft())
                {
                    if (!(roadIdUnk[2] & rot1Flag))
                    {
                        bool place = !(roadIdUnk[1] & rot0Flag);
                        roadIdUnk[1] |= rot0Flag;
                        if (place)
                        {
                            placeAddition(i, 1);
                        }
                    }
                }
                // 0x0047677A
                auto requiresAdditionalRight = [&roadIdUnk, rot0Flag, rot1Flag, rot2Flag, rot3Flag]() {
                    if ((roadIdUnk[0] & rot2Flag)
                        && ((roadIdUnk[2] & rot2Flag)
                            || (roadIdUnk[1] & rot1Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[1] & rot0Flag) && ((roadIdUnk[1] & rot2Flag) || (roadIdUnk[2] & rot3Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[2] & rot1Flag) && ((roadIdUnk[1] & rot2Flag) || (roadIdUnk[2] & rot3Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[0] & rot1Flag) && ((roadIdUnk[0] & rot2Flag) || (roadIdUnk[1] & rot3Flag)))
                    {
                        return true;
                    }
                    return false;
                };

                if (requiresAdditionalRight())
                {
                    if (!(roadIdUnk[1] & rot0Flag))
                    {
                        bool place = !(roadIdUnk[2] & rot1Flag);
                        roadIdUnk[1] |= rot1Flag;
                        if (place)
                        {
                            placeAddition(i + 1, 2);
                        }
                    }
                }
                // 0x004768F0
                auto requiresAdditionalStraight1 = [&roadIdUnk, rot0Flag, rot1Flag, rot2Flag, rot3Flag]() {
                    if ((roadIdUnk[1] & rot0Flag)
                        && ((roadIdUnk[1] & rot1Flag)
                            || (roadIdUnk[2] & rot2Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[2] & rot0Flag) && ((roadIdUnk[1] & rot1Flag) || (roadIdUnk[2] & rot2Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[1] & rot3Flag) && ((roadIdUnk[1] & rot1Flag) || (roadIdUnk[2] & rot2Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[1] & rot0Flag) && ((roadIdUnk[1] & rot2Flag) || (roadIdUnk[2] & rot3Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[2] & rot1Flag) && ((roadIdUnk[1] & rot2Flag) || (roadIdUnk[2] & rot3Flag)))
                    {
                        return true;
                    }
                    if ((roadIdUnk[2] & rot1Flag) && (roadIdUnk[1] & rot1Flag))
                    {
                        return true;
                    }
                    return false;
                };

                if (requiresAdditionalStraight1())
                {
                    if (!(roadIdUnk[0] & rot2Flag))
                    {
                        bool place = !(roadIdUnk[0] & rot0Flag);
                        roadIdUnk[0] |= rot0Flag;
                        if (place)
                        {
                            placeAddition(i, 0);
                        }
                    }
                }

                // 0x00476AB2
                auto requiresAdditionalStraight2 = [&roadIdUnk, rot1Flag, rot2Flag]() {
                    if ((roadIdUnk[2] & rot1Flag)
                        && (roadIdUnk[2] & rot2Flag))
                    {
                        return true;
                    }
                    return false;
                };

                if (requiresAdditionalStraight2())
                {
                    if (!(roadIdUnk[0] & rot2Flag))
                    {
                        bool place = !(roadIdUnk[0] & rot0Flag);
                        roadIdUnk[0] |= rot0Flag;
                        if (place)
                        {
                            placeAddition(i, 0);
                        }
                    }
                }
            }
        }

        if ((_byte_1136073 & (1U << 0)) && !(_byte_1136073 & (1U << 6)))
        {
            auto* bridgeObj = ObjectManager::get<BridgeObject>(args.bridge);
            const auto heightCost = _byte_1136074 * bridgeObj->heightCostFactor;
            const auto bridgeBaseCost = Economy::getInflationAdjustedCost(bridgeObj->baseCostFactor + heightCost, bridgeObj->costIndex, 10);
            auto cost = (bridgeBaseCost * World::TrackData::getRoadMiscData(args.roadId).costFactor) / 256;
            if (_byte_1136073 & (1U << 7))
            {
                cost *= 2;
            }
            totalCost += cost;
        }

        if (((_byte_1136072 & ElementPositionFlags::underground) != ElementPositionFlags::none) && !(_byte_1136073 & (1U << 6)))
        {
            const auto tunnelBaseCost = Economy::getInflationAdjustedCost(roadObj->tunnelCostFactor, 2, 8);
            auto cost = (tunnelBaseCost * World::TrackData::getTrackMiscData(args.roadId).costFactor) / 256;

            totalCost += cost;
        }

        if ((flags & Flags::apply) && hasStation)
        {
            auto* station = StationManager::get(stationId);
            station->invalidate();
            recalculateStationModes(stationId);
            recalculateStationCenter(stationId);
            station->updateLabel();
            station->invalidate();
        }
        if ((flags & Flags::apply)
            && !(flags & (Flags::aiAllocated | Flags::ghost))
            && getUpdatingCompanyId() != CompanyId::neutral
            && totalCost != 0)
        {
            playConstructionPlacementSound(getPosition());
        }
        return totalCost;
    }

    void createRoad(registers& regs)
    {
        regs.ebx = createRoad(RoadPlacementArgs(regs), regs.bl);
    }
}
