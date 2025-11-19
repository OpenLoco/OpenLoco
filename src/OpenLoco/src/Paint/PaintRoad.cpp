#include "PaintRoad.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Logging.h"
#include "Map/RoadElement.h"
#include "Objects/LevelCrossingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/StreetLightObject.h"
#include "Paint.h"
#include "PaintRoadAdditionsData.h"
#include "PaintRoadCommonData.h"
#include "PaintRoadStyle0Data.h"
#include "PaintRoadStyle1Data.h"
#include "PaintRoadStyle2Data.h"
#include "PaintTileDecorations.h"
#include "ScenarioManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Diagnostics;
namespace OpenLoco::Paint
{
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

    // 0x00475DDF
    static void paintLevelCrossing(PaintSession& session, const ImageId baseRoadImageColour, const World::RoadElement& elRoad, const uint8_t rotation)
    {
        auto* crossingObj = ObjectManager::get<LevelCrossingObject>(elRoad.levelCrossingObjectId());

        uint8_t frame = elRoad.unk6l();
        if (frame != 0)
        {
            if (frame == 15)
            {
                frame = (((ScenarioManager::getScenarioTicks() / (1U << crossingObj->animationSpeed)) & (crossingObj->closingFrames - 1)) + crossingObj->closedFrames + 1);
            }
        }

        uint32_t imageIndex0 = crossingObj->image + ((rotation & 1) * 4) + (frame * 8);

        const auto height = elRoad.baseHeight();
        const auto heightOffset = World::Pos3{ 0,
                                               0,
                                               height };

        {
            const auto bbOffset = World::Pos3{ 2, 2, 1 } + heightOffset;
            const auto bbSize = World::Pos3{ 1, 1, 8 };
            const auto image0 = baseRoadImageColour.withIndex(imageIndex0);
            session.addToPlotList4FD150(image0, heightOffset, bbOffset, bbSize);
        }
        {
            const auto bbOffset = World::Pos3{ 2, 30, 1 } + heightOffset;
            const auto bbSize = World::Pos3{ 1, 1, 8 };
            const auto image1 = baseRoadImageColour.withIndex(imageIndex0 + 1);
            session.addToPlotList4FD150(image1, heightOffset, bbOffset, bbSize);
        }
        {
            const auto bbOffset = World::Pos3{ 30, 2, 1 } + heightOffset;
            const auto bbSize = World::Pos3{ 1, 1, 8 };
            const auto image2 = baseRoadImageColour.withIndex(imageIndex0 + 2);
            session.addToPlotList4FD150(image2, heightOffset, bbOffset, bbSize);
        }

        const auto image3 = baseRoadImageColour.withIndex(imageIndex0 + 3);
        if (elRoad.unk6l() != 15)
        {
            const auto bbOffset = World::Pos3{ 30, 30, 1 } + heightOffset;
            const auto bbSize = World::Pos3{ 1, 1, 8 };
            session.addToPlotList4FD150(image3, heightOffset, bbOffset, bbSize);
        }
        else
        {
            if (rotation & (1U << 0))
            {
                const auto bbOffset = World::Pos3{ 17, 30, 1 } + heightOffset;
                const auto bbSize = World::Pos3{ 14, 1, 8 };
                session.addToPlotList4FD150(image3, heightOffset, bbOffset, bbSize);
            }
            else
            {
                const auto bbOffset = World::Pos3{ 30, 17, 1 } + heightOffset;
                const auto bbSize = World::Pos3{ 1, 14, 8 };
                session.addToPlotList4FD150(image3, heightOffset, bbOffset, bbSize);
            }
        }
    }

    constexpr std::array<std::array<uint32_t, 4>, 3> kStreetlightImageFromStyle = {
        std::array<uint32_t, 4>{
            Streetlight::ImageIds::kStyle0NE,
            Streetlight::ImageIds::kStyle0SE,
            Streetlight::ImageIds::kStyle0SW,
            Streetlight::ImageIds::kStyle0NW,
        },
        std::array<uint32_t, 4>{
            Streetlight::ImageIds::kStyle1NE,
            Streetlight::ImageIds::kStyle1SE,
            Streetlight::ImageIds::kStyle1SW,
            Streetlight::ImageIds::kStyle1NW,
        },
        std::array<uint32_t, 4>{
            Streetlight::ImageIds::kStyle2NE,
            Streetlight::ImageIds::kStyle2SE,
            Streetlight::ImageIds::kStyle2SW,
            Streetlight::ImageIds::kStyle2NW,
        },
    };

