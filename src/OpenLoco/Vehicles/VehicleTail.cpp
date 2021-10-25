#include "../Map/AnimationManager.h"
#include "../Map/TileManager.h"
#include "../Map/Track/TrackData.h"
#include "../ViewportManager.h"
#include "Vehicle.h"
#include "VehicleManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    static loco_global<int32_t, 0x0113612C> vehicleUpdate_var_113612C; // Speed
    static loco_global<uint32_t, 0x01136114> vehicleUpdate_var_1136114;
    static loco_global<Map::Pos2[16], 0x00503C6C> _503C6C;

    // 0x0048963F
    static uint8_t sub_48963F(const Map::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint8_t trackType, uint32_t flags)
    {
        const auto unk1 = flags & 0xFFFF;
        auto trackStart = loc;
        if (trackAndDirection.isReversed())
        {
            auto& trackSize = Map::TrackData::getUnkTrack(trackAndDirection._data);
            trackStart += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                trackStart -= Map::Pos3{ _503C6C[trackSize.rotationEnd] };
            }
            flags ^= (1ULL << 31);
        }

        auto& trackPieces = Map::TrackData::getTrackPiece(trackAndDirection.id());
        for (auto& trackPiece : trackPieces)
        {
            auto signalLoc = trackStart + Map::Pos3{ Math::Vector::rotate(Map::Pos2{ trackPiece.x, trackPiece.y }, trackAndDirection.cardinalDirection()) };
            signalLoc.z += trackPiece.z;
            auto tile = Map::TileManager::get(signalLoc);
            Map::TrackElement* foundTrack = nullptr;
            for (auto& el : tile)
            {
                if (el.baseZ() != signalLoc.z / 4)
                {
                    continue;
                }

                auto* elTrack = el.as<TrackElement>();
                if (elTrack == nullptr)
                {
                    continue;
                }

                if (!elTrack->hasSignal())
                {
                    continue;
                }

                if (elTrack->unkDirection() != trackAndDirection.cardinalDirection())
                {
                    continue;
                }

                if (elTrack->sequenceIndex() != trackPiece.index)
                {
                    continue;
                }

                if (elTrack->trackObjectId() != trackType)
                {
                    continue;
                }

                if (elTrack->trackId() != trackAndDirection.id())
                {
                    continue;
                }
                foundTrack = elTrack;
                break;
            }

            if (foundTrack == nullptr)
            {
                if (unk1 == 10)
                {
                    return 0;
                }
                return loc.x; // Odd???
            }

            auto* elSignal = foundTrack->next()->as<SignalElement>();
            if (elSignal == nullptr)
            {
                continue;
            }
            // edx
            auto& signalSide = (flags & (1ULL << 31)) ? elSignal->getRight() : elSignal->getLeft();
            if (unk1 == 16)
            {
                uint8_t lightStates = 0;
                if (flags & (1ULL << 30))
                {
                    lightStates |= 1 << 2;
                    if (flags & (1ULL << 28))
                    {
                        lightStates |= 1 << 0;
                    }
                }
                if (flags & (1ULL << 29))
                {
                    lightStates |= 1 << 3;
                    if (flags & (1ULL << 27))
                    {
                        lightStates |= 1 << 1;
                    }
                }

                if (signalSide.allLights() != lightStates)
                {
                    bool shouldInvalidate = false;
                    signalSide.setAllLights(lightStates);
                    if (flags & (1ULL << 31))
                    {
                        shouldInvalidate = foundTrack->sequenceIndex() == 0;
                    }
                    else
                    {
                        shouldInvalidate = foundTrack->isFlag6();
                    }

                    if (shouldInvalidate)
                    {
                        Ui::ViewportManager::invalidate(signalLoc, signalLoc.z, signalLoc.z + 24, ZoomLevel::half);
                    }
                }
            }
            else if (unk1 == 10)
            {
                uint8_t ret = 0;
                if (signalSide.hasUnk4_40())
                {
                    ret |= (1 << 0);
                }
                if (signalSide.hasSignal())
                {
                    ret |= (1 << 1);
                }
                if (flags & (1ULL << 31))
                {
                    if (!elSignal->getLeft().hasSignal() || elSignal->isLeftGhost())
                    {
                        ret |= (1 << 2);
                    }

                    if (elSignal->isRightGhost() && elSignal->getLeft().hasSignal())
                    {
                        ret |= (1 << 1);
                    }
                }
                else
                {
                    if (!elSignal->getRight().hasSignal() || elSignal->isRightGhost())
                    {
                        ret |= (1 << 2);
                    }

                    if (elSignal->isLeftGhost() && elSignal->getRight().hasSignal())
                    {
                        ret |= (1 << 1);
                    }
                }
                return ret;
            }
            else if (unk1 == 8)
            {
                signalSide.setUnk4_40(true);
            }
            else if (unk1 > 8)
            {
                signalSide.setUnk4_40(false);
            }
            else
            {
                signalSide.setUnk4(unk1 & 0x3);
                if ((unk1 & 0x3) == 0)
                {
                    // Clear lights 0b1100_0000
                    signalSide.setAllLights(signalSide.allLights() & 0xC);
                }
                bool animate = false;
                if (flags & (1ULL << 31))
                {
                    if (foundTrack->isFlag6())
                    {
                        animate = true;
                    }
                }
                else
                {
                    if (foundTrack->sequenceIndex() == 0)
                    {
                        animate = true;
                    }
                }
                if (animate)
                {
                    Map::AnimationManager::createAnimation(0, signalLoc, signalLoc.z / 4);
                    Ui::ViewportManager::invalidate(signalLoc, signalLoc.z, signalLoc.z + 24, ZoomLevel::half);
                }
            }
        }
        return loc.x; // Odd???
    }

    // 0x004A2AD7
    static void sub_4A2AD7(const Map::Pos3& loc, const TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType)
    {
        addr<0x001135F88, uint16_t>() = 0;
        registers regs;
        regs.ax = loc.x;
        regs.cx = loc.y;
        regs.dx = loc.z;
        regs.bl = enumValue(company);
        regs.bh = trackType;
        regs.ebp = trackAndDirection.track._data;
        regs.esi = 0x004A2AF0;
        regs.edi = 0x004A2CE7;
        call(0x004A2E46, regs);
    }

    // 0x004794BC
    static void leaveLevelCrossing(const Map::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint16_t unk)
    {
        auto levelCrossingLoc = loc;
        if (trackAndDirection.isReversed())
        {
            auto& trackSize = Map::TrackData::getUnkTrack(trackAndDirection._data);
            levelCrossingLoc += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                levelCrossingLoc -= Map::Pos3{ _503C6C[trackSize.rotationEnd], 0 };
            }
        }

        auto& trackPiece = Map::TrackData::getTrackPiece(trackAndDirection.id());
        levelCrossingLoc += Map::Pos3{ Math::Vector::rotate(Map::Pos2{ trackPiece[0].x, trackPiece[0].y }, trackAndDirection.cardinalDirection()), 0 };
        levelCrossingLoc.z += trackPiece[0].z;
        auto tile = Map::TileManager::get(levelCrossingLoc);
        for (auto& el : tile)
        {
            if (el.baseZ() != levelCrossingLoc.z / 4)
            {
                continue;
            }

            auto* road = el.as<RoadElement>();
            if (road == nullptr)
            {
                continue;
            }

            if (road->roadId() != 0)
            {
                continue;
            }

            road->setUnk7_10(false);
            if (unk != 8)
            {
                continue;
            }

            Map::AnimationManager::createAnimation(1, levelCrossingLoc, levelCrossingLoc.z / 4);
        }

        Ui::ViewportManager::invalidate(levelCrossingLoc, levelCrossingLoc.z, levelCrossingLoc.z + 32, ZoomLevel::full);
    }

    // 0x004AA24A
    bool VehicleTail::update()
    {
        if (mode == TransportMode::air || mode == TransportMode::water)
        {
            return true;
        }

        const auto _oldRoutingHandle = routingHandle;
        const Map::Pos3 _oldTilePos = Map::Pos3(tile_x, tile_y, tile_base_z * 4);

        vehicleUpdate_var_1136114 = 0;
        sub_4B15FF(*vehicleUpdate_var_113612C);

        if (*vehicleUpdate_var_1136114 & (1 << 1))
        {
            sub_4AA464();
            return false;
        }

        if (_oldRoutingHandle == routingHandle)
        {
            return true;
        }

        const auto ref = RoutingManager::getRouting(_oldRoutingHandle);
        TrackAndDirection trackAndDirection((ref & 0x1F8) >> 3, ref & 0x7);
        RoutingManager::freeRouting(_oldRoutingHandle);

        if (mode == TransportMode::road)
        {
            sub_47D959(_oldTilePos, trackAndDirection.road, false);
        }
        else
        {
            if (ref & (1 << 15))
            {
                // Update signal state?
                sub_48963F(_oldTilePos, trackAndDirection.track, track_type, 0);
            }

            const auto& trackSize = Map::TrackData::getUnkTrack(ref & 0x1FF);
            auto nextTile = _oldTilePos + trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                nextTile -= Map::Pos3{ _503C6C[trackSize.rotationEnd], 0 };
            }
            auto trackAndDirection2 = trackAndDirection;
            trackAndDirection2.track.setReversed(!trackAndDirection2.track.isReversed());
            sub_4A2AD7(nextTile, trackAndDirection2, owner, track_type);
            leaveLevelCrossing(_oldTilePos, trackAndDirection.track, 9);
        }
        return true;
    }
}
