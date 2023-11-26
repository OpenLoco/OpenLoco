#include "CreateTrainStation.h"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainStationObject.h"
#include "World/StationManager.h"

namespace OpenLoco::GameCommands
{
    static loco_global<StationId, 0x0112C730> _lastPlacedTrackStationId;
    static loco_global<bool, 0x0112C7A9> _112C7A9;
    static loco_global<uint32_t, 0x00112C734> _lastConstructedAdjoiningStationId;           // Can be 0xFFFF'FFFFU for no adjoining station
    static loco_global<World::Pos2, 0x00112C792> _lastConstructedAdjoiningStationCentrePos; // Can be x = -1 for no adjoining station

    struct NearbyStation
    {
        StationId id;
        bool isPhysicallyAttached;
    };

    // 0x0048FF36
    NearbyStation sub_48FF36(World::Pos3 pos, uint16_t tad, uint8_t trackObjectId)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.dx = pos.z;
        regs.bp = tad;
        regs.bh = trackObjectId;
        call(0x0048FF36, regs);
        NearbyStation result{};
        result.id = static_cast<StationId>(regs.bx);
        result.isPhysicallyAttached = regs.eax & (1U << 31);
        return result;
    }

    bool sub_48FEF4(StationId id, World::Pos3 pos)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.dx = pos.z;
        regs.ebx = enumValue(id);
        return call(0x0048FF36, regs) & X86_FLAG_CARRY;
    }

    // 0x0048FFF7
    NearbyStation sub_48FFF7(World::Pos3 pos, uint16_t tad, uint8_t trackObjectId)
    {
        // This one is for ai preview allocated track
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.dx = pos.z;
        regs.bp = tad;
        regs.bh = trackObjectId;
        call(0x0048FFF7, regs);
        NearbyStation result{};
        result.id = static_cast<StationId>(regs.bx);
        result.isPhysicallyAttached = regs.eax & (1U << 31);
        return result;
    }

    enum class NearbyStationValidation
    {
        okay,
        requiresNewStation,
        failure,
    };

    // 0x0048BDCE & 0x0048BD40
    std::pair<NearbyStationValidation, StationId> validateNearbyStation(const World::Pos3 pos, const uint8_t rotation, const uint16_t tad, uint8_t trackObjectId, const uint8_t flags)
    {
        auto func = (flags & Flags::flag_4) ? &sub_48FFF7 : &sub_48FF36;
        auto nearbyStation = func(pos, tad, trackObjectId);
        if (nearbyStation.id == StationId::null)
        {
            return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
        }

        // _lastPlacedTrackStationId = nearbyStation.id; set in callers
        auto* station = StationManager::get(nearbyStation.id);
        if (station->stationTileSize > 80)
        {
            if (nearbyStation.isPhysicallyAttached)
            {
                setErrorText(StringIds::station_too_large);
                return std::make_pair(NearbyStationValidation::failure, StationId::null);
            }
            return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
        }
        else
        {
            if (!(flags & Flags::flag_4))
            {
                if (sub_48FEF4(nearbyStation.id, pos))
                {
                    if (nearbyStation.isPhysicallyAttached)
                    {
                        setErrorText(StringIds::station_too_spread_out);
                        return std::make_pair(NearbyStationValidation::failure, StationId::null);
                    }
                    return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
                }
            }
        }
        return std::make_pair(NearbyStationValidation::okay, nearbyStation.id);
    }

    World::TrackElement* getElTrack(World::Pos3 pos, uint8_t rotation, uint8_t trackObjectId, uint8_t trackId, uint8_t index)
    {
        auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* elTrack = el.as<World::TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }
            if (elTrack->baseHeight() != pos.z)
            {
                continue;
            }
            if (elTrack->unkDirection() != rotation)
            {
                continue;
            }
            if (elTrack->trackId() != trackId)
            {
                continue;
            }
            if (elTrack->trackObjectId() != trackObjectId)
            {
                continue;
            }
            if (elTrack->sequenceIndex() != index)
            {
                continue;
            }
            return elTrack;
        }
        return nullptr;
    }

    // 0x0048BB20
    currency32_t createTrainStation(const TrackStationPlacementArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));

        _lastPlacedTrackStationId = StationId::null;
        _lastConstructedAdjoiningStationCentrePos = World::Pos2(-1, -1);
        _lastConstructedAdjoiningStationId = 0xFFFFFFFFU;

        auto* trackObj = ObjectManager::get<TrackObject>(args.trackObjectId);
        auto* stationObj = ObjectManager::get<TrainStationObject>(args.type);

        const auto compatibleTrack = trackObj->stationTrackPieces & stationObj->trackPieces & World::TrackData::getTrackCompatibleFlags(args.trackId);
        if (compatibleTrack == 0)
        {
            setErrorText(StringIds::track_road_unsuitable_for_station);
            return FAILURE;
        }

        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        auto* initialElTrack = getElTrack(args.pos, args.rotation, args.trackObjectId, args.trackId, args.index);

        auto index = args.index;

        if (initialElTrack == nullptr)
        {
            if (flags & Flags::apply)
            {
                return FAILURE;
            }
            if (!(flags & Flags::flag_4))
            {
                return FAILURE;
            }
            // Why???
            index = 0;
        }
        else
        {
            if (initialElTrack->hasStationElement())
            {
                setErrorText(StringIds::station_in_the_way);
                return FAILURE;
            }

            if (!sub_431E6A(initialElTrack->owner(), reinterpret_cast<World::TileElement*>(initialElTrack)))
            {
                return FAILURE;
            }
        }
        auto& trackPieces = World::TrackData::getTrackPiece(args.trackId);
        auto& argPiece = trackPieces[index];
        const auto trackStart = args.pos - World::Pos3(Math::Vector::rotate(World::Pos2(argPiece.x, argPiece.y), args.rotation), argPiece.z);

        if ((flags & Flags::ghost) && (flags & Flags::apply))
        {
            _lastConstructedAdjoiningStationCentrePos = trackStart;
            uint16_t tad = (args.trackId << 3) | args.rotation;
            auto nearbyStation = sub_48FF36(trackStart, tad, args.trackObjectId);
            _lastConstructedAdjoiningStationId = static_cast<int16_t>(nearbyStation.id);
        }

        if (!(flags & Flags::ghost))
        {
            if (flags & Flags::apply)
            {
                auto [result, nearbyStationId] = validateNearbyStation(trackStart, args.rotation, (args.trackId << 3) | args.rotation, args.trackObjectId, flags);
                switch (result)
                {
                    case NearbyStationValidation::failure:
                        // Odd???
                        break;
                    case NearbyStationValidation::requiresNewStation:
                    {
                        const auto newStationId = StationManager::allocateNewStation(trackStart, getUpdatingCompanyId(), 0);
                        if (newStationId != StationId::null)
                        {
                            _lastPlacedTrackStationId = newStationId;
                            auto* station = StationManager::get(newStationId);
                            station->updateLabel();
                        }
                    }
                    case NearbyStationValidation::okay:
                        _lastPlacedTrackStationId = nearbyStationId;
                        break;
                }
                _112C7A9 = true;
            }
            else
            {
                // Same as the other branch but deallocate after allocating and return failure on failure
                auto [result, nearbyStationId] = validateNearbyStation(trackStart, args.rotation, (args.trackId << 3) | args.rotation, args.trackObjectId, flags);
                switch (result)
                {
                    case NearbyStationValidation::failure:
                        break;
                    case NearbyStationValidation::requiresNewStation:
                    {
                        const auto newStationId = StationManager::allocateNewStation(trackStart, getUpdatingCompanyId(), 0);
                        if (newStationId == StationId::null)
                        {
                            return FAILURE;
                        }
                        StationManager::deallocateStation(newStationId);
                        // _lastPlacedTrackStationId not set but thats fine since this is the no apply side
                    }
                    case NearbyStationValidation::okay:
                        _lastPlacedTrackStationId = nearbyStationId;
                        break;
                }
            }
        }

        for (auto& piece : trackPieces)
        {
            const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

            auto* elTrack = getElTrack(trackLoc, args.rotation, args.trackObjectId, args.trackId, piece.index);
            if (elTrack == nullptr)
            {
                // 0x0048BEC7
                // Why are we doing anything???
            }
            else
            {
                if (elTrack->hasSignal())
                {
                    setErrorText(StringIds::signal_in_the_way);
                    return FAILURE;
                }
                if (elTrack->hasLevelCrossing())
                {
                    setErrorText(StringIds::level_crossing_in_the_way);
                    return FAILURE;
                }
                // Connect flags validation
                piece.connectFlags;
                // 0x0048BF1F
            }
        }
        if (!(flags & Flags::ghost) && (flags & Flags::apply))
        {
            // 0x0048C2CD
        }
    }

    void createTrainStation(registers& regs)
    {
        regs.ebx = createTrainStation(TrackStationPlacementArgs(regs), regs.bl);
    }
}