    constexpr std::array<World::Pos3, 4> kStreetlightOffsets = {
        World::Pos3{ 2, 15, 0 },
        World::Pos3{ 15, 28, 0 },
        World::Pos3{ 28, 15, 0 },
        World::Pos3{ 15, 2, 0 },
    };
    constexpr std::array<World::Pos3, 4> kStreetlightBoundingBoxOffsets = {
        World::Pos3{ 2, 15, 6 },
        World::Pos3{ 15, 28, 6 },
        World::Pos3{ 28, 15, 6 },
        World::Pos3{ 15, 2, 6 },
    };
    constexpr auto kStreetlightBoundingBoxSizes = World::Pos3{ 1, 1, 6 };

    static void paintRoadStreetlight(PaintSession& session, const int16_t baseHeight, const uint8_t style, const int16_t streetlightHeight, const uint8_t rotation)
    {
        assert(style < static_cast<int32_t>(kStreetlightImageFromStyle.size()));
        const auto imageIndexOffset = kStreetlightImageFromStyle[style][rotation];

        const auto& streetlightObj = ObjectManager::get<StreetLightObject>();

        const int16_t height = baseHeight + streetlightHeight;
        const auto heightOffset = World::Pos3{ 0,
                                               0,
                                               height };

        session.addToPlotListAsParent(
            ImageId(streetlightObj->image).withIndexOffset(imageIndexOffset),
            heightOffset + kStreetlightOffsets[rotation],
            heightOffset + kStreetlightBoundingBoxOffsets[rotation],
            kStreetlightBoundingBoxSizes);
    }

    static void paintRoadStreetlights(PaintSession& session, const World::RoadElement& elRoad, const std::array<int16_t, 4>& streetlightHeights)
    {
        auto r = 0;

        const auto style = elRoad.streetLightStyle() - 1;
        for (auto& height : streetlightHeights)
        {
            if (height != kNoStreetlight)
            {
                paintRoadStreetlight(session, elRoad.baseHeight(), style, height, r);
            }
            r++;
        }
    }

    namespace AdditionStyle1
    {
        static void paintSupport(PaintSession& session, const RoadAdditionSupport& tppaSupport, const uint8_t rotation, const ImageId baseImageId, int16_t height)
        {
            TrackRoadAdditionSupports support{};
            support.height = height + tppaSupport.height;
            support.occupiedSegments = session.getOccupiedAdditionSupportSegments();
            uint8_t i = 0;
            auto segments = enumValue(tppaSupport.segments[rotation]);
            for (auto seg = Numerics::bitScanForward(segments); seg != -1; seg = Numerics::bitScanForward(segments))
            {
                segments &= ~(1U << seg);
                assert(i < 2);

                support.segmentFrequency[seg] = tppaSupport.frequencies[rotation];
                support.segmentImages[seg] = baseImageId.withIndexOffset(tppaSupport.imageIds[rotation][i * 2]).toUInt32();
                support.segmentInteractionItem[seg] = session.getCurrentItem();
                support.segmentInteractionType[seg] = session.getItemType();
                i++;
            }
            session.setAdditionSupport(support);
        }

        static void paintRoadAdditionPPMergeable(PaintSession& session, const World::RoadElement& elRoad, const uint8_t rotation, const ImageId baseImageId, const RoadPaintAdditionPiece& tppa)
        {
            const auto height = elRoad.baseHeight();
            const auto heightOffset = World::Pos3{ 0,
                                                   0,
                                                   height };

            session.addToPlotListTrackRoadAddition(
                baseImageId.withIndexOffset(tppa.imageIds[rotation]),
                0,
                heightOffset,
                tppa.boundingBoxOffsets[rotation] + heightOffset,
                tppa.boundingBoxSizes[rotation]);
            if (tppa.supports.has_value())
            {
                paintSupport(session, tppa.supports.value(), rotation, baseImageId, height);
            }
        }

        static void paintRoadAdditionPPStandard(PaintSession& session, const World::RoadElement& elRoad, const uint8_t rotation, const ImageId baseImageId, const RoadPaintAdditionPiece& tppa)
        {
            const auto height = elRoad.baseHeight();
            const auto heightOffset = World::Pos3{ 0,
                                                   0,
                                                   height };

            session.addToPlotList4FD150(
                baseImageId.withIndexOffset(tppa.imageIds[rotation]),
                heightOffset,
                tppa.boundingBoxOffsets[rotation] + heightOffset,
                tppa.boundingBoxSizes[rotation]);
            if (tppa.supports.has_value())
            {
                paintSupport(session, tppa.supports.value(), rotation, baseImageId, height);
            }
        }

