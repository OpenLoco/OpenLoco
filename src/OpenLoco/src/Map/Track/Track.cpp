#include "Track.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "TrackData.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Map::Track
{
    void TrackConnections::push_back(uint16_t value)
    {
        if (size + 1 < std::size(data))
        {
            data[size++] = value;
            data[size] = 0xFFFF;
        }
    }

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

    // Part of 0x004A2604
    // For 0x004A2604 call this followed by getTrackConnections
    std::pair<Map::Pos3, uint8_t> getTrackConnectionEnd(const Map::Pos3& pos, const uint16_t trackAndDirection)
    {
        const auto& trackData = TrackData::getUnkTrack(trackAndDirection);

        return std::make_pair(pos + trackData.pos, trackData.rotationEnd);
    }

    // 0x004A2638, 0x004A2601
    void getTrackConnections(const Map::Pos3& nextTrackPos, const uint8_t nextRotation, TrackConnections& data, const CompanyId company, const uint8_t trackObjectId)
    {
        _1135FAE = StationId::null; // stationId
        _113607D = 0;

        uint8_t baseZ = nextTrackPos.z / 4;

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

namespace OpenLoco::Map
{
    TrackElement::TrackElement(Map::SmallZ baseZ, Map::SmallZ clearZ, uint8_t direction, uint8_t quarterTile, uint8_t sequenceIndex, uint8_t trackObjId, uint8_t trackId, std::optional<uint8_t> bridge, CompanyId owner, uint8_t mods)
    {
        setType(Map::ElementType::track);
        setBaseZ(baseZ);
        setClearZ(clearZ);
        _type |= direction & 0x3;
        _flags = static_cast<ElementFlags>(quarterTile) & ElementFlags::invalid; //The flags only occupy the top 4 bits of quarterTile
        _4 = (trackId & 0x3F) | (bridge ? 0x80 : 0);
        _5 = (sequenceIndex & 0xF) | ((trackObjId & 0xF) << 4);
        _6 = bridge ? (*bridge << 5) : 0;
        _7 = enumValue(owner) | (mods << 4);
    }
}
