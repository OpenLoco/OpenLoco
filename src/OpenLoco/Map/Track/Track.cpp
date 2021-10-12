#include "Track.h"
#include "../../Interop/Interop.hpp"
#include "../TileManager.h"
#include "TrackData.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Map::Track
{
    static loco_global<uint32_t, 0x00525FC0> _525FC0;
    static loco_global<uint8_t, 0x0112C2EE> _112C2EE;
    static loco_global<uint8_t, 0x0112C2ED> _112C2ED;
    static loco_global<StationId, 0x01135FAE> _1135FAE;
    static loco_global<uint8_t[2], 0x0113601A> _113601A;
    static loco_global<uint16_t, 0x01136087> _1136087;
    static loco_global<uint8_t, 0x0113607D> _113607D;

    // 0x00478895
    void getRoadConnections(const Map::Pos3& pos, TrackConnections& data, const CompanyId company, const uint8_t roadObjectId, const uint16_t trackAndDirection)
    {
        const auto nextTrackPos = pos + TrackData::getUnkRoad(trackAndDirection).pos;
        _1135FAE = StationId::null; // stationId

        uint8_t baseZ = nextTrackPos.z / 4;
        uint8_t nextRotation = TrackData::getUnkRoad(trackAndDirection).rotationEnd;
        _112C2EE = nextRotation;

        const auto tile = Map::TileManager::get(nextTrackPos);
        for (const auto& el : tile)
        {
            auto* elRoad = el.as<RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }

            if (!(_525FC0 & (1 << elRoad->roadObjectId())))
            {
                if (elRoad->owner() != company)
                {
                    continue;
                }
            }

            if (elRoad->roadObjectId() != roadObjectId)
            {
                if (roadObjectId != 0xFF)
                {
                    continue;
                }
                if (!(_525FC0 & (1 << roadObjectId)))
                {
                    continue;
                }
            }

            if ((elRoad->mods() & _113601A[0]) != _113601A[0])
            {
                continue;
            }

            if (elRoad->isGhost() || elRoad->isFlag5())
            {
                continue;
            }

            if (elRoad->sequenceIndex() == 0)
            {
                auto trackAndDirection2 = (elRoad->roadId() << 3) | elRoad->unkDirection();
                if (nextRotation == TrackData::getUnkRoad(trackAndDirection2).rotationBegin)
                {
                    const auto& roadPiece = TrackData::getRoadPiece(elRoad->roadId());
                    if (baseZ == (elRoad->baseZ() - roadPiece[0].z / 4))
                    {
                        if (elRoad->hasBridge())
                        {
                            trackAndDirection2 |= elRoad->bridge() << 9;
                            trackAndDirection2 |= (1 << 12);
                        }

                        if (_113601A[1] != elRoad->mods())
                        {
                            trackAndDirection2 |= (1 << 13);
                        }

                        if (elRoad->hasStationElement())
                        {
                            auto* elStation = tile.roadStation(elRoad->roadId(), elRoad->unkDirection(), elRoad->baseZ());
                            if (elStation == nullptr)
                            {
                                continue;
                            }

                            if (!elStation->isFlag5() && !elStation->isGhost())
                            {
                                _1135FAE = elStation->stationId();
                                _1136087 = elStation->objectId();
                            }
                        }

                        _112C2ED = elRoad->roadObjectId();
                        data.push_back(trackAndDirection2);
                    }
                }
            }

            if (!elRoad->isFlag6())
            {
                continue;
            }

            auto trackAndDirection2 = (elRoad->roadId() << 3) | (1 << 2) | elRoad->unkDirection();
            if (nextRotation != TrackData::getUnkRoad(trackAndDirection2).rotationBegin)
            {
                continue;
            }
            const auto& roadPiece = TrackData::getRoadPiece(elRoad->roadId());
            if (baseZ != (elRoad->baseZ() - (TrackData::getUnkRoad(trackAndDirection2).pos.z + roadPiece[elRoad->sequenceIndex()].z) / 4))
            {
                continue;
            }
            if (elRoad->hasBridge())
            {
                trackAndDirection2 |= elRoad->bridge() << 9;
                trackAndDirection2 |= (1 << 12);
            }

            if (_113601A[1] != elRoad->mods())
            {
                trackAndDirection2 |= (1 << 13);
            }

            if (elRoad->hasStationElement())
            {
                auto* elStation = tile.roadStation(elRoad->roadId(), elRoad->unkDirection(), elRoad->baseZ());
                if (elStation == nullptr)
                {
                    continue;
                }

                if (!elStation->isFlag5() && !elStation->isGhost())
                {
                    _1135FAE = elStation->stationId();
                    _1136087 = elStation->objectId();
                }
            }
            data.push_back(trackAndDirection2);
        }
    }

    // 0x004A2604
    void getTrackConnections(const Map::Pos3& pos, TrackConnections& data, const CompanyId company, const uint8_t trackObjectId, const uint16_t trackAndDirection)
    {
        const auto nextTrackPos = pos + TrackData::getUnkTrack(trackAndDirection).pos;
        _1135FAE = StationId::null; // stationId
        _113607D = 0;

        uint8_t baseZ = nextTrackPos.z / 4;
        uint8_t nextRotation = TrackData::getUnkTrack(trackAndDirection).rotationEnd;

        const auto tile = Map::TileManager::get(nextTrackPos);
        for (const auto& el : tile)
        {
            auto* elTrack = el.as<TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }

            if (elTrack->owner() != company)
            {
                continue;
            }

            if (elTrack->trackObjectId() != trackObjectId)
            {
                continue;
            }

            if ((elTrack->mods() & _113601A[0]) != _113601A[0])
            {
                continue;
            }

            if (elTrack->isGhost() || elTrack->isFlag5())
            {
                continue;
            }

            if (elTrack->sequenceIndex() == 0)
            {
                auto trackAndDirection2 = (elTrack->trackId() << 3) | elTrack->unkDirection();
                if (nextRotation == TrackData::getUnkTrack(trackAndDirection2).rotationBegin)
                {
                    const auto& trackPiece = TrackData::getTrackPiece(elTrack->trackId());
                    if (baseZ == (elTrack->baseZ() - trackPiece[0].z / 4))
                    {
                        if (elTrack->hasBridge())
                        {
                            trackAndDirection2 |= elTrack->bridge() << 9;
                            trackAndDirection2 |= (1 << 12);
                        }

                        if (_113601A[1] != elTrack->mods())
                        {
                            trackAndDirection2 |= (1 << 13);
                        }

                        if (elTrack->hasStationElement())
                        {
                            auto* elStation = elTrack->next()->as<StationElement>();
                            if (elStation == nullptr)
                            {
                                continue;
                            }

                            if (!elStation->isFlag5() && !elStation->isGhost())
                            {
                                _1135FAE = elStation->stationId();
                            }
                        }

                        if (elTrack->has_6_10())
                        {
                            _113607D = 1;
                        }

                        if (elTrack->hasSignal())
                        {
                            auto* elSignal = elTrack->next()->as<SignalElement>();
                            if (elSignal == nullptr)
                            {
                                continue;
                            }

                            if (!elSignal->isFlag5() && !elSignal->isGhost())
                            {
                                trackAndDirection2 |= (1 << 15);
                            }
                        }
                        data.push_back(trackAndDirection2);
                    }
                }
            }

            if (!elTrack->isFlag6())
            {
                continue;
            }

            auto trackAndDirection2 = (elTrack->trackId() << 3) | (1 << 2) | elTrack->unkDirection();
            if (nextRotation != TrackData::getUnkTrack(trackAndDirection2).rotationBegin)
            {
                continue;
            }

            const auto previousBaseZ = elTrack->baseZ() - (TrackData::getTrackPiece(elTrack->trackId())[elTrack->sequenceIndex()].z + TrackData::getUnkTrack(trackAndDirection2).pos.z) / 4;
            if (previousBaseZ != baseZ)
            {
                continue;
            }

            if (elTrack->hasBridge())
            {
                trackAndDirection2 |= elTrack->bridge() << 9;
                trackAndDirection2 |= (1 << 12);
            }

            if (_113601A[1] != elTrack->mods())
            {
                trackAndDirection2 |= (1 << 13);
            }

            if (elTrack->hasStationElement())
            {
                auto* elStation = elTrack->next()->as<StationElement>();
                if (elStation == nullptr)
                {
                    continue;
                }

                if (!elStation->isFlag5() && !elStation->isGhost())
                {
                    _1135FAE = elStation->stationId();
                }
            }

            if (elTrack->has_6_10())
            {
                _113607D = 1;
            }

            if (elTrack->hasSignal())
            {
                auto* elSignal = elTrack->next()->as<SignalElement>();
                if (elSignal == nullptr)
                {
                    continue;
                }

                if (!elSignal->isFlag5() && !elSignal->isGhost())
                {
                    trackAndDirection2 |= (1 << 15);
                }
            }
            data.push_back(trackAndDirection2);
        }
    }
}