        static void paintRoadAdditionPP(PaintSession& session, const World::RoadElement& elRoad, const uint8_t rotation, const ImageId baseImageId, const RoadPaintAdditionPiece& tppa)
        {
            // TODO: Better way to detect kNullTrackPaintAdditionPiece
            if (tppa.imageIds[3] != 0)
            {
                if (tppa.isIsMergeable)
                {
                    paintRoadAdditionPPMergeable(session, elRoad, rotation, baseImageId, tppa);
                }
                else
                {
                    paintRoadAdditionPPStandard(session, elRoad, rotation, baseImageId, tppa);
                }
            }
        }
    }

    namespace Style02
    {
        constexpr std::array<uint32_t, 3> kMergeBaseImageIndex = {
            RoadObj::ImageIds::Style0::kStraight0NE,
            RoadObj::ImageIds::Style2::kStraight0NE,
            RoadObj::ImageIds::Style2::kStraight0SW,
        };

        static void paintRoadPPMultiTileMerge(PaintSession& session, const World::RoadElement& elRoad, const RoadPaintCommon& roadSession, const uint8_t rotation, const RoadPaintMergeablePiece& rpp, const RoadPaintCommonPiece& rpcp)
        {
            const auto height = elRoad.baseHeight();
            const auto heightOffset = World::Pos3{ 0,
                                                   0,
                                                   height };
            if (session.isHitTest())
            {
                session.addToPlotListTrackRoad(
                    ImageId(rpp.imageIndexOffsets[rotation]),
                    2,
                    heightOffset,
                    rpcp.boundingBoxOffsets[rotation] + heightOffset,
                    rpcp.boundingBoxSizes[rotation]);
            }
            else
            {
                session.setRoadExits(session.getRoadExits() | rpcp.bridgeEdges[rotation]);
                session.setMergeRoadBaseImage(roadSession.roadBaseImageId.withIndexOffset(kMergeBaseImageIndex[enumValue(rpp.isMultiTileMerge[rotation]) - 1]).toUInt32());
                session.setMergeRoadHeight(height);
            }
            if (session.getRenderTarget()->zoomLevel == 0 && !elRoad.hasLevelCrossing() && !elRoad.hasSignalElement() && !elRoad.hasStationElement() && elRoad.streetLightStyle() != 0)
            {
                session.setMergeRoadStreetlight(elRoad.streetLightStyle());
            }
        }

        static void paintRoadPPStandard(PaintSession& session, const World::RoadElement& elRoad, const RoadPaintCommon& roadSession, const uint8_t rotation, const RoadPaintMergeablePiece& rpp, const RoadPaintCommonPiece& rpcp)
        {
            const auto height = elRoad.baseHeight();
            const auto heightOffset = World::Pos3{ 0,
                                                   0,
                                                   height };
            const auto baseImage = roadSession.roadBaseImageId;

            session.addToPlotListTrackRoad(
                baseImage.withIndexOffset(rpp.imageIndexOffsets[rotation]),
                2,
                heightOffset,
                rpcp.boundingBoxOffsets[rotation] + heightOffset,
                rpcp.boundingBoxSizes[rotation]);

            if (session.getRenderTarget()->zoomLevel == 0 && !elRoad.hasLevelCrossing() && !elRoad.hasSignalElement() && !elRoad.hasStationElement() && elRoad.streetLightStyle() != 0)
            {
                paintRoadStreetlights(session, elRoad, rpp.streetlightHeights[rotation]);
            }
        }

