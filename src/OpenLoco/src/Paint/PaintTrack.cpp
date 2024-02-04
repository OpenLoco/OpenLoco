#include "PaintTrack.h"
#include "Graphics/Colour.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Paint.h"
#include "PaintTileDecorations.h"
#include "Ui.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/WindowManager.h"
#include "Viewport.hpp"
#include "World/CompanyManager.h"
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <iostream>

using namespace OpenLoco::Interop;

namespace OpenLoco::Paint
{
    static loco_global<uint32_t, 0x001135F26> _trackBaseImageId;
    static loco_global<uint32_t, 0x001135F2E> _trackExtraImageId;
    static loco_global<uint32_t, 0x001135F32> _trackImageId1;
    static loco_global<uint32_t, 0x001135F36> _trackImageId2;
    static loco_global<uint32_t** [1], 0x004FFB7C> _trackPaintModes;
    static loco_global<uint32_t** [2], 0x004FFB80> _trackExtraPaintModes;
    static loco_global<uint8_t, 0x00113605E> _trackTunnel;
    static loco_global<uint8_t, 0x00522095> _byte_522095;
    static Interop::loco_global<uint8_t[4], 0x0050C185> _tunnelCounts;

    void startTileTPA(TestPaint& tp, uint32_t baseImageId, int16_t height, uint8_t index, uint8_t rotation, uint8_t trackId, uint32_t callOffset, uint8_t paintStyle)
    {
        tp.currentTileTrackAddition.baseImageId = baseImageId;
        tp.currentTileTrackAddition.callCount = 0;
        tp.currentTileTrackAddition.callOffset = callOffset;
        tp.currentTileTrackAddition.height = height;
        tp.currentTileTrackAddition.index = index;
        tp.currentTileTrackAddition.isTrackAddition = true;
        tp.currentTileTrackAddition.paintStyle = paintStyle;
        tp.currentTileTrackAddition.rotation = rotation;
        tp.currentTileTrackAddition.trackId = trackId;
        tp.currentTileTrackAddition.ta = {};
        tp.currentTileTrackAddition.ta.callType = -1;
    }

    void startTileTP(TestPaint& tp, uint32_t baseImageId, int16_t height, uint8_t index, uint8_t rotation, uint8_t trackId, uint32_t callOffset)
    {
        tp.currentTileTrack.baseImageId = baseImageId;
        tp.currentTileTrack.height = height;
        tp.currentTileTrack.index = index;
        tp.currentTileTrack.rotation = rotation;
        tp.currentTileTrack.trackId = trackId;
        tp.currentTileTrack.callCount = 0;
        tp.currentTileTrack.callOffset = callOffset;
        tp.currentTileTrack.isTrack = true;
        tp.currentTileTrack.track = TestPaint::Track{};
        tp.currentTileTrack.tunnelsCount[0] = _tunnelCounts[0];
        tp.currentTileTrack.tunnelsCount[1] = _tunnelCounts[1];
        tp.currentTileTrack.tunnelsCount[2] = _tunnelCounts[2];
        tp.currentTileTrack.tunnelsCount[3] = _tunnelCounts[3];
    }

    void endTileTPA(TestPaint& tp, PaintSession& ps)
    {
        auto& ct = tp.currentTileTrackAddition;
        assert(ct.paintStyle < 2);
        assert(tp.trackAdditions[ct.paintStyle].size() > ct.trackId);
        auto& targetTrackVec = tp.trackAdditions[ct.paintStyle][ct.trackId];
        if (targetTrackVec.size() < static_cast<size_t>(ct.index + 1))
        {
            targetTrackVec.resize(ct.index + 1);
        }
        auto& supports = ps.getAdditionSupports();
        if (supports.height != 0)
        {
            ct.ta.hasSupports = true;
            ct.ta.supportHeight[ct.rotation] = supports.height - ct.height;
            bool supportFound = false;
            for (auto i = 0U; i < 9; ++i)
            {
                if (supports.segmentImages[i] != 0)
                {
                    assert(!supportFound);
                    supportFound = true;
                    ct.ta.supportSegment[ct.rotation] = (1 << i);
                    ct.ta.supportFrequency[ct.rotation] = supports.segmentFrequency[i];
                    ct.ta.supportImageId[ct.rotation] = ImageId::fromUInt32(supports.segmentImages[i]).getIndex() - ct.baseImageId;
                }
            }
        }
        else
        {
            ct.ta.hasSupports = false;
        }
        auto& targetTrack = targetTrackVec[ct.index];
        targetTrack.imageIds[ct.rotation] = ct.ta.imageIds[ct.rotation];
        targetTrack.offsets[ct.rotation] = ct.ta.offsets[ct.rotation];
        targetTrack.boundingBoxOffsets[ct.rotation] = ct.ta.boundingBoxOffsets[ct.rotation];
        targetTrack.boundingBoxSizes[ct.rotation] = ct.ta.boundingBoxSizes[ct.rotation];
        targetTrack.priority = ct.ta.priority;
        targetTrack.callType = ct.ta.callType;
        targetTrack.supportImageId[ct.rotation] = ct.ta.supportImageId[ct.rotation];
        targetTrack.supportHeight[ct.rotation] = ct.ta.supportHeight[ct.rotation];
        targetTrack.supportFrequency[ct.rotation] = ct.ta.supportFrequency[ct.rotation];
        targetTrack.supportSegment[ct.rotation] = ct.ta.supportSegment[ct.rotation];
        targetTrack.callOffset[ct.rotation] = ct.callOffset;
        targetTrack.hasSupports = ct.ta.hasSupports;
        ct.isTrackAddition = false;
    }

