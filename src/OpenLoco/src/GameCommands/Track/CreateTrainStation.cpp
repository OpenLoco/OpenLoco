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

        auto elTrack = [&args]() -> World::TrackElement* {
            auto tile = World::TileManager::get(args.pos);
            for (auto& el : tile)
            {
                auto* elTrack = el.as<World::TrackElement>();
                if (elTrack == nullptr)
                {
                    continue;
                }
                if (elTrack->baseHeight() != args.pos.z)
                {
                    continue;
                }
                if (elTrack->unkDirection() != args.rotation)
                {
                    continue;
                }
                if (elTrack->trackId() != args.trackId)
                {
                    continue;
                }
                if (elTrack->trackObjectId() != args.trackObjectId)
                {
                    continue;
                }
                if (elTrack->sequenceIndex() != args.index)
                {
                    continue;
                }
                return elTrack;
            }
            return nullptr;
        }();

        auto index = args.index;

        if (elTrack == nullptr)
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
            if (elTrack->hasStationElement())
            {
                setErrorText(StringIds::station_in_the_way);
                return FAILURE;
            }

            if (!sub_431E6A(elTrack->owner(), reinterpret_cast<World::TileElement*>(elTrack)))
            {
                return FAILURE;
            }
        }
        auto& piece = World::TrackData::getTrackPiece(args.trackId)[index];
        const auto previousPos = args.pos - World::Pos3(Math::Vector::rotate(World::Pos2(piece.x, piece.y), args.rotation), piece.z);

        if ((flags & Flags::ghost) && (flags & Flags::apply))
        {
            _lastConstructedAdjoiningStationCentrePos = previousPos;
            uint16_t tad = (args.trackId << 3) | args.rotation;
            auto nearbyStation = sub_48FF36(previousPos, tad, args.trackObjectId);
            _lastConstructedAdjoiningStationId = static_cast<int16_t>(nearbyStation.id);
        }

        if (!(flags & Flags::ghost))
        {
            if (flags & Flags::apply)
            {
                // 0x0048BDCE
            }
            else
            {
                auto func = (flags & Flags::flag_4) ? &sub_48FFF7 : &sub_48FF36;
                auto nearbyStation = func(previousPos, (args.trackId << 3) | args.rotation, args.trackObjectId);
                bool createNewStation = false;
                if (nearbyStation.id != StationId::null)
                {
                    _lastPlacedTrackStationId = nearbyStation.id;
                    auto* station = StationManager::get(nearbyStation.id);
                    if (station->stationTileSize > 80)
                    {
                        if (nearbyStation.isPhysicallyAttached)
                        {
                            setErrorText(StringIds::station_too_large);
                            return FAILURE;
                        }
                        createNewStation = true;
                    }
                    else
                    {
                        if (!(flags & Flags::flag_4))
                        {
                            if (sub_48FEF4(nearbyStation.id, previousPos))
                            {
                                if (nearbyStation.isPhysicallyAttached)
                                {
                                    setErrorText(StringIds::station_too_spread_out);
                                    return FAILURE;
                                }
                                createNewStation = true;
                            }
                        }
                    }
                }
                if (createNewStation)
                {
                    // 0x0048BDB6
                }
            }
        }
        // 0x0048BE52
    }

    void createTrainStation(registers& regs)
    {
        regs.ebx = createTrainStation(TrackStationPlacementArgs(regs), regs.bl);
    }
}