        static void paintRoadPP(PaintSession& session, const World::RoadElement& elRoad, const RoadPaintCommon& roadSession, const uint8_t rotation, const RoadPaintMergeablePiece& rpp, const RoadPaintCommonPiece& rpcp)
        {
            // Only need to check [0] for this as none is set for all of them when not a merge
            if (rpp.isMultiTileMerge[0] != RoadPaintMergeType::none)
            {
                paintRoadPPMultiTileMerge(session, elRoad, roadSession, rotation, rpp, rpcp);
            }
            else
            {
                paintRoadPPStandard(session, elRoad, roadSession, rotation, rpp, rpcp);
            }
        }

    }
    namespace Style1
    {
        static void paintRoadPP(PaintSession& session, const World::RoadElement& elRoad, const RoadPaintCommon& roadSession, const uint8_t rotation, const RoadPaintPiece& rpp, const RoadPaintCommonPiece& rpcp)
        {
            const auto height = elRoad.baseHeight();
            const auto heightOffset = World::Pos3{ 0,
                                                   0,
                                                   height };

            const auto baseImage = roadSession.roadBaseImageId;

            session.addToPlotListTrackRoad(
                baseImage.withIndexOffset(rpp.imageIndexOffsets[rotation][0]),
                0,
                heightOffset,
                rpcp.boundingBoxOffsets[rotation] + heightOffset,
                rpcp.boundingBoxSizes[rotation]);
            session.addToPlotListTrackRoad(
                baseImage.withIndexOffset(rpp.imageIndexOffsets[rotation][1]),
                1,
                heightOffset,
                rpcp.boundingBoxOffsets[rotation] + heightOffset,
                rpcp.boundingBoxSizes[rotation]);
            session.addToPlotListTrackRoad(
                baseImage.withIndexOffset(rpp.imageIndexOffsets[rotation][2]),
                3,
                heightOffset,
                rpcp.boundingBoxOffsets[rotation] + heightOffset,
                rpcp.boundingBoxSizes[rotation]);
        }
    }

    static void paintRoadPCP(PaintSession& session, const World::RoadElement& elRoad, const RoadPaintCommon& roadSession, const uint8_t rotation, const RoadPaintCommonPiece& rpcp)
    {
        const auto height = elRoad.baseHeight();
        if (elRoad.hasBridge())
        {
            auto newBridgeEntry = BridgeEntry(
                height,
                rpcp.bridgeType[rotation],
                rpcp.bridgeEdges[rotation],
                rpcp.bridgeQuarters[rotation],
                elRoad.bridge(),
                roadSession.bridgeColoursBaseImageId);
            // There may be other bridge edge/quarters due to merging so OR them together
            newBridgeEntry.edgesQuarters |= session.getBridgeEntry().edgesQuarters;
            session.setBridgeEntry(newBridgeEntry);
        }

        session.insertTunnels(rpcp.tunnelHeights[rotation], height, roadSession.tunnelType);

        session.set525CF8(session.get525CF8() | rpcp.segments[rotation]);
        session.setOccupiedAdditionSupportSegments(session.getOccupiedAdditionSupportSegments() | rpcp.segments[rotation]);
    }

    // 0x004759A6
    void paintRoad(PaintSession& session, const World::RoadElement& elRoad)
    {
        if (elRoad.isAiAllocated() && !showAiPlanningGhosts())
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
            && roadObj->hasFlags(RoadObjectFlags::isOneWay))
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

        // This is an ImageId but it has no image index set!
        auto baseRoadImageColour = ImageId(0, CompanyManager::getCompanyColour(elRoad.owner()));

        if (elRoad.isGhost() || elRoad.isAiAllocated())
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            baseRoadImageColour = Gfx::applyGhostToImage(0);

