#include "CreateTrack.h"
#include "Audio/Audio.h"
#include "Economy/Economy.h"
#include "GameState.h"
#include "Localisation/StringIds.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/BridgeObject.h"
#include "Objects/LevelCrossingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Random.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco::GameCommands
{
    using namespace World::TileManager;

    static loco_global<ElementPositionFlags, 0x01136072> _byte_1136072;
    static loco_global<uint8_t, 0x01136073> _byte_1136073;
    static loco_global<World::MicroZ, 0x01136074> _byte_1136074;
    static loco_global<uint8_t, 0x01136075> _byte_1136075;
    static loco_global<uint16_t[44], 0x004F8764> _4F8764;

    // TODO: Move this somewhere else used by multiple game commands
    // 0x0048B013
    void playPlacementSound(World::Pos3 pos)
    {
        const auto frequency = gPrng1().randNext(17955, 26146);
        Audio::playSound(Audio::SoundId::construct, pos, 0, frequency);
    }

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

    // 0x0049C275
    World::TileClearance::ClearFuncResult clearFunction(World::TileElement& el, uint8_t flags, currency32_t& totalCost, uint8_t trackObjectId, uint8_t bridgeId, uint8_t trackId, uint8_t rotation, bool& hasLevelCrossing, uint8_t pieceIndex, bool isLastPiece)
    {
        // 0x0113607C hasLevelCrossing
        // 0x01135F5E pieceIndex
        // 0x01135F5E isLastPiece as well
        // ebp+14h flags
        // ebp+0h cost
        // ebp+Ch trackObjectId
        // ebp+Fh bridge
        // ebp+Dh trackId
        // ebp+15h rotation
        return World::TileClearance::ClearFuncResult::noCollision;
    }

    // 0x0049BB98
    uint32_t createTrack(const TrackPlacementArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3{ 16, 16, 0 });
        _byte_1136072 = ElementPositionFlags::none;
        _byte_1136073 = 0; // Bridge related
        _byte_1136074 = 0;
        _byte_1136075 = 0xFFU;
        // 0x01135C68 = unkFlags

        if (flags & Flags::apply && !(flags & Flags::aiAllocated))
        {
            companySetObservation(getUpdatingCompanyId(), ObservationStatus::buildingTrackRoad, args.pos, EntityId::null, args.trackObjectId);
        }

        if (World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        auto* trackObj = ObjectManager::get<TrackObject>(args.trackObjectId);
        const auto compatFlags = World::TrackData::getTrackCompatibleFlags(args.trackId);
        uint8_t validMods = args.mods;

        for (auto i = 0U; i < 4; ++i)
        {
            if (args.mods & (1U << i))
            {
                auto* extraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[i]);
                if ((compatFlags & extraObj->trackPieces) != compatFlags)
                {
                    validMods &= ~(1U << i);
                }
            }
        }

        // This is a check from RCT2 and is not used in loco TRACK_ELEM_FLAG_STARTS_AT_HALF_HEIGHT
        if (_4F8764[args.trackId] & (1U << 10))
        {
            if ((args.pos.z & 0xF) != 8)
            {
                return FAILURE;
            }
        }
        else
        {
            if (args.pos.z & 0xF)
            {
                return FAILURE;
            }
        }

        currency32_t totalCost = 0;

        {
            const auto trackBaseCost = Economy::getInflationAdjustedCost(trackObj->buildCostFactor, trackObj->costIndex, 10);
            const auto cost = (trackBaseCost * World::TrackData::getTrackCostFactor(args.trackId)) / 256;
            totalCost += cost;
        }
        for (auto i = 0U; i < 4; ++i)
        {
            if (validMods & (1U << i))
            {
                auto* extraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[i]);
                const auto trackExtraBaseCost = Economy::getInflationAdjustedCost(extraObj->buildCostFactor, extraObj->costIndex, 10);
                const auto cost = (trackExtraBaseCost * World::TrackData::getTrackCostFactor(args.trackId)) / 256;
                totalCost += cost;
            }
        }

        auto& trackPieces = World::TrackData::getTrackPiece(args.trackId);
        for (auto& piece : trackPieces)
        {
            const auto trackLoc = args.pos + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };
            const auto quarterTile = piece.subTileClearance.rotate(args.rotation);
            const auto unk = Numerics::rotl4bit(enumValue(piece.flags) & 0xF, 1);

            if (trackLoc.z < 16)
            {
                setErrorText(StringIds::error_too_low);
                return FAILURE;
            }

            const auto baseZ = trackLoc.z / World::kSmallZStep;
            auto clearZ = baseZ + (piece.clearZ + 32) / World::kSmallZStep;
            _byte_1136073 = _byte_1136073 & ~(1U << 1);

            // Why aren't we just failing invalid???
            if (World::validCoords(trackLoc))
            {
                const auto tile = World::TileManager::get(trackLoc);
                auto* elSurface = tile.surface();
                if (elSurface->water())
                {
                    _byte_1136073 = _byte_1136073 | (1U << 7);
                }

                const bool requiresBridge = isBridgeRequired(baseZ, *elSurface, piece, unk);
                if (requiresBridge)
                {
                    // 0x0049BF1E
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
                    if (bridgeObj->disabledTrackCfg & _4F8764[args.trackId])
                    {
                        setErrorText(StringIds::bridge_type_unsuitable_for_this_configuration);
                        return FAILURE;
                    }
                    clearZ += bridgeObj->clearHeight / World::kSmallZStep;
                }
            }
            // 0x0049BF7C
            if (clearZ > 236)
            {
                setErrorText(StringIds::error_too_high);
                return FAILURE;
            }

            // 0x0113607C
            bool hasLevelCrossing = false;
            // 0x01135F5E
            uint8_t pieceIndex = piece.index;
            // 0x01135F5E as well
            bool isLastPiece = piece.index == (trackPieces.size() - 1);
            // ebp+14h flags
            // ebp+0h cost
            // ebp+Ch trackObjectId
            // ebp+Fh bridge
            // ebp+Dh trackId
            // ebp+15h rotation

            auto clearFunc = [flags, &totalCost, trackObjectId = args.trackObjectId, bridgeId = args.bridge, trackId = args.trackId, rotation = args.rotation, &hasLevelCrossing, pieceIndex, isLastPiece](World::TileElement& el) {
                return clearFunction(el, flags, totalCost, trackObjectId, bridgeId, trackId, rotation, hasLevelCrossing, pieceIndex, isLastPiece);
            };

            if (!World::TileClearance::applyClearAtStandardHeight(trackLoc, baseZ, clearZ, quarterTile, clearFunc))
            {
                return FAILURE;
            }

            if (hasLevelCrossing)
            {
                auto* levelCrossObj = ObjectManager::get<LevelCrossingObject>(getGameState().var_1AC);
                totalCost += Economy::getInflationAdjustedCost(levelCrossObj->costFactor, levelCrossObj->costIndex, 10);
            }

            if (flags & Flags::apply && !(flags & (Flags::ghost | Flags::aiAllocated)))
            {
                World::TileManager::removeAllWallsOnTileBelow(World::toTileSpace(trackLoc), baseZ);
            }

            // 0x0049C015
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

            if (CompanyManager::isPlayerCompany(getUpdatingCompanyId()))
            {
                companyEmotionEvent(getUpdatingCompanyId(), Emotion::thinking);
            }
            if (!(flags & (Flags::ghost | Flags::aiAllocated)))
            {
                // sub_46908D
                // sub_469174
            }

            auto* newElTrack = World::TileManager::insertElement<World::TrackElement>(trackLoc, baseZ, quarterTile.getBaseQuarterOccupied());
            if (newElTrack == nullptr)
            {
                return FAILURE;
            }
            newElTrack->setClearZ(clearZ);
            newElTrack->setRotation(args.rotation);
            newElTrack->setTrackObjectId(args.trackObjectId);
            newElTrack->setSequenceIndex(pieceIndex);
            newElTrack->setTrackId(args.trackId);
            newElTrack->setOwner(getUpdatingCompanyId());
            for (auto i = 0U; i < 4; ++i)
            {
                if (validMods & (1U << i))
                {
                    newElTrack->setMod(i, true);
                }
            }
            newElTrack->setBridgeObjectId(args.bridge);
            newElTrack->setHasBridge(_byte_1136073 & (1U << 1));
            newElTrack->setHasLevelCrossing(hasLevelCrossing);
            newElTrack->setFlag6(isLastPiece);
            newElTrack->setGhost(flags & Flags::ghost);
            newElTrack->setAiAllocated(flags & Flags::aiAllocated);
            if (!(flags & Flags::aiAllocated))
            {
                World::TileManager::mapInvalidateTileFull(trackLoc);
            }
        }

        if (_byte_1136073 & (1U << 0))
        {
            auto* bridgeObj = ObjectManager::get<BridgeObject>(args.bridge);
            const auto heightCost = _byte_1136074 * bridgeObj->heightCostFactor;
            const auto bridgeBaseCost = Economy::getInflationAdjustedCost(bridgeObj->baseCostFactor + heightCost, bridgeObj->costIndex, 10);
            auto cost = (bridgeBaseCost * World::TrackData::getTrackCostFactor(args.trackId)) / 256;
            if (_byte_1136073 & (1U << 7))
            {
                cost *= 2;
            }
            totalCost += cost;
        }

        if ((_byte_1136072 & ElementPositionFlags::underground) != ElementPositionFlags::none)
        {
            const auto tunnelBaseCost = Economy::getInflationAdjustedCost(trackObj->tunnelCostFactor, 2, 8);
            auto cost = (tunnelBaseCost * World::TrackData::getTrackCostFactor(args.trackId)) / 256;

            totalCost += cost;
        }

        if (flags & Flags::apply && !(flags & (Flags::aiAllocated | Flags::ghost)))
        {
            playPlacementSound(getPosition());
        }
        return totalCost;
    }

    void createTrack(registers& regs)
    {
        regs.ebx = createTrack(TrackPlacementArgs(regs), regs.bl);
    }
}
