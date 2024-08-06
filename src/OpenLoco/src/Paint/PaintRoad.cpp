#include "PaintRoad.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Logging.h"
#include "Map/RoadElement.h"
#include "Objects/LevelCrossingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Paint.h"
#include "PaintTileDecorations.h"
#include "ScenarioManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Diagnostics;
namespace OpenLoco::Paint
{
    static Interop::loco_global<uint32_t, 0x0112C280> _roadBaseImageId;
    static Interop::loco_global<uint32_t, 0x0112C28C> _roadExtraImageId;
    static Interop::loco_global<uint32_t, 0x01135F32> _roadImageId1;
    static Interop::loco_global<uint32_t, 0x01135F36> _roadImageId2;
    static Interop::loco_global<uint8_t, 0x00113605E> _roadTunnel;
    static Interop::loco_global<uint8_t, 0x00522095> _byte_522095;
    static Interop::loco_global<uint32_t** [3], 0x004FE43C> _roadPaintModes;
    static Interop::loco_global<uint32_t** [2], 0x004FE448> _roadExtraPaintModes;

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
    static void paintLevelCrossing(PaintSession& session, const World::RoadElement& elRoad, const uint8_t rotation)
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
            const auto image0 = ImageId::fromUInt32(_roadImageId1).withIndex(imageIndex0);
            session.addToPlotList4FD150(image0, heightOffset, bbOffset, bbSize);
        }
        {
            const auto bbOffset = World::Pos3{ 2, 30, 1 } + heightOffset;
            const auto bbSize = World::Pos3{ 1, 1, 8 };
            const auto image1 = ImageId::fromUInt32(_roadImageId1).withIndex(imageIndex0 + 1);
            session.addToPlotList4FD150(image1, heightOffset, bbOffset, bbSize);
        }
        {
            const auto bbOffset = World::Pos3{ 30, 2, 1 } + heightOffset;
            const auto bbSize = World::Pos3{ 1, 1, 8 };
            const auto image2 = ImageId::fromUInt32(_roadImageId1).withIndex(imageIndex0 + 2);
            session.addToPlotList4FD150(image2, heightOffset, bbOffset, bbSize);
        }

        const auto image3 = ImageId::fromUInt32(_roadImageId1).withIndex(imageIndex0 + 3);
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
            const auto roadPaintFunc = _roadPaintModes[roadObj->paintStyle][elRoad.roadId()][rotation];
            Interop::registers regs;
            regs.esi = Interop::X86Pointer(&elRoad);
            regs.ebp = elRoad.sequenceIndex();
            regs.ecx = rotation;
            regs.dx = height;
            call(roadPaintFunc, regs);

            /// Paint Style 0:
            /// For very small and straight only sets up globals (unless in hit detection mode where it paints some dummies)
            /// For small paints road and streetlights
            /// For hills paints road and streetlights * 2 (either side)
            ///
            /// Paint Style 1:
            /// Mergeable track style 3 parts
            ///
            /// Paint Style 2:
            /// Same as style 0 but not symmetrical for image ids

            // if (elRoad.roadId() < kRoadPaintParts.size() && elRoad.sequenceIndex() < kRoadPaintParts[elRoad.roadId()].size())
            //{
            //     auto& parts = kRoadPaintParts[elRoad.roadId()];
            //     auto& tpp = parts[elRoad.sequenceIndex()];
            //     paintRoadPP(session, elRoad, roadSession, rotation, tpp);
            // }
            // else
            //{
            //     assert(false);
            //     Logging::error("Tried to draw invalid track id or sequence index: TrackId {} SequenceIndex {}", elRoad.roadId(), elRoad.sequenceIndex());
            // }
        }

        if (session.getRenderTarget()->zoomLevel > 1)
        {
            return;
        }

        if (elRoad.hasLevelCrossing())
        {
            paintLevelCrossing(session, elRoad, rotation);
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
            if (elRoad.hasMod(mod))
            {
                _roadExtraImageId = _roadImageId1 + roadExtraObj->image;
            }
            else if (elRoad.hasGhostMods() && (ghostMods & (1U << mod)))
            {
                _roadExtraImageId = Gfx::applyGhostToImage(roadExtraObj->image).toUInt32();
            }
            else
            {
                continue;
            }
            const auto trackExtraBaseImage = ImageId::fromUInt32(_roadExtraImageId);

            session.setTrackModId(mod);

            const auto paintStyle = roadExtraObj->paintStyle;
            assert(paintStyle == 1);

            const auto roadPaintFunc = _roadExtraPaintModes[paintStyle][elRoad.roadId()][rotation];
            Interop::registers regs;
            regs.esi = Interop::X86Pointer(&elRoad);
            regs.ebp = elRoad.sequenceIndex();
            regs.ecx = rotation;
            regs.dx = height;
            call(roadPaintFunc, regs);
            // if (paintStyle == 0 && elTrack.trackId() < Style0::kTrackPaintAdditionParts.size() && elTrack.sequenceIndex() < Style0::kTrackPaintAdditionParts[elTrack.trackId()].size())
            //{
            //     auto& parts = Style0::kTrackPaintAdditionParts[elTrack.trackId()];
            //     auto& tppa = parts[elTrack.sequenceIndex()];

            //    Style0::paintTrackAdditionPP(session, elTrack, rotation, trackExtraBaseImage, tppa);
            //}
            // else if (paintStyle == 1 && elTrack.trackId() < Style1::kTrackPaintAdditionParts.size() && elTrack.sequenceIndex() < Style1::kTrackPaintAdditionParts[elTrack.trackId()].size())
            //{
            //    auto& parts = Style1::kTrackPaintAdditionParts[elTrack.trackId()];
            //    auto& tppa = parts[elTrack.sequenceIndex()];

            //    Style1::paintTrackAdditionPP(session, elTrack, rotation, trackExtraBaseImage, tppa);
            //}
            // else
            //{
            //    assert(false);
            //    Logging::error("Tried to draw invalid track id or sequence index: TrackId {} SequenceIndex {}", elTrack.trackId(), elTrack.sequenceIndex());
            //}
        }
    }

    void registerRoadHooks()
    {
        // These ret's are required to patch all the individual trackid paint functions
        // in vanilla they were jmp's that returned into paintTrack but we can't do that.
        // This hook can be removed after all of the individual paint functions have been
        // implemented.
        Interop::writeRet(0x004083AC);
        Interop::writeRet(0x00409000);
        Interop::writeRet(0x00409198);
        Interop::writeRet(0x0040924D);
        Interop::writeRet(0x00409304);
        Interop::writeRet(0x00409488);
        Interop::writeRet(0x0040950D);
        Interop::writeRet(0x00409594);
        Interop::writeRet(0x004096C4);
        Interop::writeRet(0x004097F4);
        Interop::writeRet(0x0040987B);
        Interop::writeRet(0x00409902);
        Interop::writeRet(0x00409A32);
        Interop::writeRet(0x00409B62);
        Interop::writeRet(0x00409BE9);
        Interop::writeRet(0x00409C6E);
        Interop::writeRet(0x00409D9E);
        Interop::writeRet(0x00409ECE);
        Interop::writeRet(0x00409F55);
        Interop::writeRet(0x00409FDC);
        Interop::writeRet(0x0040A10C);
        Interop::writeRet(0x0040A23C);
        Interop::writeRet(0x0040A2C1);
        Interop::writeRet(0x0040A348);
        Interop::writeRet(0x0040A478);
        Interop::writeRet(0x0040A5A8);
        Interop::writeRet(0x0040A62F);
        Interop::writeRet(0x0040A6B6);
        Interop::writeRet(0x0040A7E6);
        Interop::writeRet(0x0040A916);
        Interop::writeRet(0x0040A99D);
        Interop::writeRet(0x0040AA22);
        Interop::writeRet(0x0040AB52);
        Interop::writeRet(0x0040AC82);
        Interop::writeRet(0x0040AD09);
        Interop::writeRet(0x0040AD90);
        Interop::writeRet(0x0040AEC0);
        Interop::writeRet(0x0040B0C5);
        Interop::writeRet(0x0040B27A);
        Interop::writeRet(0x0040B42B);
        Interop::writeRet(0x0040B5E0);
        Interop::writeRet(0x0040B791);
        Interop::writeRet(0x0040B946);
        Interop::writeRet(0x0040BAF7);
        Interop::writeRet(0x0040BCAC);
        Interop::writeRet(0x0040BE5D);
        Interop::writeRet(0x0040C012);
        Interop::writeRet(0x0040C1C3);
        Interop::writeRet(0x0040C378);
        Interop::writeRet(0x0040C529);
        Interop::writeRet(0x0040C6DE);
        Interop::writeRet(0x0040C88F);
        Interop::writeRet(0x0040CA44);
        Interop::writeRet(0x0040CC29);
        Interop::writeRet(0x0040CE0E);
        Interop::writeRet(0x0040CFF3);
        Interop::writeRet(0x0040D1D8);
        Interop::writeRet(0x0040D3BD);
        Interop::writeRet(0x0040D5A2);
        Interop::writeRet(0x0040D787);
        Interop::writeRet(0x0040D96C);
        Interop::writeRet(0x0040D9A3);
        Interop::writeRet(0x0040D9A8);
        Interop::writeRet(0x0040DAD9);
        Interop::writeRet(0x0040DC0A);
        Interop::writeRet(0x0040DD41);
        Interop::writeRet(0x0040DE78);
        Interop::writeRet(0x0040DFAD);
        Interop::writeRet(0x0040E0E4);
        Interop::writeRet(0x0040E1EE);
        Interop::writeRet(0x0040E2F6);
        Interop::writeRet(0x0040E3FE);
        Interop::writeRet(0x0040E508);
        Interop::writeRet(0x0040E641);
        Interop::writeRet(0x0040E710);
        Interop::writeRet(0x0040E7E1);
        Interop::writeRet(0x0040E8E2);
        Interop::writeRet(0x0040E9E3);
        Interop::writeRet(0x0040EAB4);
        Interop::writeRet(0x0040EB85);
        Interop::writeRet(0x0040EC86);
        Interop::writeRet(0x0040ED87);
        Interop::writeRet(0x0040EE58);
        Interop::writeRet(0x0040EF27);
        Interop::writeRet(0x0040F028);
        Interop::writeRet(0x0040F129);
        Interop::writeRet(0x0040F1FA);
        Interop::writeRet(0x0040F2CB);
        Interop::writeRet(0x0040F3CC);
        Interop::writeRet(0x0040F50D);
        Interop::writeRet(0x0040F61A);
        Interop::writeRet(0x0040F723);
        Interop::writeRet(0x0040F830);
        Interop::writeRet(0x0040F939);
        Interop::writeRet(0x0040FA46);
        Interop::writeRet(0x0040FB4F);
        Interop::writeRet(0x0040FC5C);
        Interop::writeRet(0x0040FD99);
        Interop::writeRet(0x0040FED6);
        Interop::writeRet(0x00410013);
        Interop::writeRet(0x00410150);
    }
}