            // TODO: apply company colour if playerCompanyID != elTrack.owner()?
        }

        RoadPaintCommon roadSession{ baseRoadImageColour.withIndex(roadObj->image), baseRoadImageColour, roadObj->tunnel };

        if (!session.skipTrackRoadSurfaces())
        {
            auto& rpcp = kRoadPaintCommonParts[elRoad.roadId()][elRoad.sequenceIndex()];
            if (roadObj->paintStyle == 0)
            {
                assert(elRoad.roadId() < Style0::kRoadPaintParts.size());
                auto& parts = Style0::kRoadPaintParts[elRoad.roadId()];
                assert(elRoad.sequenceIndex() < parts.size());
                auto& rpp = parts[elRoad.sequenceIndex()];
                Style02::paintRoadPP(session, elRoad, roadSession, rotation, rpp, rpcp);
                paintRoadPCP(session, elRoad, roadSession, rotation, rpcp);
            }
            else if (roadObj->paintStyle == 1)
            {
                assert(elRoad.roadId() < Style1::kRoadPaintParts.size());
                auto& parts = Style1::kRoadPaintParts[elRoad.roadId()];
                assert(elRoad.sequenceIndex() < parts.size());
                auto& rpp = parts[elRoad.sequenceIndex()];
                Style1::paintRoadPP(session, elRoad, roadSession, rotation, rpp, rpcp);
                paintRoadPCP(session, elRoad, roadSession, rotation, rpcp);
            }
            else if (roadObj->paintStyle == 2)
            {
                assert(elRoad.roadId() < Style2::kRoadPaintParts.size());
                auto& parts = Style2::kRoadPaintParts[elRoad.roadId()];
                assert(elRoad.sequenceIndex() < parts.size());
                auto& rpp = parts[elRoad.sequenceIndex()];
                Style02::paintRoadPP(session, elRoad, roadSession, rotation, rpp, rpcp);
                paintRoadPCP(session, elRoad, roadSession, rotation, rpcp);
            }
            else
            {
                assert(false);
                Logging::error("Tried to draw invalid road paint style: paintStyle {}", roadObj->paintStyle);
            }
        }

        if (session.getRenderTarget()->zoomLevel > 1)
        {
            return;
        }

        if (elRoad.hasLevelCrossing())
        {
            paintLevelCrossing(session, baseRoadImageColour, elRoad, rotation);
        }

        if (session.getRenderTarget()->zoomLevel > 0 || roadObj->hasFlags(RoadObjectFlags::unk_03))
        {
            return;
        }
        session.setItemType(Ui::ViewportInteraction::InteractionItem::roadExtra);
        const auto ghostMods = Ui::Windows::Construction::getLastSelectedMods();
        for (auto mod = 0; mod < 2; ++mod)
        {
            const auto* roadExtraObj = ObjectManager::get<RoadExtraObject>(roadObj->mods[mod]);
            ImageId roadExtraBaseImage{};
            if (elRoad.hasMod(mod))
            {
                roadExtraBaseImage = baseRoadImageColour.withIndex(roadExtraObj->image);
            }
            else if (elRoad.hasGhostMods() && (ghostMods & (1U << mod)))
            {
                roadExtraBaseImage = Gfx::applyGhostToImage(roadExtraObj->image);
            }
            else
            {
                continue;
            }

            session.setTrackModId(mod);

            const auto paintStyle = roadExtraObj->paintStyle;
            if (paintStyle == 1 && elRoad.roadId() < AdditionStyle1::kRoadPaintAdditionParts.size() && elRoad.sequenceIndex() < AdditionStyle1::kRoadPaintAdditionParts[elRoad.roadId()].size())
            {
                auto& parts = AdditionStyle1::kRoadPaintAdditionParts[elRoad.roadId()];
                auto& tppa = parts[elRoad.sequenceIndex()];

                AdditionStyle1::paintRoadAdditionPP(session, elRoad, rotation, roadExtraBaseImage, tppa);
            }
            else
            {
                assert(false);
                Logging::error("Tried to draw invalid road id or sequence index: RoadId {} SequenceIndex {}", elRoad.roadId(), elRoad.sequenceIndex());
            }
        }
    }

    constexpr std::array<World::Pos3, 11> kMergeBoundingBoxOffsets = {
        World::Pos3{ 2, 5, 0 },
        World::Pos3{ 5, 2, 0 },
        World::Pos3{ 2, 2, 0 },
        World::Pos3{ 2, 2, 0 },
        World::Pos3{ 2, 2, 0 },
        World::Pos3{ 2, 2, 0 },
        World::Pos3{ 2, 2, 0 },
        World::Pos3{ 2, 2, 0 },
        World::Pos3{ 2, 2, 0 },
        World::Pos3{ 2, 2, 0 },
        World::Pos3{ 2, 2, 0 },
    };
    constexpr std::array<World::Pos3, 11> kMergeBoundingBoxSizes = {
        World::Pos3{ 28, 22, 0 },
        World::Pos3{ 22, 28, 0 },
        World::Pos3{ 28, 28, 0 },
        World::Pos3{ 28, 28, 0 },
        World::Pos3{ 28, 28, 0 },
        World::Pos3{ 28, 28, 0 },
        World::Pos3{ 28, 28, 0 },
        World::Pos3{ 28, 28, 0 },
        World::Pos3{ 28, 28, 0 },
        World::Pos3{ 28, 28, 0 },
        World::Pos3{ 28, 28, 0 },
    };

    // Currently unused but will be soon
    [[maybe_unused]] constexpr std::array<uint32_t, 11> kStyle0MergeImageIndexs = {
        RoadObj::ImageIds::Style0::kStraight0NE,
        RoadObj::ImageIds::Style0::kStraight0SE,
        RoadObj::ImageIds::Style0::kRightCurveVerySmall0NE,
        RoadObj::ImageIds::Style0::kRightCurveVerySmall0SE,
        RoadObj::ImageIds::Style0::kRightCurveVerySmall0SW,
        RoadObj::ImageIds::Style0::kRightCurveVerySmall0NW,
        RoadObj::ImageIds::Style0::kJunctionLeft0NE,
        RoadObj::ImageIds::Style0::kJunctionLeft0SE,
        RoadObj::ImageIds::Style0::kJunctionLeft0SW,
        RoadObj::ImageIds::Style0::kJunctionLeft0NW,
        RoadObj::ImageIds::Style0::kJunctionCrossroads0NE,
    };
    [[maybe_unused]] constexpr std::array<uint32_t, 11> kStyle2LeftMergeImageIndexs = {
        RoadObj::ImageIds::Style2::kStraight0NE,
        RoadObj::ImageIds::Style2::kStraight0SE,
        RoadObj::ImageIds::Style2::kLeftCurveVerySmall0NW,
        RoadObj::ImageIds::Style2::kLeftCurveVerySmall0NE,
        RoadObj::ImageIds::Style2::kLeftCurveVerySmall0SE,
        RoadObj::ImageIds::Style2::kLeftCurveVerySmall0SW,
        RoadObj::ImageIds::Style2::kJunctionLeft0NE,
        RoadObj::ImageIds::Style2::kJunctionLeft0SE,
        RoadObj::ImageIds::Style2::kJunctionLeft0SW,
        RoadObj::ImageIds::Style2::kJunctionLeft0NW,
        RoadObj::ImageIds::Style2::kJunctionCrossroads0NE,
    };
    [[maybe_unused]] constexpr std::array<uint32_t, 11> kStyle2RightMergeImageIndexs = {
        RoadObj::ImageIds::Style2::kStraight0SW,
        RoadObj::ImageIds::Style2::kStraight0NW,
        RoadObj::ImageIds::Style2::kRightCurveVerySmall0NE,
        RoadObj::ImageIds::Style2::kRightCurveVerySmall0SE,
        RoadObj::ImageIds::Style2::kRightCurveVerySmall0SW,
        RoadObj::ImageIds::Style2::kRightCurveVerySmall0NW,
        RoadObj::ImageIds::Style2::kJunctionRight0NE,
        RoadObj::ImageIds::Style2::kJunctionRight0SE,
        RoadObj::ImageIds::Style2::kJunctionRight0SW,
        RoadObj::ImageIds::Style2::kJunctionRight0NW,
        RoadObj::ImageIds::Style2::kJunctionCrossroads0NE2,
    };

    constexpr std::array<uint8_t, 16> kExitsToMergeId = {
        0,  // 0b0000 (not possible)
        0,  // 0b0001 (not possible)
        1,  // 0b0010 (not possible)
        5,  // 0b0011 (turn)
        0,  // 0b0100 (not possible)
        0,  // 0b0101 (straight)
        2,  // 0b0110 (turn)
        8,  // 0b0111 (junction)
        1,  // 0b1000 (not possible)
        4,  // 0b1001 (turn)
        1,  // 0b1010 (straight)
        7,  // 0b1011 (junction)
        3,  // 0b1100 (turn)
        6,  // 0b1101 (junction)
        9,  // 0b1110 (junction)
        10, // 0b1111 (crossroads)
    };

    // 0x004792E7
    void finalisePaintRoad(PaintSession& session)
    {
        const auto exits = session.getRoadExits();
        assert(exits < kExitsToMergeId.size());
        const auto mergeId = kExitsToMergeId[exits];
        const auto height = session.getMergeRoadHeight();
        const auto streetlightType = session.getMergeRoadStreetlight();
        const auto baseImage = ImageId::fromUInt32(session.getMergeRoadBaseImage());
        session.setRoadExits(0);
        session.setMergeRoadStreetlight(0);

        const auto heightOffset = World::Pos3{ 0,
                                               0,
                                               height };
        const auto image = baseImage.withIndexOffset(mergeId);
        const auto boundingBoxOffset = kMergeBoundingBoxOffsets[mergeId] + heightOffset;
        const auto boundingBoxSize = kMergeBoundingBoxSizes[mergeId];
        session.addToPlotListTrackRoad(image, 2, heightOffset, boundingBoxOffset, boundingBoxSize);

        if (streetlightType != 0)
        {
            const auto style = streetlightType - 1;
            for (auto i = 0U; i < 4; ++i)
            {
                if (!(exits & (1U << i)))
                {
                    // Note: Vanilla used a different bboffset and size for this unsure why
                    paintRoadStreetlight(session, height, style, 0, i);
                }
            }
        }
    }
}
