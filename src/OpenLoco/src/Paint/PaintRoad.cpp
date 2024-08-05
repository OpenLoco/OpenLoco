#include "PaintRoad.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Logging.h"
#include "Map/RoadElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Paint.h"
#include "PaintTileDecorations.h"
#include "Ui/ViewportInteraction.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Diagnostics;
namespace OpenLoco::Paint
{
    static Interop::loco_global<uint32_t, 0x0112C280> _roadBaseImageId;
    // static Interop::loco_global<uint32_t, 0x001135F2E> _trackExtraImageId;
    static Interop::loco_global<uint32_t, 0x01135F32> _roadImageId1;
    static Interop::loco_global<uint32_t, 0x01135F36> _roadImageId2;
    static Interop::loco_global<uint8_t, 0x00113605E> _roadTunnel;
    static Interop::loco_global<uint8_t, 0x00522095> _byte_522095;

    struct RoadPaintCommon
    {
        ImageId roadBaseImageId;          // 0x0112C280 with colours and image index set to base of roadObject image table
        ImageId bridgeColoursBaseImageId; // 0x01135F36 with only the colours set (image index not set!)
        uint8_t tunnelType;               // 0x0113605E
    };
    constexpr std::array<uint32_t, 4> kDefaultOneWayArrowImageIndexs{
        ImageIds::one_way_direction_arrow_left_lane_north_east,
        ImageIds::one_way_direction_arrow_left_lane_south_east,
        ImageIds::one_way_direction_arrow_right_lane_south_west,
        ImageIds::one_way_direction_arrow_right_lane_north_east,
    };
    constexpr std::array<uint32_t, 4> kSmallLeftCurveOneWayArrowImageIndexs{
        ImageIds::one_way_direction_arrow_right_lane_north_east,
        ImageIds::one_way_direction_arrow_left_lane_north_east,
        ImageIds::one_way_direction_arrow_left_lane_south_east,
        ImageIds::one_way_direction_arrow_right_lane_south_west,
    };
    constexpr std::array<uint32_t, 4> kSmallRightCurveOneWayArrowImageIndexs{
        ImageIds::one_way_direction_arrow_left_lane_south_east,
        ImageIds::one_way_direction_arrow_right_lane_south_west,
        ImageIds::one_way_direction_arrow_right_lane_north_east,
        ImageIds::one_way_direction_arrow_left_lane_north_east,
    };
    constexpr std::array<uint32_t, 4> kVerySmallLeftCurveOneWayArrowImageIndexs{
        ImageIds::one_way_direction_arrow_right_lane_north,
        ImageIds::one_way_direction_arrow_right_lane_west_2,
        ImageIds::one_way_direction_arrow_right_lane_south_2,
        ImageIds::one_way_direction_arrow_right_lane_west,
    };
    constexpr std::array<uint32_t, 4> kVerySmallRightCurveOneWayArrowImageIndexs{
        ImageIds::one_way_direction_arrow_left_lane_east,
        ImageIds::one_way_direction_arrow_left_lane_south,
        ImageIds::one_way_direction_arrow_left_lane_east_2,
        ImageIds::one_way_direction_arrow_left_lane_north_2,
    };
    constexpr std::array<std::array<std::array<uint32_t, 4>, 2>, 10> kOneWayArrowImageIndexs = {
        std::array<std::array<uint32_t, 4>, 2>{
            kDefaultOneWayArrowImageIndexs,
            kDefaultOneWayArrowImageIndexs,
        },
        std::array<std::array<uint32_t, 4>, 2>{
            kVerySmallLeftCurveOneWayArrowImageIndexs,
            kVerySmallLeftCurveOneWayArrowImageIndexs,
        },
        std::array<std::array<uint32_t, 4>, 2>{
            kVerySmallRightCurveOneWayArrowImageIndexs,
            kVerySmallRightCurveOneWayArrowImageIndexs,
        },
        std::array<std::array<uint32_t, 4>, 2>{
            kDefaultOneWayArrowImageIndexs,
            kSmallLeftCurveOneWayArrowImageIndexs,
        },
        std::array<std::array<uint32_t, 4>, 2>{
            kDefaultOneWayArrowImageIndexs,
            kSmallRightCurveOneWayArrowImageIndexs,
        },
        std::array<std::array<uint32_t, 4>, 2>{
            kDefaultOneWayArrowImageIndexs,
            kDefaultOneWayArrowImageIndexs,
        },
        std::array<std::array<uint32_t, 4>, 2>{
            kDefaultOneWayArrowImageIndexs,
            kDefaultOneWayArrowImageIndexs,
        },
        std::array<std::array<uint32_t, 4>, 2>{
            kDefaultOneWayArrowImageIndexs,
            kDefaultOneWayArrowImageIndexs,
        },
        std::array<std::array<uint32_t, 4>, 2>{
            kDefaultOneWayArrowImageIndexs,
            kDefaultOneWayArrowImageIndexs,
        },
        std::array<std::array<uint32_t, 4>, 2>{
            kDefaultOneWayArrowImageIndexs,
            kDefaultOneWayArrowImageIndexs,
        },
    };

