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
#include "SceneManager.h"
// #include "Objects/LevelCrossingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/RoadStationObject.h"
#include "Objects/TrackObject.h"
// #include "World/CompanyManager.h"
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
        std::array<uint8_t, 16> roadIdUnk;
    };

    // 0x00476EC4
    static World::TileClearance::ClearFuncResult clearTrack(World::TrackElement& elTrack, const ClearFunctionArgs& args, bool& hasLevelCrossing)
    {
        if (elTrack.hasBridge())
        {
            _byte_1136075 = elTrack.bridge();
            auto* bridgeObj = ObjectManager::get<BridgeObject>(elTrack.bridge());
            if ((bridgeObj->disabledTrackCfg & CommonTraitFlags::junction) != CommonTraitFlags::none)
            {
                setErrorText(StringIds::bridge_not_suitable_for_junction);
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }
        }

        auto* trackObj = ObjectManager::get<TrackObject>(elTrack.trackObjectId());
        auto* roadObj = ObjectManager::get<RoadObject>(args.roadObjectId);

        if (!(trackObj->compatibleRoads & (1U << args.roadObjectId))
            && !(roadObj->compatibleTracks & (1U << elTrack.trackObjectId())))
        {
            FormatArguments::common(trackObj->name);
            setErrorText(StringIds::unable_to_cross_or_create_junction_with_string);
            return World::TileClearance::ClearFuncResult::collisionErrorSet;
        }

        if (elTrack.hasSignal())
        {
            setErrorText(StringIds::signal_in_the_way);
            return World::TileClearance::ClearFuncResult::collisionErrorSet;
        }

        if (elTrack.hasStationElement())
        {
            setErrorText(StringIds::station_in_the_way);
            return World::TileClearance::ClearFuncResult::collisionErrorSet;
        }

        if (elTrack.hasBridge())
        {
            if (elTrack.bridge() != args.bridgeId)
            {
                setErrorText(StringIds::bridge_types_must_match);
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }
        }

        if (elTrack.trackId() != 0
            || args.roadId != 0
            || !((elTrack.rotation() - args.rotation) & 0b1)
            || elTrack.baseHeight() != args.pos.z)
        {
            setErrorText(StringIds::level_crossing_only_possible_with_straight_road_and_track_at_same_level);
            return World::TileClearance::ClearFuncResult::collisionErrorSet;
        }

        hasLevelCrossing = true;
        _byte_1136073 = _byte_1136073 | (1U << 2);

        if (!(args.flags & (Flags::aiAllocated | Flags::ghost))
            && (args.flags & Flags::apply))
        {
            elTrack.setHasLevelCrossing(true);
        }
        return World::TileClearance::ClearFuncResult::noCollision;
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

    // 0x00476FAB
    static World::TileClearance::ClearFuncResult clearRoad(World::RoadElement& elRoad, const ClearFunctionArgs& args, uint8_t& levelCrossingObjId, bool& hasStation, StationId& stationId)
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
            return World::TileClearance::ClearFuncResult::noCollision;
        }

        // RoadObject of the road element we are clearing
        const auto& targetRoadObj = ObjectManager::get<RoadObject>(elRoad.roadObjectId());
        // RoadObject of the road we are placing as part of createRoad
        const auto& newRoadObj = ObjectManager::get<RoadObject>(args.roadObjectId);

        if ((args.flags & Flags::aiAllocated) && (args.unkFlags & (1U << 4)) && targetRoadObj->hasFlags(RoadObjectFlags::unk_00))
        {
            setErrorText(StringIds::junctions_not_possible);
            return World::TileClearance::ClearFuncResult::collisionErrorSet;
        }

        const auto& gs = getGameState();
        if (!(gs.roadObjectIdIsFlag7 & (1U << elRoad.roadObjectId()))
            || !(gs.roadObjectIdIsFlag7 & (1U << args.roadObjectId)))
        {
            if (!sub_431E6A(elRoad.owner(), reinterpret_cast<const World::TileElement*>(&elRoad)))
            {
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }
        }

        bool confirmedOverlay = false;
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
                    if (args.unkFlags & ((1U << 4) | (1U << 5))
                        && args.flags & Flags::aiAllocated
                        && !elRoad.isAiAllocated())
                    {
                        if ((args.unkFlags & (1U << 4))
                            || elRoad.owner() == getUpdatingCompanyId())
                        {
                            confirmedOverlay = true;
                        }
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
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }

            if (elRoad.baseHeight() != args.pos.z)
            {
                FormatArguments::common(targetRoadObj->name);
                setErrorText(StringIds::string_id_in_the_way_wrong_height_for_junction);
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }
        }

        if (elRoad.hasLevelCrossing())
        {
            levelCrossingObjId = elRoad.levelCrossingObjectId();
        }

        _alternateTrackObjectId = elRoad.roadObjectId();

        if (elRoad.hasSignalElement())
        {
            setErrorText(StringIds::signal_in_the_way);
            return World::TileClearance::ClearFuncResult::collisionErrorSet;
        }

        if (elRoad.hasStationElement())
        {
            if (args.roadId != 0
                || (args.rotation != elRoad.rotation() && (args.rotation != (elRoad.rotation() ^ (1U << 1)))))
            {
                setErrorText(StringIds::station_in_the_way);
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }

            auto* elStation = getRoadStationElement(args.pos);
            stationId = elStation->stationId();
            // Check if either the road object is compatible with the station object
            // or if the station object is compatible with the road object
            for (const auto compatStationObjId : newRoadObj->stations)
            {
                if (compatStationObjId == elStation->objectId())
                {
                    hasStation = true;
                    break;
                }
            }
            if (!hasStation)
            {
                auto* roadStationObj = ObjectManager::get<RoadStationObject>(elStation->objectId());
                for (const auto compatRoadObjId : roadStationObj->mods)
                {
                    if (compatRoadObjId == args.roadObjectId)
                    {
                        hasStation = true;
                        break;
                    }
                }
            }
            if (!hasStation)
            {
                setErrorText(StringIds::station_in_the_way);
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }
        }

        if (elRoad.hasBridge())
        {
            if (elRoad.bridge() != args.bridgeId)
            {
                setErrorText(StringIds::bridge_types_must_match);
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }

            auto* bridgeObj = ObjectManager::get<BridgeObject>(args.bridgeId);
            if ((bridgeObj->disabledTrackCfg & CommonTraitFlags::junction) != CommonTraitFlags::none)
            {
                if (args.roadObjectId == elRoad.roadObjectId())
                {
                    setErrorText(StringIds::bridge_not_suitable_for_junction);
                    return World::TileClearance::ClearFuncResult::collisionErrorSet;
                }
                if (args.roadId != elRoad.roadId()
                    || args.rotation != elRoad.rotation()
                    || args.index != elRoad.sequenceIndex())
                {
                    // This is working out reversed elements
                    if ((World::TrackData::getRoadMiscData(elRoad.roadId()).reverseRoadId != args.roadId)
                        || (((World::TrackData::getRoadMiscData(elRoad.roadId()).reverseRotation + elRoad.rotation()) & 0x3) != args.rotation)
                        || (World::TrackData::getRoadMiscData(elRoad.roadId()).reverseLane != World::TrackData::getRoadMiscData(args.roadId).reverseLane))
                    {
                        setErrorText(StringIds::bridge_not_suitable_for_junction);
                        return World::TileClearance::ClearFuncResult::collisionErrorSet;
                    }
                    if (args.isLastIndex && elRoad.sequenceIndex() != 0)
                    {
                        setErrorText(StringIds::bridge_not_suitable_for_junction);
                        return World::TileClearance::ClearFuncResult::collisionErrorSet;
                    }
                    if ((elRoad.isFlag6() && args.index != 0) || elRoad.sequenceIndex() != args.index)
                    {
                        setErrorText(StringIds::bridge_not_suitable_for_junction);
                        return World::TileClearance::ClearFuncResult::collisionErrorSet;
                    }
                }
            }
        }
        // 0x004772F4

        if (elRoad.roadId() == args.roadId)
        {
            if (elRoad.rotation() == args.rotation)
            {
                if (elRoad.sequenceIndex() == args.index)
                {
                    _byte_1136073 = _byte_1136073 | (1U << 5);
                    if (elRoad.roadObjectId() == args.roadObjectId)
                    {
                        // 0x004773BC
                        if (args.unkFlags & ((1U << 5) | (1U << 4)))
                        {
                            if ((args.flags & Flags::aiAllocated)
                                && !elRoad.isAiAllocated())
                            {
                                if ((args.unkFlags & (1U << 4))
                                    || elRoad.owner() == getUpdatingCompanyId())
                                {
                                    // 0x004773FE
                                }
                            }
                        }
                        setErrorText(StringIds::already_built_here);
                        return World::TileClearance::ClearFuncResult::collisionErrorSet;
                    }
                    // 0x0047744C
                }
            }
        }
        // 0x00477336
        if (World::TrackData::getRoadMiscData(elRoad.roadId()).reverseRoadId == args.roadId)
        {
            if (((World::TrackData::getRoadMiscData(elRoad.roadId()).reverseRotation + elRoad.rotation()) & 0x3) == args.rotation)
            {
                if (World::TrackData::getRoadMiscData(elRoad.roadId()).reverseLane == World::TrackData::getRoadMiscData(args.roadId).reverseLane)
                {
                    if (elRoad.sequenceIndex() == 0)
                    {
                        if (!args.isLastIndex)
                        {
                            // 0x004773FE
                        }
                    }
                    else
                    {
                        if (elRoad.isFlag6())
                        {
                            if (args.index != 0)
                            {
                                // 0x004773FE
                            }
                        }
                        else
                        {
                            if (args.index != elRoad.sequenceIndex())
                            {
                                // 0x004773FE
                            }
                        }
                    }

                    // 0x004773A5
                    _byte_1136073 = _byte_1136073 | (1U << 5);
                    if (elRoad.roadObjectId() == args.roadObjectId)
                    {
                        // 0x004773BC
                    }
                    // 0x0047744C
                }
            }
        }
        // 0x004773FE
        if (World::TrackData::getRoadPiece(args.roadId).size() > 1
            || World::TrackData::getRoadPiece(elRoad.roadId()).size() > 1)
        {
            setErrorText(StringIds::junction_not_possible);
            return World::TileClearance::ClearFuncResult::collisionErrorSet;
        }

        if (elRoad.roadObjectId() == args.roadObjectId)
        {
            args.roadIdUnk[elRoad.roadId()] |= 1U << elRoad.rotation();
            args.roadIdUnk[elRoad.roadId()] |= 1U << 7;
        }
        // 0x0047744C

        if (elRoad.roadObjectId() == args.roadObjectId)
        {
            if (!targetRoadObj->hasTraitFlags(RoadTraitFlags::junction))
            {
                setErrorText(StringIds::junctions_not_possible);
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }
        }
        else
        {
            if (!(targetRoadObj->compatibleRoads & (1U << args.roadObjectId))
                && !(newRoadObj->compatibleRoads & (1U << elRoad.roadObjectId())))
            {
                FormatArguments::common(targetRoadObj->name);
                setErrorText(StringIds::unable_to_cross_or_create_junction_with_string);
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }
        }

        return World::TileClearance::ClearFuncResult::noCollision;
    }

    // 0x00476D40
    static World::TileClearance::ClearFuncResult clearFunction(World::TileElement& el, currency32_t& totalCost, bool& hasLevelCrossing, bool& hasStation, std::set<World::Pos3, World::LessThanPos3>& removedBuildings, uint8_t& levelCrossingObjId, StationId& stationId, const ClearFunctionArgs& args)
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
                    return clearTrack(*elTrack, args, hasLevelCrossing);
                }
                break;
            }
            case World::ElementType::station:
            {
                auto* elStation = el.as<World::StationElement>();
                if (elStation->stationType() == StationType::roadStation)
                {
                    return World::TileClearance::ClearFuncResult::noCollision;
                }
                return World::TileClearance::ClearFuncResult::collision;
            }
            case World::ElementType::signal:
                return World::TileClearance::ClearFuncResult::noCollision;
            case World::ElementType::building:
            {
                auto* elBuilding = el.as<World::BuildingElement>();
                if (elBuilding == nullptr)
                {
                    return World::TileClearance::ClearFuncResult::noCollision;
                }
                _byte_1136073 = _byte_1136073 | (1U << 4);
                return World::TileClearance::clearBuildingCollision(*elBuilding, args.pos, removedBuildings, args.flags, totalCost);
            }
            case World::ElementType::tree:
            {
                auto* elTree = el.as<World::TreeElement>();
                if (elTree == nullptr)
                {
                    return World::TileClearance::ClearFuncResult::noCollision;
                }
                return World::TileClearance::clearTreeCollision(*elTree, args.pos, args.flags, totalCost);
            }
            case World::ElementType::road:
            {
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad != nullptr)
                {
                    return clearRoad(*elRoad, args, levelCrossingObjId, hasStation, stationId);
                }
                break;
            }
            case World::ElementType::surface:
            case World::ElementType::wall:
            case World::ElementType::industry:
                return World::TileClearance::ClearFuncResult::collision;
        }
        return World::TileClearance::ClearFuncResult::collision;
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
            // 0x0113C2EC
            bool hasStation = false;
            // 0x0112C2EA
            StationId stationId = StationId::null;

            ClearFunctionArgs clearArgs{};
            clearArgs.pos = roadLoc;
            clearArgs.rotation = args.rotation;
            clearArgs.roadId = args.roadId;
            clearArgs.bridgeId = args.bridge;
            clearArgs.roadObjectId = args.roadObjectId;
            clearArgs.index = piece.index;
            clearArgs.isLastIndex = piece.index == (roadPieces.size() - 1);
            clearArgs.flags = flags;
            clearArgs.roadIdUnk[args.roadId] |= 1U << args.rotation;
            clearArgs.unkFlags = args.unkFlags;

            auto clearFunc = [&totalCost, &hasLevelCrossing, &hasStation, &removedBuildings, &levelCrossingObjId, &stationId, &clearArgs](World::TileElement& el) {
                return clearFunction(el, totalCost, hasLevelCrossing, hasStation, removedBuildings, levelCrossingObjId, stationId, clearArgs);
            };

            if (!World::TileClearance::applyClearAtStandardHeight(roadLoc, baseZ, clearZ, quarterTile, clearFunc))
            {
                return FAILURE;
            }

            //        if (hasLevelCrossing)
            //        {
            //            auto* levelCrossObj = ObjectManager::get<LevelCrossingObject>(getGameState().currentDefaultLevelCrossingType);
            //            totalCost += Economy::getInflationAdjustedCost(levelCrossObj->costFactor, levelCrossObj->costIndex, 10);
            //        }

            //        if ((flags & Flags::apply) && !(flags & (Flags::ghost | Flags::aiAllocated)))
            //        {
            //            World::TileManager::removeAllWallsOnTileBelow(World::toTileSpace(trackLoc), baseZ);
            //        }

            //        // 0x0049C015
            //        const auto posFlags = World::TileClearance::getPositionFlags();

            //        // Abridged flags for just above/underground
            //        const auto newGroundFlags = posFlags & (ElementPositionFlags::aboveGround | ElementPositionFlags::underground);
            //        if (_byte_1136072 != ElementPositionFlags::none && (*_byte_1136072 & newGroundFlags) == ElementPositionFlags::none)
            //        {
            //            setErrorText(StringIds::cant_build_partly_above_partly_below_ground);
            //            return FAILURE;
            //        }
            //        _byte_1136072 = newGroundFlags;

            //        if ((posFlags & ElementPositionFlags::partiallyUnderwater) != ElementPositionFlags::none)
            //        {
            //            setErrorText(StringIds::cant_build_this_underwater);
            //            return FAILURE;
            //        }
            //        if ((posFlags & ElementPositionFlags::underwater) != ElementPositionFlags::none)
            //        {
            //            setErrorText(StringIds::too_close_to_water_surface);
            //            return FAILURE;
            //        }

            //        if (!(flags & Flags::apply))
            //        {
            //            continue;
            //        }

            //        if (CompanyManager::isPlayerCompany(getUpdatingCompanyId()))
            //        {
            //            companyEmotionEvent(getUpdatingCompanyId(), Emotion::thinking);
            //        }
            //        if (!(flags & (Flags::ghost | Flags::aiAllocated)))
            //        {
            //            World::TileManager::removeSurfaceIndustryAtHeight(trackLoc);
            //            World::TileManager::setTerrainStyleAsClearedAtHeight(trackLoc);
            //        }

            //        auto* newElTrack = World::TileManager::insertElement<World::TrackElement>(trackLoc, baseZ, quarterTile.getBaseQuarterOccupied());
            //        if (newElTrack == nullptr)
            //        {
            //            return FAILURE;
            //        }
            //        newElTrack->setClearZ(clearZ);
            //        newElTrack->setRotation(args.rotation);
            //        newElTrack->setTrackObjectId(args.trackObjectId);
            //        newElTrack->setSequenceIndex(piece.index);
            //        newElTrack->setTrackId(args.trackId);
            //        newElTrack->setOwner(getUpdatingCompanyId());
            //        for (auto i = 0U; i < 4; ++i)
            //        {
            //            if (validMods & (1U << i))
            //            {
            //                newElTrack->setMod(i, true);
            //            }
            //        }
            //        newElTrack->setBridgeObjectId(args.bridge);
            //        newElTrack->setHasBridge(_byte_1136073 & (1U << 1));
            //        newElTrack->setHasLevelCrossing(hasLevelCrossing);
            //        newElTrack->setFlag6(piece.index == (trackPieces.size() - 1));
            //        newElTrack->setGhost(flags & Flags::ghost);
            //        newElTrack->setAiAllocated(flags & Flags::aiAllocated);
            //        if (!(flags & Flags::aiAllocated))
            //        {
            //            World::TileManager::mapInvalidateTileFull(trackLoc);
            //        }
        }

        //    if (_byte_1136073 & (1U << 0))
        //    {
        //        auto* bridgeObj = ObjectManager::get<BridgeObject>(args.bridge);
        //        const auto heightCost = _byte_1136074 * bridgeObj->heightCostFactor;
        //        const auto bridgeBaseCost = Economy::getInflationAdjustedCost(bridgeObj->baseCostFactor + heightCost, bridgeObj->costIndex, 10);
        //        auto cost = (bridgeBaseCost * World::TrackData::getTrackMiscData(args.trackId).costFactor) / 256;
        //        if (_byte_1136073 & (1U << 7))
        //        {
        //            cost *= 2;
        //        }
        //        totalCost += cost;
        //    }

        //    if ((_byte_1136072 & ElementPositionFlags::underground) != ElementPositionFlags::none)
        //    {
        //        const auto tunnelBaseCost = Economy::getInflationAdjustedCost(trackObj->tunnelCostFactor, 2, 8);
        //        auto cost = (tunnelBaseCost * World::TrackData::getTrackMiscData(args.trackId).costFactor) / 256;

        //        totalCost += cost;
        //    }

        //    if ((flags & Flags::apply) && !(flags & (Flags::aiAllocated | Flags::ghost)))
        //    {
        //        playConstructionPlacementSound(getPosition());
        //    }
        //    return totalCost;
    }

    void createRoad(registers& regs)
    {
        regs.ebx = createRoad(RoadPlacementArgs(regs), regs.bl);
    }
}
