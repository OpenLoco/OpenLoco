#include "PaintTrack.h"
#include "Graphics/Colour.h"
#include "Logging.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Paint.h"
#include "PaintTileDecorations.h"
#include "PaintTrackAdditionsData.h"
#include "PaintTrackData.h"
#include "Ui.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/WindowManager.h"
#include "Viewport.hpp"
#include "World/CompanyManager.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Paint
{
    static loco_global<uint32_t, 0x001135F26> _trackBaseImageId;
    static loco_global<uint32_t, 0x001135F2E> _trackExtraImageId;
    static loco_global<uint32_t, 0x001135F32> _trackImageId1;
    static loco_global<uint32_t, 0x001135F36> _trackImageId2;
    static loco_global<uint32_t** [2], 0x004FFB80> _trackExtraPaintModes;
    static loco_global<uint8_t, 0x00113605E> _trackTunnel;
    static loco_global<uint8_t, 0x00522095> _byte_522095;

    namespace Style0
    {
        static void paintTrackAdditionPPMergable(PaintSession& session, const World::TrackElement& elTrack, const uint8_t rotation, const ImageId baseImageId, const TrackPaintAdditionPiece& tppa)
        {
            const auto height = elTrack.baseHeight();
            const auto heightOffset = World::Pos3{ 0,
                                                   0,
                                                   height };

            session.addToPlotListTrackRoad(
                baseImageId.withIndexOffset(tppa.imageIds[rotation]),
                4,
                heightOffset,
                tppa.boundingBoxOffsets[rotation] + heightOffset,
                tppa.boundingBoxSizes[rotation]);
        }

        static void paintTrackAdditionPPStandard(PaintSession& session, const World::TrackElement& elTrack, const uint8_t rotation, const ImageId baseImageId, const TrackPaintAdditionPiece& tppa)
        {
            const auto height = elTrack.baseHeight();
            const auto heightOffset = World::Pos3{ 0,
                                                   0,
                                                   height };

            session.addToPlotListAsChild(
                baseImageId.withIndexOffset(tppa.imageIds[rotation]),
                heightOffset,
                tppa.boundingBoxOffsets[rotation] + heightOffset,
                tppa.boundingBoxSizes[rotation]);
        }
    }
    namespace Style1
    {
        static void paintSupport(PaintSession& session, const TrackAdditionSupport& tppaSupport, const uint8_t rotation, const ImageId baseImageId, int16_t height)
        {
            const auto seg = Numerics::bitScanForward(enumValue(tppaSupport.segments[rotation]));
            assert(seg != -1);
            TrackRoadAdditionSupports support{};
            support.height = height + tppaSupport.height;
            support.occupiedSegments = session.getOccupiedAdditionSupportSegments();
            support.segmentFrequency[seg] = tppaSupport.frequencies[rotation];
            support.segmentImages[seg] = baseImageId.withIndexOffset(tppaSupport.imageIds[rotation][0]).toUInt32();
            support.segmentInteractionItem[seg] = session.getCurrentItem();
            support.segmentInteractionType[seg] = session.getItemType();
            session.setAdditionSupport(support);
        }

        static void paintTrackAdditionPPMergable(PaintSession& session, const World::TrackElement& elTrack, const uint8_t rotation, const ImageId baseImageId, const TrackPaintAdditionPiece& tppa)
        {
            const auto height = elTrack.baseHeight();
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

        static void paintTrackAdditionPPStandard(PaintSession& session, const World::TrackElement& elTrack, const uint8_t rotation, const ImageId baseImageId, const TrackPaintAdditionPiece& tppa)
        {
            const auto height = elTrack.baseHeight();
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
    }

    struct TrackPaintCommon
    {
        ImageId trackBaseImageId;         // 0x01135F26 with colours and image index set to base of trackObject image table
        ImageId bridgeColoursBaseImageId; // 0x01135F36 with only the colours set (image index not set!)
        uint8_t tunnelType;               // 0x0113605E
    };

    static void paintTrackPPMergable(PaintSession& session, const World::TrackElement& elTrack, const TrackPaintCommon& trackSession, const uint8_t rotation, const TrackPaintPiece& tpp)
    {
        const auto height = elTrack.baseHeight();
        const auto heightOffset = World::Pos3{ 0,
                                               0,
                                               height };
        if (elTrack.hasBridge())
        {
            auto newBridgeEntry = BridgeEntry(
                height,
                tpp.bridgeType[rotation],
                tpp.bridgeEdges[rotation],
                tpp.bridgeQuarters[rotation],
                elTrack.bridge(),
                trackSession.bridgeColoursBaseImageId);
            // There may be other bridge edge/quarters due to merging so OR them together
            newBridgeEntry.edgesQuarters |= session.getBridgeEntry().edgesQuarters;
            session.setBridgeEntry(newBridgeEntry);
        }

        const auto baseImage = trackSession.trackBaseImageId;

        session.addToPlotListTrackRoad(
            baseImage.withIndexOffset(tpp.imageIndexOffsets[rotation][0]),
            0,
            heightOffset,
            tpp.boundingBoxOffsets[rotation] + heightOffset,
            tpp.boundingBoxSizes[rotation]);
        session.addToPlotListTrackRoad(
            baseImage.withIndexOffset(tpp.imageIndexOffsets[rotation][1]),
            1,
            heightOffset,
            tpp.boundingBoxOffsets[rotation] + heightOffset,
            tpp.boundingBoxSizes[rotation]);
        session.addToPlotListTrackRoad(
            baseImage.withIndexOffset(tpp.imageIndexOffsets[rotation][2]),
            3,
            heightOffset,
            tpp.boundingBoxOffsets[rotation] + heightOffset,
            tpp.boundingBoxSizes[rotation]);

        session.insertTunnels(tpp.tunnelHeights[rotation], height, trackSession.tunnelType);

        session.set525CF8(session.get525CF8() | tpp.segments[rotation]);
        session.setOccupiedAdditionSupportSegments(session.getOccupiedAdditionSupportSegments() | tpp.segments[rotation]);
    }

    static void paintTrackPPStandard(PaintSession& session, const World::TrackElement& elTrack, const TrackPaintCommon& trackSession, const uint8_t rotation, const TrackPaintPiece& tpp)
    {
        const auto height = elTrack.baseHeight();
        const auto heightOffset = World::Pos3{ 0,
                                               0,
                                               height };
        if (elTrack.hasBridge())
        {
            auto newBridgeEntry = BridgeEntry(
                height,
                tpp.bridgeType[rotation],
                tpp.bridgeEdges[rotation],
                tpp.bridgeQuarters[rotation],
                elTrack.bridge(),
                trackSession.bridgeColoursBaseImageId);
            // There may be other bridge edge/quarters due to merging so OR them together
            newBridgeEntry.edgesQuarters |= session.getBridgeEntry().edgesQuarters;
            session.setBridgeEntry(newBridgeEntry);
        }

        const auto baseImage = trackSession.trackBaseImageId;

        session.addToPlotList4FD150(
            baseImage.withIndexOffset(tpp.imageIndexOffsets[rotation][0]),
            heightOffset,
            tpp.boundingBoxOffsets[rotation] + heightOffset,
            tpp.boundingBoxSizes[rotation]);

        session.insertTunnels(tpp.tunnelHeights[rotation], height, trackSession.tunnelType);

        session.set525CF8(session.get525CF8() | tpp.segments[rotation]);
        session.setOccupiedAdditionSupportSegments(session.getOccupiedAdditionSupportSegments() | tpp.segments[rotation]);
    }

    // 0x0049B6BF
    void paintTrack(PaintSession& session, const World::TrackElement& elTrack)
    {
        if (elTrack.isAiAllocated())
        {
            return;
        }
        if (elTrack.isGhost()
            && CompanyManager::getSecondaryPlayerId() != CompanyId::null
            && CompanyManager::getSecondaryPlayerId() == elTrack.owner())
        {
            return;
        }
        const auto height = elTrack.baseZ() * 4;
        const auto rotation = (session.getRotation() + elTrack.unkDirection()) & 0x3;
        if (((session.getViewFlags() & Ui::ViewportFlags::height_marks_on_tracks_roads) != Ui::ViewportFlags::none)
            && session.getRenderTarget()->zoomLevel == 0)
        {
            const bool isLast = elTrack.isFlag6();
            const bool isFirstTile = elTrack.sequenceIndex() == 0;
            if (elTrack.sequenceIndex() == 0 || isLast)
            {
                session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
                const auto markerHeight = height + getTrackDecorationHeightOffset(isFirstTile, elTrack.trackId()) + 8;
                const auto imageId = ImageId{ getHeightMarkerImage(markerHeight), Colour::blue };
                const World::Pos3 offset(16, 16, markerHeight);
                const World::Pos3 bbOffset(1000, 1000, 1087);
                const World::Pos3 bbSize(1, 1, 0);
                session.addToPlotListAsParent(imageId, offset, bbOffset, bbSize);
            }
        }

        session.setItemType(Ui::ViewportInteraction::InteractionItem::track);
        const auto* trackObj = ObjectManager::get<TrackObject>(elTrack.trackObjectId());
        _trackBaseImageId = trackObj->image;
        _trackTunnel = trackObj->tunnel;

        _trackImageId1 = Gfx::recolour(0, CompanyManager::getCompanyColour(elTrack.owner()));
        _trackImageId2 = _trackImageId1;

        if (elTrack.isGhost())
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            _trackImageId1 = Gfx::applyGhostToImage(_trackImageId1).toUInt32();
            _trackImageId2 = Gfx::applyGhostToImage(_trackImageId2).toUInt32();
        }

        _trackBaseImageId |= _trackImageId1;
        TrackPaintCommon trackSession{ ImageId::fromUInt32(_trackBaseImageId), ImageId::fromUInt32(_trackImageId2), _trackTunnel };

        if (!(*_byte_522095 & (1 << 0)))
        {
            if (elTrack.trackId() < kTrackPaintParts.size() && elTrack.sequenceIndex() < kTrackPaintParts[elTrack.trackId()].size())
            {
                auto& parts = kTrackPaintParts[elTrack.trackId()];
                auto& tpp = parts[elTrack.sequenceIndex()];
                if (tpp.isMergable)
                {
                    paintTrackPPMergable(session, elTrack, trackSession, rotation, tpp);
                }
                else
                {
                    paintTrackPPStandard(session, elTrack, trackSession, rotation, tpp);
                }
            }
            else
            {
                assert(false);
                Logging::error("Tried to draw invalid track id or sequence index: TrackId {} SequenceIndex {}", elTrack.trackId(), elTrack.sequenceIndex());
            }
        }

        if (session.getRenderTarget()->zoomLevel > 0)
        {
            return;
        }

        session.setItemType(Ui::ViewportInteraction::InteractionItem::trackExtra);
        const auto ghostMods = Ui::Windows::Construction::getLastSelectedMods();
        for (auto mod = 0; mod < 4; ++mod)
        {
            const auto* trackExtraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[mod]);
            if (elTrack.hasMod(mod))
            {
                _trackExtraImageId = _trackImageId1 + trackExtraObj->image;
            }
            else if (elTrack.hasGhostMods() && ghostMods & (1 << mod))
            {
                _trackExtraImageId = Gfx::applyGhostToImage(trackExtraObj->image).toUInt32();
            }
            else
            {
                continue;
            }

            session.setTrackModId(mod);

            const auto paintStyle = trackExtraObj->paintStyle;
            if (paintStyle == 0 && elTrack.trackId() < Style0::kTrackPaintAdditionParts.size() && elTrack.sequenceIndex() < Style0::kTrackPaintAdditionParts[elTrack.trackId()].size())
            {
                auto& parts = Style0::kTrackPaintAdditionParts[elTrack.trackId()];
                auto& tppa = parts[elTrack.sequenceIndex()];
                if (tppa.isIsMergable)
                {
                    Style0::paintTrackAdditionPPMergable(session, elTrack, rotation, ImageId::fromUInt32(_trackExtraImageId), tppa);
                }
                else
                {
                    Style0::paintTrackAdditionPPStandard(session, elTrack, rotation, ImageId::fromUInt32(_trackExtraImageId), tppa);
                }
            }
            else if (paintStyle == 1 && elTrack.trackId() < Style1::kTrackPaintAdditionParts.size() && elTrack.sequenceIndex() < Style1::kTrackPaintAdditionParts[elTrack.trackId()].size())
            {
                auto& parts = Style1::kTrackPaintAdditionParts[elTrack.trackId()];
                auto& tppa = parts[elTrack.sequenceIndex()];
                // TODO: Better way to detect kNullTrackPaintAdditionPiece
                if (tppa.imageIds[3] != 0)
                {
                    if (tppa.isIsMergable)
                    {
                        Style1::paintTrackAdditionPPMergable(session, elTrack, rotation, ImageId::fromUInt32(_trackExtraImageId), tppa);
                    }
                    else
                    {
                        Style1::paintTrackAdditionPPStandard(session, elTrack, rotation, ImageId::fromUInt32(_trackExtraImageId), tppa);
                    }
                }
            }
            else
            {
                assert(false);
                Logging::error("Tried to draw invalid track id or sequence index: TrackId {} SequenceIndex {}", elTrack.trackId(), elTrack.sequenceIndex());

                const auto trackExtraPaintFunc = _trackExtraPaintModes[trackExtraObj->paintStyle][elTrack.trackId()][rotation];
                registers regs;
                regs.esi = X86Pointer(&elTrack);
                regs.ebp = elTrack.sequenceIndex();
                regs.ecx = rotation;
                regs.dx = height;
                call(trackExtraPaintFunc, regs);
            }
        }
    }
}