    void endTileTP(TestPaint& tp, PaintSession& ps)
    {
        auto& ct = tp.currentTileTrack;
        assert(tp.tracks.size() > ct.trackId);
        auto& targetTrackVec = tp.tracks[ct.trackId];
        if (targetTrackVec.size() < static_cast<size_t>(ct.index + 1))
        {
            targetTrackVec.resize(ct.index + 1);
        }
        if (std::holds_alternative<TestPaint::Track3>(ct.track))
        {
            auto& t3Ct = std::get<TestPaint::Track3>(ct.track);
            auto& bridge = ps.getBridgeEntry();
            t3Ct.segments[ct.rotation] = ps.get525CF8();
            assert(ps.get525CF8() == ps.getOccupiedAdditionSuportSegments());
            if (!bridge.isEmpty())
            {
                t3Ct.bridgeQuarters[ct.rotation] = bridge.edgesQuarters & 0xF;
                t3Ct.bridgeEdges[ct.rotation] = (bridge.edgesQuarters >> 4) & 0xF;
                t3Ct.bridgeType[ct.rotation] = bridge.subType;
                assert(bridge.height == ct.height);
            }

            uint8_t tunnelEdges = 0;
            for (auto i = 0; i < 4; ++i)
            {
                auto tunnels = ps.getTunnels(i);
                if (_tunnelCounts[i] != ct.tunnelsCount[i])
                {
                    tunnelEdges |= (1U << i);
                    t3Ct.tunnelHeight[ct.rotation][i] = (tunnels[0].height * World::kMicroZStep) - ct.height;
                    // assert(tunnels[0].height == ct.height / World::kMicroZStep);
                }
                else
                {
                    t3Ct.tunnelHeight[ct.rotation][i] = -1;
                }
            }
            t3Ct.tunnelEdges[ct.rotation] = tunnelEdges;
            t3Ct.callOffset[ct.rotation] = ct.callOffset;
        }
        else if (std::holds_alternative<TestPaint::Track1>(ct.track))
        {
            auto& t3Ct = std::get<TestPaint::Track1>(ct.track);
            auto& bridge = ps.getBridgeEntry();
            t3Ct.segments[ct.rotation] = ps.get525CF8();
            assert(ps.get525CF8() == ps.getOccupiedAdditionSuportSegments());
            if (!bridge.isEmpty())
            {
                t3Ct.bridgeQuarters[ct.rotation] = bridge.edgesQuarters & 0xF;
                t3Ct.bridgeEdges[ct.rotation] = (bridge.edgesQuarters >> 4) & 0xF;
                t3Ct.bridgeType[ct.rotation] = bridge.subType;
                assert(bridge.height == ct.height);
            }
            uint8_t tunnelEdges = 0;
            for (auto i = 0; i < 4; ++i)
            {
                auto tunnels = ps.getTunnels(i);
                if (_tunnelCounts[i] != ct.tunnelsCount[i])
                {
                    tunnelEdges |= (1U << i);
                    t3Ct.tunnelHeight[ct.rotation][i] = (tunnels[0].height * World::kMicroZStep) - ct.height;
                    // assert(tunnels[0].height == ct.height / World::kMicroZStep);
                }
                else
                {
                    t3Ct.tunnelHeight[ct.rotation][i] = -1;
                }
            }
            t3Ct.tunnelEdges[ct.rotation] = tunnelEdges;

            t3Ct.callOffset[ct.rotation] = ct.callOffset;
        }

        auto& targetTrack = targetTrackVec[ct.index];
        if (std::holds_alternative<TestPaint::Track3>(targetTrack) && std::holds_alternative<TestPaint::Track3>(ct.track))
        {
            auto& t3 = std::get<TestPaint::Track3>(targetTrack);
            auto& t3Ct = std::get<TestPaint::Track3>(ct.track);
            t3.boundingBoxOffsets[ct.rotation] = t3Ct.boundingBoxOffsets[ct.rotation];
            t3.boundingBoxSizes[ct.rotation] = t3Ct.boundingBoxSizes[ct.rotation];
            t3.offsets[ct.rotation] = t3Ct.offsets[ct.rotation];
            t3.bridgeEdges[ct.rotation] = t3Ct.bridgeEdges[ct.rotation];
            t3.bridgeQuarters[ct.rotation] = t3Ct.bridgeQuarters[ct.rotation];
            t3.bridgeType[ct.rotation] = t3Ct.bridgeType[ct.rotation];
            t3.imageIds[ct.rotation] = t3Ct.imageIds[ct.rotation];
            t3.tunnelEdges[ct.rotation] = t3Ct.tunnelEdges[ct.rotation];
            t3.tunnelHeight[ct.rotation] = t3Ct.tunnelHeight[ct.rotation];
            t3.segments[ct.rotation] = t3Ct.segments[ct.rotation];
            t3.callOffset[ct.rotation] = t3Ct.callOffset[ct.rotation];
        }
        else if (std::holds_alternative<TestPaint::Track1>(targetTrack) && std::holds_alternative<TestPaint::Track1>(ct.track))
        {
            auto& t1 = std::get<TestPaint::Track1>(targetTrack);
            auto& t1Ct = std::get<TestPaint::Track1>(ct.track);
            t1.boundingBoxOffsets[ct.rotation] = t1Ct.boundingBoxOffsets[ct.rotation];
            t1.boundingBoxSizes[ct.rotation] = t1Ct.boundingBoxSizes[ct.rotation];
            t1.offsets[ct.rotation] = t1Ct.offsets[ct.rotation];
            t1.bridgeEdges[ct.rotation] = t1Ct.bridgeEdges[ct.rotation];
            t1.bridgeQuarters[ct.rotation] = t1Ct.bridgeQuarters[ct.rotation];
            t1.bridgeType[ct.rotation] = t1Ct.bridgeType[ct.rotation];
            t1.imageIds[ct.rotation] = t1Ct.imageIds[ct.rotation];
            t1.tunnelEdges[ct.rotation] = t1Ct.tunnelEdges[ct.rotation];
            t1.tunnelHeight[ct.rotation] = t1Ct.tunnelHeight[ct.rotation];
            t1.segments[ct.rotation] = t1Ct.segments[ct.rotation];
            t1.callOffset[ct.rotation] = t1Ct.callOffset[ct.rotation];
        }
        else if (std::holds_alternative<TestPaint::Track3>(ct.track) && targetTrack.index() == 0)
        {
            auto& t3Ct = std::get<TestPaint::Track3>(ct.track);
            targetTrack = t3Ct;
            assert(t3Ct.priority[0] == 0);
            assert(t3Ct.priority[1] == 1);
            assert(t3Ct.priority[2] == 3);
        }
        else if (std::holds_alternative<TestPaint::Track1>(ct.track) && targetTrack.index() == 0)
        {
            auto& t3Ct = std::get<TestPaint::Track1>(ct.track);
            targetTrack = t3Ct;
        }
        ct.isTrack = false;
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

        if (!(*_byte_522095 & (1 << 0)))
        {
            auto callOffset = _trackPaintModes[trackObj->var_06][elTrack.trackId()][rotation];
            if (session.tp.tracks[elTrack.trackId()].size() > 1)
            {
                // Dealing with a multi tile so we need to get the calloffset different
                // Confirm we have the jmp instruction
                assert(*reinterpret_cast<uint16_t*>(callOffset) == 0x24FF);
                callOffset = (*reinterpret_cast<uint32_t**>(callOffset + 3))[elTrack.sequenceIndex()];
            }
            // Set to zero all the things that are OR'd
            BridgeEntry entry = session.getBridgeEntry();
            entry.edgesQuarters = 0;
            session.setBridgeEntry(entry);
            session.set525CF8(SegmentFlags::none);
            session.setOccupiedAdditionSupportSegments(SegmentFlags::none);
            startTileTP(session.tp, trackObj->image, height, elTrack.sequenceIndex(), rotation, elTrack.trackId(), callOffset);
            const auto trackPaintFunc = _trackPaintModes[trackObj->var_06][elTrack.trackId()][rotation];
            registers regs;
            regs.esi = X86Pointer(&elTrack);
            regs.ebp = elTrack.sequenceIndex();
            regs.ecx = rotation;
            regs.dx = height;
            call(trackPaintFunc, regs);
            endTileTP(session.tp, session);
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

            auto callOffset = _trackExtraPaintModes[trackExtraObj->paintStyle][elTrack.trackId()][rotation];
            if (session.tp.trackAdditions[trackExtraObj->paintStyle][elTrack.trackId()].size() > 1)
            {
                // Dealing with a multi tile so we need to get the calloffset different
                // Confirm we have the jmp instruction
                assert(*reinterpret_cast<uint16_t*>(callOffset) == 0x24FF);
                callOffset = (*reinterpret_cast<uint32_t**>(callOffset + 3))[elTrack.sequenceIndex()];
            }
            startTileTPA(session.tp, trackExtraObj->image, height, elTrack.sequenceIndex(), rotation, elTrack.trackId(), callOffset, trackExtraObj->paintStyle);
            const auto trackExtraPaintFunc = _trackExtraPaintModes[trackExtraObj->paintStyle][elTrack.trackId()][rotation];
            registers regs;
            regs.esi = X86Pointer(&elTrack);
            regs.ebp = elTrack.sequenceIndex();
            regs.ecx = rotation;
            regs.dx = height;
            call(trackExtraPaintFunc, regs);
            endTileTPA(session.tp, session);
        }
    }