    // 0x004759A6
    void paintRoad(PaintSession& session, const World::RoadElement& elRoad)
    {
        if (elRoad.isAiAllocated())
        {
            return;
        }
        if (elRoad.isGhost()
            && CompanyManager::getSecondaryPlayerId() != CompanyId::null
            && CompanyManager::getSecondaryPlayerId() == elRoad.owner())
        {
            return;
        }
        const auto height = elRoad.baseHeight();
        const auto rotation = (session.getRotation() + elRoad.rotation()) & 0x3;
        if (((session.getViewFlags() & Ui::ViewportFlags::height_marks_on_tracks_roads) != Ui::ViewportFlags::none)
            && session.getRenderTarget()->zoomLevel == 0)
        {
            const bool isLast = elRoad.isFlag6();
            const bool isFirstTile = elRoad.sequenceIndex() == 0;
            if (isFirstTile || isLast)
            {
                session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                const auto markerHeight = height + getRoadDecorationHeightOffset(isFirstTile, elRoad.roadId()) + 8;
                const auto imageId = ImageId{ getHeightMarkerImage(markerHeight), Colour::blue };
                const World::Pos3 offset(16, 16, markerHeight);
                const World::Pos3 bbOffset(1000, 1000, 1087);
                const World::Pos3 bbSize(1, 1, 0);
                session.addToPlotListAsParent(imageId, offset, bbOffset, bbSize);
            }
        }

        auto* roadObj = ObjectManager::get<RoadObject>(elRoad.roadObjectId());
        if (((session.getViewFlags() & Ui::ViewportFlags::one_way_direction_arrows) != Ui::ViewportFlags::none)
            && session.getRenderTarget()->zoomLevel == 0
            && !elRoad.isGhost()
            && roadObj->hasFlags(RoadObjectFlags::unk_00))
        {
            const bool isLast = elRoad.isFlag6();
            const bool isFirstTile = elRoad.sequenceIndex() == 0;
            if (isFirstTile || isLast)
            {
                session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                const auto markerHeight = height + getRoadDecorationHeightOffset(isFirstTile, elRoad.roadId()) + 8;
                const auto imageId = ImageId{ kOneWayArrowImageIndexs[elRoad.roadId()][isLast ? 1 : 0][rotation], Colour::mutedAvocadoGreen };
                const World::Pos3 offset(0, 0, markerHeight);
                const auto bbOffset = World::Pos3{ 15, 15, 16 } + World::Pos3(0, 0, markerHeight);
                const World::Pos3 bbSize(1, 1, 0);
                session.addToPlotListAsParent(imageId, offset, bbOffset, bbSize);
            }
        }

        session.setItemType(Ui::ViewportInteraction::InteractionItem::road);
        _roadBaseImageId = roadObj->image;
        _roadTunnel = roadObj->tunnel;

        _roadImageId1 = Gfx::recolour(0, CompanyManager::getCompanyColour(elRoad.owner()));
        _roadImageId2 = _roadImageId1;

        if (elRoad.isGhost())
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            _roadImageId1 = Gfx::applyGhostToImage(_roadImageId1).toUInt32();
            _roadImageId2 = Gfx::applyGhostToImage(_roadImageId2).toUInt32();
        }

        _roadBaseImageId |= _roadImageId1;
        RoadPaintCommon roadSession{ ImageId::fromUInt32(_roadBaseImageId), ImageId::fromUInt32(_roadImageId2), _roadTunnel };

        if (!(*_byte_522095 & (1 << 0)))
        {
            if (elRoad.roadId() < kRoadPaintParts.size() && elRoad.sequenceIndex() < kRoadPaintParts[elRoad.roadId()].size())
            {
                auto& parts = kRoadPaintParts[elRoad.roadId()];
                auto& tpp = parts[elRoad.sequenceIndex()];
                paintRoadPP(session, elRoad, roadSession, rotation, tpp);
            }
            else
            {
                assert(false);
                Logging::error("Tried to draw invalid track id or sequence index: TrackId {} SequenceIndex {}", elRoad.roadId(), elRoad.sequenceIndex());
            }
        }

        //if (session.getRenderTarget()->zoomLevel > 0)
        //{
        //    return;
        //}

        //session.setItemType(Ui::ViewportInteraction::InteractionItem::trackExtra);
        //const auto ghostMods = Ui::Windows::Construction::getLastSelectedMods();
        //for (auto mod = 0; mod < 4; ++mod)
        //{
        //    const auto* trackExtraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[mod]);
        //    if (elTrack.hasMod(mod))
        //    {
        //        _trackExtraImageId = _trackImageId1 + trackExtraObj->image;
        //    }
        //    else if (elTrack.hasGhostMods() && ghostMods & (1 << mod))
        //    {
        //        _trackExtraImageId = Gfx::applyGhostToImage(trackExtraObj->image).toUInt32();
        //    }
        //    else
        //    {
        //        continue;
        //    }
        //    const auto trackExtraBaseImage = ImageId::fromUInt32(_trackExtraImageId);

        //    session.setTrackModId(mod);

        //    const auto paintStyle = trackExtraObj->paintStyle;
        //    if (paintStyle == 0 && elTrack.trackId() < Style0::kTrackPaintAdditionParts.size() && elTrack.sequenceIndex() < Style0::kTrackPaintAdditionParts[elTrack.trackId()].size())
        //    {
        //        auto& parts = Style0::kTrackPaintAdditionParts[elTrack.trackId()];
        //        auto& tppa = parts[elTrack.sequenceIndex()];

        //        Style0::paintTrackAdditionPP(session, elTrack, rotation, trackExtraBaseImage, tppa);
        //    }
        //    else if (paintStyle == 1 && elTrack.trackId() < Style1::kTrackPaintAdditionParts.size() && elTrack.sequenceIndex() < Style1::kTrackPaintAdditionParts[elTrack.trackId()].size())
        //    {
        //        auto& parts = Style1::kTrackPaintAdditionParts[elTrack.trackId()];
        //        auto& tppa = parts[elTrack.sequenceIndex()];

        //        Style1::paintTrackAdditionPP(session, elTrack, rotation, trackExtraBaseImage, tppa);
        //    }
        //    else
        //    {
        //        assert(false);
        //        Logging::error("Tried to draw invalid track id or sequence index: TrackId {} SequenceIndex {}", elTrack.trackId(), elTrack.sequenceIndex());
        //    }
        //}
    }
}
