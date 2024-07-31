#include "Track.h"
#include "GameState.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "TrackData.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::World::Track
{
    void LegacyTrackConnections::push_back(uint16_t value)
    {
        if (size + 1 < std::size(data))
        {
            data[size++] = value;
            data[size] = 0xFFFF;
        }
    }

    static loco_global<uint8_t, 0x0112C2EE> _112C2EE;
    static loco_global<uint8_t, 0x0112C2ED> _112C2ED;
    static loco_global<StationId, 0x01135FAE> _1135FAE;
    static loco_global<uint16_t, 0x01136087> _1136087;
    static loco_global<uint8_t, 0x0113607D> _113607D;

    // Part of 0x00478895
    // For 0x00478895 call this followed by getRoadConnections
    ConnectionEnd getRoadConnectionEnd(const World::Pos3& pos, const uint16_t trackAndDirection)
    {
        const auto& roadData = TrackData::getUnkRoad(trackAndDirection);

        return ConnectionEnd{ pos + roadData.pos, roadData.rotationEnd };
    }

    // 0x004788C8
    RoadConnections getRoadConnections(const World::Pos3& nextTrackPos, const uint8_t nextRotation, const CompanyId company, const uint8_t roadObjectId, const uint8_t requiredMods, const uint8_t queryMods)
    {
        RoadConnections result{};

        uint8_t baseZ = nextTrackPos.z / 4;
        _112C2EE = nextRotation;

        const auto tile = World::TileManager::get(nextTrackPos);
        for (const auto& el : tile)
        {
            auto* elRoad = el.as<RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }

            if (!(getGameState().roadObjectIdIsFlag7 & (1 << elRoad->roadObjectId())))
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
                if (!(getGameState().roadObjectIdIsNotTram & (1 << elRoad->roadObjectId())))
                {
                    continue;
                }
            }

            if ((elRoad->mods() & requiredMods) != requiredMods)
            {
                continue;
            }

            if (elRoad->isGhost() || elRoad->isAiAllocated())
            {
                continue;
            }

            if (elRoad->sequenceIndex() == 0)
            {
                auto trackAndDirection2 = (elRoad->roadId() << 3) | elRoad->rotation();
                if (nextRotation == TrackData::getUnkRoad(trackAndDirection2).rotationBegin)
                {
                    const auto& roadPiece = TrackData::getRoadPiece(elRoad->roadId());
                    if (baseZ == (elRoad->baseZ() - roadPiece[0].z / 4))
                    {
                        if (elRoad->hasBridge())
                        {
                            trackAndDirection2 |= elRoad->bridge() << 9;
                            trackAndDirection2 |= AdditionalTaDFlags::hasBridge;
                        }

                        if ((queryMods & elRoad->mods()) != 0)
                        {
                            trackAndDirection2 |= AdditionalTaDFlags::hasMods;
                        }

                        if (elRoad->hasStationElement())
                        {
                            auto* elStation = tile.roadStation(elRoad->roadId(), elRoad->rotation(), elRoad->baseZ());
                            if (elStation == nullptr)
                            {
                                continue;
                            }

                            if (!elStation->isAiAllocated() && !elStation->isGhost())
                            {
                                result.stationId = elStation->stationId();
                                result.stationObjectId = elStation->objectId();
                            }
                        }

                        result.roadObjectId = elRoad->roadObjectId();
                        result.connections.push_back(trackAndDirection2);
                    }
                }
            }

            if (!elRoad->isFlag6())
            {
                continue;
            }

            auto trackAndDirection2 = (elRoad->roadId() << 3) | (1 << 2) | elRoad->rotation();
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
                trackAndDirection2 |= AdditionalTaDFlags::hasBridge;
            }

            if ((queryMods & elRoad->mods()) != 0)
            {
                trackAndDirection2 |= AdditionalTaDFlags::hasMods;
            }

            if (elRoad->hasStationElement())
            {
                auto* elStation = tile.roadStation(elRoad->roadId(), elRoad->rotation(), elRoad->baseZ());
                if (elStation == nullptr)
                {
                    continue;
                }

                if (!elStation->isAiAllocated() && !elStation->isGhost())
                {
                    result.stationId = elStation->stationId();
                    result.stationObjectId = elStation->objectId();
                }
            }
            result.connections.push_back(trackAndDirection2);
        }
        return result;
    }

    // Part of 0x004A2604
    // For 0x004A2604 call this followed by getTrackConnections
    ConnectionEnd getTrackConnectionEnd(const World::Pos3& pos, const uint16_t trackAndDirection)
    {
        const auto& trackData = TrackData::getUnkTrack(trackAndDirection);

        return ConnectionEnd{ pos + trackData.pos, trackData.rotationEnd };
    }

    void toLegacyConnections(const TrackConnections& src, LegacyTrackConnections& data)
    {
        for (auto& c : src.connections)
        {
            data.push_back(c);
        }
        _1135FAE = src.stationId;
        _113607D = src.hasLevelCrossing ? 1 : 0;
    }

    void toLegacyConnections(const RoadConnections& src, LegacyTrackConnections& data)
    {
        for (auto& c : src.connections)
        {
            data.push_back(c);
        }
        _1135FAE = src.stationId;
        _112C2ED = src.roadObjectId;
        _1136087 = src.stationObjectId;
    }

    constexpr uint16_t kNullTad = 0xFFFFU;

    static uint16_t getElTrackConnection(const World::TrackElement& elTrack, uint8_t nextRotation, uint8_t baseZ, uint8_t queryMods)
    {
        if (elTrack.sequenceIndex() == 0)
        {
            auto trackAndDirection2 = (elTrack.trackId() << 3) | elTrack.rotation();
            if (nextRotation == TrackData::getUnkTrack(trackAndDirection2).rotationBegin)
            {
                const auto& trackPiece = TrackData::getTrackPiece(elTrack.trackId());
                if (baseZ == (elTrack.baseZ() - trackPiece[0].z / 4))
                {
                    if (elTrack.hasBridge())
                    {
                        trackAndDirection2 |= elTrack.bridge() << 9;
                        trackAndDirection2 |= AdditionalTaDFlags::hasBridge;
                    }

                    if ((queryMods & elTrack.mods()) != 0)
                    {
                        trackAndDirection2 |= AdditionalTaDFlags::hasMods;
                    }

                    if (elTrack.hasSignal())
                    {
                        auto* elSignal = elTrack.next()->as<SignalElement>();
                        if (elSignal != nullptr)
                        {
                            if (!elSignal->isAiAllocated() && !elSignal->isGhost())
                            {
                                trackAndDirection2 |= (1 << 15);
                            }
                        }
                    }
                    return trackAndDirection2;
                }
            }
        }

        if (!elTrack.isFlag6())
        {
            return kNullTad;
        }

        auto trackAndDirection2 = (elTrack.trackId() << 3) | (1 << 2) | elTrack.rotation();
        if (nextRotation != TrackData::getUnkTrack(trackAndDirection2).rotationBegin)
        {
            return kNullTad;
        }

        const auto previousBaseZ = elTrack.baseZ() - (TrackData::getTrackPiece(elTrack.trackId())[elTrack.sequenceIndex()].z + TrackData::getUnkTrack(trackAndDirection2).pos.z) / 4;
        if (previousBaseZ != baseZ)
        {
            return kNullTad;
        }

        if (elTrack.hasBridge())
        {
            trackAndDirection2 |= elTrack.bridge() << 9;
            trackAndDirection2 |= AdditionalTaDFlags::hasBridge;
        }

        if ((queryMods & elTrack.mods()) != 0)
        {
            trackAndDirection2 |= AdditionalTaDFlags::hasMods;
        }

        if (elTrack.hasSignal())
        {
            auto* elSignal = elTrack.next()->as<SignalElement>();
            if (elSignal != nullptr)
            {
                if (!elSignal->isAiAllocated() && !elSignal->isGhost())
                {
                    trackAndDirection2 |= (1 << 15);
                }
            }
        }
        return trackAndDirection2;
    }

    // 0x004A2638, 0x004A2601
    TrackConnections getTrackConnections(const World::Pos3& nextTrackPos, const uint8_t nextRotation, const CompanyId company, const uint8_t trackObjectId, const uint8_t requiredMods, const uint8_t queryMods)
    {
        TrackConnections result{};

        uint8_t baseZ = nextTrackPos.z / 4;

        const auto tile = World::TileManager::get(nextTrackPos);
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

            if ((elTrack->mods() & requiredMods) != requiredMods)
            {
                continue;
            }

            if (elTrack->isGhost() || elTrack->isAiAllocated())
            {
                continue;
            }

            const auto connection = getElTrackConnection(*elTrack, nextRotation, baseZ, queryMods);
            if (connection == kNullTad)
            {
                continue;
            }

            result.connections.push_back(connection);
            if (elTrack->hasStationElement())
            {
                auto* elStation = elTrack->next()->as<StationElement>();
                if (elStation != nullptr)
                {
                    if (!elStation->isAiAllocated() && !elStation->isGhost())
                    {
                        result.stationId = elStation->stationId();
                    }
                }
            }

            if (elTrack->hasLevelCrossing())
            {
                result.hasLevelCrossing = 1;
            }
        }
        return result;
    }

    // 0x004A2820, 0x004A2854
    // For 0x004A2820 call getTrackConnectionEnd first then this
    TrackConnections getTrackConnectionsAi(const World::Pos3& nextTrackPos, const uint8_t nextRotation, const CompanyId company, const uint8_t trackObjectId, const uint8_t requiredMods, const uint8_t queryMods)
    {
        // Same as getTrackConnections but for aiAllocated preview track
        // i.e. can find connections on track that isn't visible yet.
        TrackConnections result{};

        uint8_t baseZ = nextTrackPos.z / 4;

        const auto tile = World::TileManager::get(nextTrackPos);
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

            if ((elTrack->mods() & requiredMods) != requiredMods)
            {
                continue;
            }

            if (!elTrack->isAiAllocated())
            {
                continue;
            }

            const auto connection = getElTrackConnection(*elTrack, nextRotation, baseZ, queryMods);
            if (connection == kNullTad)
            {
                continue;
            }

            result.connections.push_back(connection);
            if (elTrack->hasStationElement())
            {
                auto* elStation = elTrack->next()->as<StationElement>();
                if (elStation != nullptr)
                {
                    // No need to consider aiAllocated or ghost flags
                    result.stationId = elStation->stationId();
                }
            }

            if (elTrack->hasLevelCrossing())
            {
                result.hasLevelCrossing = 1;
            }
        }
        return result;
    }
}

namespace OpenLoco::World
{
    TrackElement::TrackElement(World::SmallZ baseZ, World::SmallZ clearZ, uint8_t direction, uint8_t quarterTile, uint8_t sequenceIndex, uint8_t trackObjId, uint8_t trackId, std::optional<uint8_t> bridge, CompanyId owner, uint8_t mods)
    {
        setType(World::ElementType::track);
        setBaseZ(baseZ);
        setClearZ(clearZ);
        _type |= direction & 0x3;
        _flags = quarterTile & 0xF;
        _4 = (trackId & 0x3F) | (bridge ? 0x80 : 0);
        _5 = (sequenceIndex & 0xF) | ((trackObjId & 0xF) << 4);
        _6 = bridge ? (*bridge << 5) : 0;
        _7 = enumValue(owner) | (mods << 4);
    }
}