    void registerTrackHooks()
    {
        // These ret's are required to patch all the individual trackid paint functions
        // in vanilla they were jmp's that returned into paintTrack but we can't do that.
        // This hook can be removed after all of the individual paint functions have been
        // implemented.
        writeRet(0x004125D8);
        writeRet(0x00412709);
        writeRet(0x0041283A);
        writeRet(0x00412974);
        writeRet(0x00412AAE);
        writeRet(0x00412BE8);
        writeRet(0x00412D22);
        writeRet(0x00412E5C);
        writeRet(0x00412F96);
        writeRet(0x004130D0);
        writeRet(0x0041320A);
        writeRet(0x00413344);
        writeRet(0x0041347E);
        writeRet(0x004135B8);
        writeRet(0x004136F2);
        writeRet(0x0041382C);
        writeRet(0x00413966);
        writeRet(0x00413AA0);
        writeRet(0x00413BDA);
        writeRet(0x00413D14);
        writeRet(0x00413E4E);
        writeRet(0x00413F86);
        writeRet(0x004140C0);
        writeRet(0x004141FA);
        writeRet(0x00414334);
        writeRet(0x0041446C);
        writeRet(0x004145A6);
        writeRet(0x004146E0);
        writeRet(0x0041481A);
        writeRet(0x00414952);
        writeRet(0x00414A8C);
        writeRet(0x00414B96);
        writeRet(0x00414C9E);
        writeRet(0x00414DA6);
        writeRet(0x00414EB0);
        writeRet(0x00414FE9);
        writeRet(0x004150B8);
        writeRet(0x00415189);
        writeRet(0x0041528A);
        writeRet(0x0041538B);
        writeRet(0x0041545A);
        writeRet(0x0041552B);
        writeRet(0x0041562C);
        writeRet(0x0041572D);
        writeRet(0x004157FC);
        writeRet(0x004158CB);
        writeRet(0x004159CC);
        writeRet(0x00415ACD);
        writeRet(0x00415B9C);
        writeRet(0x00415C6D);
        writeRet(0x00415D6E);
        writeRet(0x00415E98);
        writeRet(0x00415F20);
        writeRet(0x00415FAA);
        writeRet(0x00416068);
        writeRet(0x00416122);
        writeRet(0x004161AA);
        writeRet(0x00416234);
        writeRet(0x004162F2);
        writeRet(0x004163AC);
        writeRet(0x00416434);
        writeRet(0x004164BC);
        writeRet(0x0041657A);
        writeRet(0x00416634);
        writeRet(0x004166BC);
        writeRet(0x00416746);
        writeRet(0x00416804);
        writeRet(0x004168C2);
        writeRet(0x0041694A);
        writeRet(0x004169D4);
        writeRet(0x00416A8E);
        writeRet(0x00416B4C);
        writeRet(0x00416BD4);
        writeRet(0x00416C5E);
        writeRet(0x00416D18);
        writeRet(0x00416DD6);
        writeRet(0x00416E5E);
        writeRet(0x00416EE6);
        writeRet(0x00416FA0);
        writeRet(0x0041705E);
        writeRet(0x004170E6);
        writeRet(0x00417170);
        writeRet(0x0041722A);
        writeRet(0x00417354);
        writeRet(0x004173DC);
        writeRet(0x00417466);
        writeRet(0x00417524);
        writeRet(0x004175DE);
        writeRet(0x00417666);
        writeRet(0x004176F0);
        writeRet(0x004177AE);
        writeRet(0x00417868);
        writeRet(0x004178F0);
        writeRet(0x00417978);
        writeRet(0x00417A36);
        writeRet(0x00417AF0);
        writeRet(0x00417B78);
        writeRet(0x00417C02);
        writeRet(0x00417CC0);
        writeRet(0x00417D7E);
        writeRet(0x00417E06);
        writeRet(0x00417E90);
        writeRet(0x00417F4A);
        writeRet(0x00418008);
        writeRet(0x00418090);
        writeRet(0x0041811A);
        writeRet(0x004181D4);
        writeRet(0x00418295);
        writeRet(0x00418320);
        writeRet(0x004183AB);
        writeRet(0x00418468);
        writeRet(0x00418529);
        writeRet(0x004185B4);
        writeRet(0x00418641);
        writeRet(0x004186FE);
        writeRet(0x00418840);
        writeRet(0x0041891A);
        writeRet(0x004189F2);
        writeRet(0x00418ACC);
        writeRet(0x00418BD6);
        writeRet(0x00418CE0);
        writeRet(0x00418DBA);
        writeRet(0x00418E94);
        writeRet(0x00418F6E);
        writeRet(0x00419078);
        writeRet(0x00419182);
        writeRet(0x0041925A);
        writeRet(0x00419334);
        writeRet(0x0041940C);
        writeRet(0x00419516);
        writeRet(0x00419620);
        writeRet(0x004196FA);
        writeRet(0x004197D4);
        writeRet(0x004198AE);
        writeRet(0x004199B8);
        writeRet(0x00419B32);
        writeRet(0x00419C0C);
        writeRet(0x00419CE6);
        writeRet(0x00419DBE);
        writeRet(0x00419E98);
        writeRet(0x00419FA2);
        writeRet(0x0041A07A);
        writeRet(0x0041A154);
        writeRet(0x0041A22C);
        writeRet(0x0041A306);
        writeRet(0x0041A410);
        writeRet(0x0041A4EA);
        writeRet(0x0041A5C4);
        writeRet(0x0041A69C);
        writeRet(0x0041A774);
        writeRet(0x0041A87E);
        writeRet(0x0041A958);
        writeRet(0x0041AA30);
        writeRet(0x0041AB08);
        writeRet(0x0041ABE2);
        writeRet(0x0041ACEC);
        writeRet(0x0041ADC6);
        writeRet(0x0041AE9E);
        writeRet(0x0041AF76);
        writeRet(0x0041B050);
        writeRet(0x0041B15A);
        writeRet(0x0041B234);
        writeRet(0x0041B30E);
        writeRet(0x0041B3E6);
        writeRet(0x0041B4BE);
        writeRet(0x0041B5C8);
        writeRet(0x0041B6A0);
        writeRet(0x0041B77A);
        writeRet(0x0041B852);
        writeRet(0x0041B92C);
        writeRet(0x0041BA36);
        writeRet(0x0041BB10);
        writeRet(0x0041BBEA);
        writeRet(0x0041BCC2);
        writeRet(0x0041BD9C);
        writeRet(0x0041BE92);
        writeRet(0x0041BF6A);
        writeRet(0x0041C042);
        writeRet(0x0041C11C);
        writeRet(0x0041C1F6);
        writeRet(0x0041C2CE);
        writeRet(0x0041C3A6);
        writeRet(0x0041C47E);
        writeRet(0x0041C573);
        writeRet(0x0041C634);
        writeRet(0x0041C6F1);
        writeRet(0x0041C7B2);
        writeRet(0x0041C86F);
        writeRet(0x0041C930);
        writeRet(0x0041C9ED);
        writeRet(0x0041CAAE);
        writeRet(0x0041CB9F);
        writeRet(0x0041CC90);
        writeRet(0x0041CD81);
        writeRet(0x0041CE72);
        writeRet(0x0041CFB4);
        writeRet(0x0041D08E);
        writeRet(0x0041D168);
        writeRet(0x0041D272);
        writeRet(0x0041D37C);
        writeRet(0x0041D454);
        writeRet(0x0041D52E);
        writeRet(0x0041D638);
        writeRet(0x0041D742);
        writeRet(0x0041D81C);
        writeRet(0x0041D8F4);
        writeRet(0x0041D9FE);
        writeRet(0x0041DB08);
        writeRet(0x0041DBE2);
        writeRet(0x0041DCBC);
        writeRet(0x0041DDC6);
    }
}
