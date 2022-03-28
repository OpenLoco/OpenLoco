#include "PaintSignal.h"
#include "../CompanyManager.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Map/Map.hpp"
#include "../Map/Tile.h"
#include "../Map/Track/TrackData.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/TrainSignalObject.h"
#include "../Ui.h"
#include "Paint.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    struct OffsetAndBBOffset
    {
        Map::Pos2 offset;
        Map::Pos2 boundingOffset;
    };
    // clang-format off
    static constexpr std::array<OffsetAndBBOffset, 16> _4FE830 = {
        OffsetAndBBOffset{ { 24, 4, },  { 24, 4, } },
        OffsetAndBBOffset{ { 4, 8, },   { 4, 8, } },
        OffsetAndBBOffset{ { 8, 28, },  { 8, 28, } },
        OffsetAndBBOffset{ { 28, 24, }, { 28, 24, } },
        OffsetAndBBOffset{ { 24, 1, },  { 24, 1, } },
        OffsetAndBBOffset{ { 1, 8, },   { 1, 8, } },
        OffsetAndBBOffset{ { 8, 31, },  { 8, 31, } },
        OffsetAndBBOffset{ { 31, 24, }, { 31, 24, } },
        OffsetAndBBOffset{ { 24, 16, }, { 24, 16, } },
        OffsetAndBBOffset{ { 16, 8, },  { 16, 8, } },
        OffsetAndBBOffset{ { 8, 16, },  { 8, 16, } },
        OffsetAndBBOffset{ { 16, 24, }, { 16, 24, } },
        OffsetAndBBOffset{ { 6, 10, },  { 0, 0, } },
        OffsetAndBBOffset{ { 10, 26, }, { 10, 26, } },
        OffsetAndBBOffset{ { 26, 22, }, { 26, 22, } },
        OffsetAndBBOffset{ { 22, 6, },  { 22, 6, } },
    };

    static constexpr std::array<OffsetAndBBOffset, 16> _4FE870 = {
        OffsetAndBBOffset{ { 24, 28, }, { 24, 28, } },
        OffsetAndBBOffset{ { 28, 8, },  { 28, 8, } },
        OffsetAndBBOffset{ { 8, 4, },   { 8, 4, } },
        OffsetAndBBOffset{ { 4, 24, },  { 4, 24, } },
        OffsetAndBBOffset{ { 24, 16, }, { 24, 16, } },
        OffsetAndBBOffset{ { 16, 8, },  { 16, 8, } },
        OffsetAndBBOffset{ { 8, 16, },  { 8, 16, } },
        OffsetAndBBOffset{ { 16, 24, }, { 16, 24, } },
        OffsetAndBBOffset{ { 24, 31, }, { 24, 31, } },
        OffsetAndBBOffset{ { 31, 8, },  { 31, 8, } },
        OffsetAndBBOffset{ { 8, 1, },   { 8, 1, } },
        OffsetAndBBOffset{ { 31, 24, }, { 31, 24, } },
        OffsetAndBBOffset{ { 22, 26, }, { 22, 26, } },
        OffsetAndBBOffset{ { 26, 10, }, { 26, 10, } },
        OffsetAndBBOffset{ { 10, 6, },  { 0, 0, } },
        OffsetAndBBOffset{ { 6, 22, },  { 6, 22, } },
    };
    // clang-format on

    // 0x004FE8B0
    static constexpr std::array<uint32_t, 16> kOneWayArrowLeft = {
        ImageIds::one_way_direction_arrow_north,
        ImageIds::one_way_direction_arrow_east,
        ImageIds::one_way_direction_arrow_south,
        ImageIds::one_way_direction_arrow_west,
        ImageIds::one_way_direction_arrow_north,
        ImageIds::one_way_direction_arrow_east,
        ImageIds::one_way_direction_arrow_south,
        ImageIds::one_way_direction_arrow_west,
        ImageIds::one_way_direction_arrow_north,
        ImageIds::one_way_direction_arrow_east,
        ImageIds::one_way_direction_arrow_south,
        ImageIds::one_way_direction_arrow_west,
        ImageIds::one_way_direction_arrow_north_east,
        ImageIds::one_way_direction_arrow_south_east,
        ImageIds::one_way_direction_arrow_south_west,
        ImageIds::one_way_direction_arrow_north_west,
    };
    // 0x004FE8B0
    static constexpr std::array<uint32_t, 16> kOneWayArrowRight = {
        ImageIds::one_way_direction_arrow_south,
        ImageIds::one_way_direction_arrow_west,
        ImageIds::one_way_direction_arrow_north,
        ImageIds::one_way_direction_arrow_east,
        ImageIds::one_way_direction_arrow_south,
        ImageIds::one_way_direction_arrow_west,
        ImageIds::one_way_direction_arrow_north,
        ImageIds::one_way_direction_arrow_east,
        ImageIds::one_way_direction_arrow_south,
        ImageIds::one_way_direction_arrow_west,
        ImageIds::one_way_direction_arrow_north,
        ImageIds::one_way_direction_arrow_east,
        ImageIds::one_way_direction_arrow_south_west,
        ImageIds::one_way_direction_arrow_north_west,
        ImageIds::one_way_direction_arrow_north_east,
        ImageIds::one_way_direction_arrow_south_east,
    };

    static loco_global<uint8_t[8 * 44], 0x004F87BC> _4F87BC;
    static loco_global<int8_t[2 * 44], 0x004F86B4> _4F86B4;

    static uint8_t getTrackRotation(const bool isRight, const uint8_t trackId, const uint8_t rotation)
    {
        if (isRight)
        {
            return Map::TrackData::getUnkTrack((trackId << 3) | (rotation + 4)).rotationBegin;
        }
        else
        {
            return Map::TrackData::getUnkTrack((trackId << 3) | rotation).rotationBegin;
        }
    }

    static uint8_t getSignalHeightOffset(const bool isRight, const uint8_t trackId)
    {
        if (isRight)
        {
            return _4F87BC[trackId * 8 + 3];
        }
        else
        {
            return _4F87BC[trackId * 8 + 2];
        }
    }

    static int8_t getOneWayArrowHeightOffset(const bool isRight, const uint8_t trackId)
    {
        if (isRight)
        {
            return _4F86B4[trackId * 2];
        }
        else
        {
            return _4F86B4[trackId * 2 + 1];
        }
    }

    static uint32_t getOneWayArrowImage(const bool isRight, const uint8_t trackId, const uint8_t rotation)
    {
        const auto& trackCoordinates = Map::TrackData::getUnkTrack((trackId << 3) | rotation);
        if (isRight)
        {
            return kOneWayArrowRight[trackCoordinates.rotationBegin];
        }
        else
        {
            return kOneWayArrowLeft[trackCoordinates.rotationEnd];
        }
    }

    static void paintSignalSide(PaintSession& session, const Map::SignalElement::Side& side, const bool isRight, const bool isGhost, const uint8_t trackId, const uint8_t rotation, const coord_t height)
    {
        if (side.hasSignal())
        {
            session.setItemType(InteractionItem::signal);
            session.setTrackModId(isRight ? 1 : 0);
            auto* signalObj = ObjectManager::get<TrainSignalObject>(side.signalObjectId());
            const auto trackRotation = getTrackRotation(isRight, trackId, rotation);
            const auto& offsetAndBBoffsetArr = (signalObj->flags & TrainSignalObjectFlags::isLeft) ? _4FE870 : _4FE830;
            const auto& offsetAndBBoffset = offsetAndBBoffsetArr[trackRotation];
            const auto imageRotationOffset = ((trackRotation & 0x3) << 1) | (trackRotation >= 12 ? 1 : 0);
            const auto imageOffset = imageRotationOffset + signalObj->image + (side.frame() << 3);

            uint32_t imageId = imageOffset;
            if (isGhost)
            {
                session.setItemType(InteractionItem::noInteraction);
                imageId = Gfx::applyGhostToImage(imageOffset);
            }
            Map::Pos3 offset(offsetAndBBoffset.offset.x, offsetAndBBoffset.offset.y, getSignalHeightOffset(isRight, trackId) + height);
            Map::Pos3 bbOffset(offsetAndBBoffset.boundingOffset.x, offsetAndBBoffset.boundingOffset.y, offset.z + 4);
            Map::Pos3 bbSize(1, 1, 14);
            session.addToPlotListAsParent(imageId, offset, bbOffset, bbSize);

            if (signalObj->flags & TrainSignalObjectFlags::hasLights)
            {
                if (side.hasRedLight())
                {
                    const auto lightOffset = side.hasRedLight2() ? TrainSignal::ImageIds::redLights2 : TrainSignal::ImageIds::redLights;
                    const auto lightImageOffset = imageRotationOffset + signalObj->image + lightOffset;
                    imageId = lightImageOffset;
                    if (isGhost)
                    {
                        session.setItemType(InteractionItem::noInteraction);
                        imageId = Gfx::applyGhostToImage(imageOffset);
                    }
                    session.addToPlotList4FD1E0(imageId, offset, bbOffset, bbSize);
                }
                if (side.hasGreenLight())
                {
                    const auto lightOffset = side.hasGreenLight2() ? TrainSignal::ImageIds::greenLights2 : TrainSignal::ImageIds::greenLights;
                    const auto lightImageOffset = imageRotationOffset + signalObj->image + lightOffset;
                    imageId = lightImageOffset;
                    if (isGhost)
                    {
                        session.setItemType(InteractionItem::noInteraction);
                        imageId = Gfx::applyGhostToImage(imageOffset);
                    }
                    session.addToPlotList4FD1E0(imageId, offset, bbOffset, bbSize);
                }
            }
        }
        else
        {
            if (session.getViewFlags() & (1 << 4) && session.getContext()->zoom_level == 0)
            {
                session.setItemType(InteractionItem::noInteraction);
                const auto imageId = Gfx::recolour(getOneWayArrowImage(!isRight, trackId, rotation), Colour::mutedAvocadoGreen);
                const Map::Pos3 offset(0, 0, height + getOneWayArrowHeightOffset(!isRight, trackId) + 2);
                const Map::Pos3 bbOffset(15, 15, offset.z + 16);
                const Map::Pos3 bbSize(1, 1, 0);
                session.addToPlotListAsParent(imageId, offset, bbOffset, bbSize);
            }
        }
    }

    // 0x0048864C
    void paintSignal(PaintSession& session, const Map::SignalElement& elSignal)
    {
        if (elSignal.isFlag5())
        {
            return;
        }

        auto* elTrack = elSignal.prev()->as<Map::TrackElement>();
        if (elTrack == nullptr)
        {
            return;
        }
        if (elSignal.isGhost()
            && CompanyManager::getSecondaryPlayerId() != CompanyId::null
            && elTrack->owner() == CompanyManager::getSecondaryPlayerId())
        {
            return;
        }

        if (session.getContext()->zoom_level > 1)
        {
            return;
        }

        const coord_t height = elSignal.baseZ() * 4;
        const auto trackId = elTrack->trackId();
        const uint8_t rotation = (session.getRotation() + elSignal.rotation()) & 0x3;
        if (elTrack->sequenceIndex() == 0)
        {
            auto& leftSignal = elSignal.getLeft();
            const bool leftIsGhost = elSignal.isGhost() || elSignal.isLeftGhost();
            paintSignalSide(session, leftSignal, false, leftIsGhost, trackId, rotation, height);
        }
        if (!elTrack->isFlag6())
        {
            return;
        }

        auto& rightSignal = elSignal.getRight();
        const bool rightIsGhost = elSignal.isGhost() || elSignal.isRightGhost();
        paintSignalSide(session, rightSignal, true, rightIsGhost, trackId, rotation, height);
    }
}
