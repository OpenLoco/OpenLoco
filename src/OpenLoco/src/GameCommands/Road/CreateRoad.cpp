#include "CreateRoad.h"
// #include "Audio/Audio.h"
#include "Economy/Economy.h"
// #include "GameState.h"
// #include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
// #include "Map/BuildingElement.h"
// #include "Map/RoadElement.h"
// #include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
// #include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "SceneManager.h"
// #include "Map/TrackElement.h"
// #include "Map/TreeElement.h"
#include "Objects/BridgeObject.h"
// #include "Objects/LevelCrossingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
// #include "Objects/TrackObject.h"
// #include "World/CompanyManager.h"
// #include "World/StationManager.h"
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

    // struct ClearFunctionArgs
    //{
    //     World::Pos3 pos;
    //     uint8_t rotation;
    //     uint8_t trackId;
    //     uint8_t unkFlags;
    //     uint8_t bridgeId;
    //     uint8_t trackObjectId;
    //     uint8_t index;
    //     bool isLastIndex;
    //     uint8_t flags;
    // };

    //// 0x0049C4FF
    // static World::TileClearance::ClearFuncResult clearTrack(World::TrackElement& elTrack, const ClearFunctionArgs& args)
    //{
    //     if (elTrack.hasBridge())
    //     {
    //         _byte_1136075 = elTrack.bridge();
    //     }

    //    const auto& targetPiece = World::TrackData::getTrackPiece(elTrack.trackId())[elTrack.sequenceIndex()];
    //    const auto& newPiece = World::TrackData::getTrackPiece(args.trackId)[args.index];

    //    const auto targetConnectFlags = targetPiece.connectFlags[elTrack.rotation()];
    //    const auto newConnectFlags = newPiece.connectFlags[args.rotation];
    //    if (!(targetConnectFlags & newConnectFlags))
    //    {
    //        return World::TileClearance::ClearFuncResult::noCollision;
    //    }

    //    if (args.unkFlags & (1U << 2))
    //    {
    //        setErrorText(StringIds::junctions_not_possible);
    //        return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //    }

    //    if (!sub_431E6A(elTrack.owner(), reinterpret_cast<const World::TileElement*>(&elTrack)))
    //    {
    //        return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //    }

    //    if (((World::TrackData::getTrackMiscData(elTrack.trackId()).flags & (CommonTraitFlags::slope | CommonTraitFlags::steepSlope)) != CommonTraitFlags::none)
    //        || ((World::TrackData::getTrackMiscData(args.trackId).flags & (CommonTraitFlags::slope | CommonTraitFlags::steepSlope)) != CommonTraitFlags::none))
    //    {
    //        setErrorText(StringIds::junction_must_be_entirely_level);
    //        return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //    }

    //    auto* targetTrackObj = ObjectManager::get<TrackObject>(elTrack.trackObjectId());
    //    auto* trackObj = ObjectManager::get<TrackObject>(args.trackObjectId);
    //    if (elTrack.baseHeight() != args.pos.z)
    //    {
    //        FormatArguments::common(targetTrackObj->name);
    //        setErrorText(StringIds::string_id_in_the_way_wrong_height_for_junction);
    //        return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //    }

    //    if (elTrack.hasSignal())
    //    {
    //        setErrorText(StringIds::signal_in_the_way);
    //        return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //    }

    //    if (elTrack.hasStationElement())
    //    {
    //        setErrorText(StringIds::station_in_the_way);
    //        return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //    }

    //    if (!((args.rotation ^ elTrack.rotation()) & 0b1))
    //    {
    //        if (((World::TrackData::getTrackMiscData(elTrack.trackId()).flags ^ World::TrackData::getTrackMiscData(args.trackId).flags) & CommonTraitFlags::oneSided) != CommonTraitFlags::none)
    //        {
    //            setErrorText(StringIds::track_combination_not_possible);
    //            return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //        }
    //    }

    //    if (elTrack.hasBridge())
    //    {
    //        if (elTrack.bridge() != args.bridgeId)
    //        {
    //            setErrorText(StringIds::bridge_types_must_match);
    //            return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //        }

    //        auto* bridgeObj = ObjectManager::get<BridgeObject>(args.bridgeId);
    //        if ((bridgeObj->disabledTrackCfg & CommonTraitFlags::junction) != CommonTraitFlags::none)
    //        {
    //            setErrorText(StringIds::bridge_not_suitable_for_junction);
    //            return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //        }
    //    }

    //    if (elTrack.trackObjectId() == args.trackObjectId)
    //    {
    //        if (elTrack.trackId() == args.trackId)
    //        {
    //            if (elTrack.rotation() == args.rotation)
    //            {
    //                if (elTrack.sequenceIndex() == args.index)
    //                {
    //                    setErrorText(StringIds::already_built_here);
    //                    return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //                }
    //            }
    //        }
    //        // This is working out reversed elements
    //        if (World::TrackData::getTrackMiscData(elTrack.trackId()).reverseTrackId == args.trackId)
    //        {
    //            if (((World::TrackData::getTrackMiscData(elTrack.trackId()).reverseRotation + elTrack.rotation()) & 0x3) == args.rotation)
    //            {
    //                if (args.isLastIndex && elTrack.sequenceIndex() == 0)
    //                {
    //                    setErrorText(StringIds::already_built_here);
    //                    return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //                }
    //            }
    //        }
    //    }

    //    if (elTrack.trackObjectId() == args.trackObjectId)
    //    {
    //        if (!targetTrackObj->hasTraitFlags(TrackTraitFlags::junction))
    //        {
    //            setErrorText(StringIds::junctions_not_possible);
    //            return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //        }
    //    }
    //    else
    //    {
    //        if (!(targetTrackObj->compatibleTracks & (1U << args.trackObjectId))
    //            && !(trackObj->compatibleTracks & (1U << elTrack.trackObjectId())))
    //        {
    //            FormatArguments::common(targetTrackObj->name);
    //            setErrorText(StringIds::unable_to_cross_or_create_junction_with_string);
    //            return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //        }
    //    }

    //    _byte_1136073 = _byte_1136073 | (1U << 3);
    //    return World::TileClearance::ClearFuncResult::noCollision;
    //}

    //// 0x0049C401
    // static World::TileClearance::ClearFuncResult clearRoad(World::RoadElement& elRoad, const ClearFunctionArgs& args, bool& hasLevelCrossing)
    //{
    //     if (elRoad.hasBridge())
    //     {
    //         _byte_1136075 = elRoad.bridge();
    //         auto* bridgeObj = ObjectManager::get<BridgeObject>(elRoad.bridge());
    //         if ((bridgeObj->disabledTrackCfg & CommonTraitFlags::junction) != CommonTraitFlags::none)
    //         {
    //             setErrorText(StringIds::bridge_not_suitable_for_junction);
    //             return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //         }
    //     }

    //    auto* roadObj = ObjectManager::get<RoadObject>(elRoad.roadObjectId());
    //    auto* trackObj = ObjectManager::get<TrackObject>(args.trackObjectId);

    //    if (!(roadObj->compatibleTracks & (1U << args.trackObjectId))
    //        && !(trackObj->compatibleRoads & (1U << elRoad.roadObjectId())))
    //    {
    //        FormatArguments::common(roadObj->name);
    //        setErrorText(StringIds::unable_to_cross_or_create_junction_with_string);
    //        return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //    }

    //    if (elRoad.hasSignalElement())
    //    {
    //        setErrorText(StringIds::signal_in_the_way);
    //        return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //    }

    //    if (elRoad.hasStationElement())
    //    {
    //        setErrorText(StringIds::station_in_the_way);
    //        return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //    }

    //    if (elRoad.hasBridge())
    //    {
    //        if (elRoad.bridge() != args.bridgeId)
    //        {
    //            setErrorText(StringIds::bridge_types_must_match);
    //            return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //        }
    //    }

    //    if (elRoad.roadId() != 0
    //        || args.trackId != 0
    //        || !((elRoad.rotation() - args.rotation) & 0b1)
    //        || elRoad.baseHeight() != args.pos.z)
    //    {
    //        setErrorText(StringIds::level_crossing_only_possible_with_straight_road_and_track_at_same_level);
    //        return World::TileClearance::ClearFuncResult::collisionErrorSet;
    //    }

    //    hasLevelCrossing = true;
    //    _byte_1136073 = _byte_1136073 | (1U << 2);

    //    if (!(args.flags & (Flags::aiAllocated | Flags::ghost))
    //        && (args.flags & Flags::apply))
    //    {
    //        elRoad.setHasLevelCrossing(true);
    //        elRoad.setLevelCrossingObjectId(getGameState().currentDefaultLevelCrossingType);
    //        elRoad.setUnk7_10(false);
    //        elRoad.setUnk6l(0);
    //    }

    //    return World::TileClearance::ClearFuncResult::noCollision;
    //}

    //// 0x0049C275
    // static World::TileClearance::ClearFuncResult clearFunction(World::TileElement& el, currency32_t& totalCost, bool& hasLevelCrossing, std::set<World::Pos3, World::LessThanPos3>& removedBuildings, const ClearFunctionArgs& args)
    //{
    //     switch (el.type())
    //     {
    //         case World::ElementType::track:
    //         {
    //             auto* elTrack = el.as<World::TrackElement>();
    //             if (elTrack != nullptr)
    //             {
    //                 return clearTrack(*elTrack, args);
    //             }
    //             break;
    //         }
    //         case World::ElementType::station:
    //         {
    //             auto* elStation = el.as<World::StationElement>();
    //             if (elStation->stationType() == StationType::trainStation)
    //             {
    //                 return World::TileClearance::ClearFuncResult::noCollision;
    //             }
    //             return World::TileClearance::ClearFuncResult::collision;
    //         }
    //         case World::ElementType::signal:
    //             return World::TileClearance::ClearFuncResult::noCollision;
    //         case World::ElementType::building:
    //         {
    //             auto* elBuilding = el.as<World::BuildingElement>();
    //             if (elBuilding == nullptr)
    //             {
    //                 return World::TileClearance::ClearFuncResult::noCollision;
    //             }
    //             _byte_1136073 = _byte_1136073 | (1U << 4);
    //             return World::TileClearance::clearBuildingCollision(*elBuilding, args.pos, removedBuildings, args.flags, totalCost);
    //         }
    //         case World::ElementType::tree:
    //         {
    //             auto* elTree = el.as<World::TreeElement>();
    //             if (elTree == nullptr)
    //             {
    //                 return World::TileClearance::ClearFuncResult::noCollision;
    //             }
    //             return World::TileClearance::clearTreeCollision(*elTree, args.pos, args.flags, totalCost);
    //         }
    //         case World::ElementType::road:
    //         {
    //             auto* elRoad = el.as<World::RoadElement>();
    //             if (elRoad != nullptr)
    //             {
    //                 return clearRoad(*elRoad, args, hasLevelCrossing);
    //             }
    //             break;
    //         }
    //         case World::ElementType::surface:
    //         case World::ElementType::wall:
    //         case World::ElementType::industry:
    //             return World::TileClearance::ClearFuncResult::collision;
    //     }
    //     return World::TileClearance::ClearFuncResult::collision;
    // }

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

            //        // 0x0113607C
            //        bool hasLevelCrossing = false;

            //        ClearFunctionArgs clearArgs{};
            //        clearArgs.pos = trackLoc;
            //        clearArgs.rotation = args.rotation;
            //        clearArgs.trackId = args.trackId;
            //        clearArgs.unkFlags = args.unkFlags;
            //        clearArgs.bridgeId = args.bridge;
            //        clearArgs.trackObjectId = args.trackObjectId;
            //        clearArgs.index = piece.index;
            //        clearArgs.isLastIndex = piece.index == (trackPieces.size() - 1);
            //        clearArgs.flags = flags;

            //        auto clearFunc = [&totalCost, &hasLevelCrossing, &removedBuildings, &clearArgs](World::TileElement& el) {
            //            return clearFunction(el, totalCost, hasLevelCrossing, removedBuildings, clearArgs);
            //        };

            //        if (!World::TileClearance::applyClearAtStandardHeight(trackLoc, baseZ, clearZ, quarterTile, clearFunc))
            //        {
            //            return FAILURE;
            //        }

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
