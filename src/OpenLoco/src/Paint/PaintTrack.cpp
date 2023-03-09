#include "PaintTrack.h"
#include "Graphics/Colour.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Paint.h"
#include "PaintTileDecorations.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include "Viewport.hpp"
#include "World/CompanyManager.h"
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>

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

    // 0x0049B6BF
    void paintTrack(PaintSession& session, const World::TrackElement& elTrack)
    {
        if (elTrack.isFlag5())
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
        if (((session.getViewFlags() & Ui::ViewportFlags::height_marks_on_land) != Ui::ViewportFlags::none)
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
            const auto trackPaintFunc = _trackPaintModes[trackObj->var_06][elTrack.trackId()][rotation];
            registers regs;
            regs.esi = X86Pointer(&elTrack);
            regs.ebp = elTrack.sequenceIndex();
            regs.ecx = rotation;
            regs.dx = height;
            call(trackPaintFunc, regs);
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

            const auto trackExtraPaintFunc = _trackExtraPaintModes[trackExtraObj->paintStyle][elTrack.trackId()][rotation];
            registers regs;
            regs.esi = X86Pointer(&elTrack);
            regs.ebp = elTrack.sequenceIndex();
            regs.ecx = rotation;
            regs.dx = height;
            call(trackExtraPaintFunc, regs);
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
