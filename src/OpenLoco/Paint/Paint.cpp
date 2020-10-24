#include "Paint.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../Ui.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    static loco_global<Gfx::drawpixelinfo_t*, 0x00E0C3E0> _sessionDpi;
    static loco_global<PaintEntry[4000], 0x00E0C410> _sessionPaintEntries;
    static loco_global<PaintStruct* [1024], 0x00E3F0C0> _sessionQuadrants;
    static loco_global<uint32_t, 0x00E400C0> _sessionQuadrantBackIndex;
    static loco_global<uint32_t, 0x00E400C4> _sessionQuadrantFrontIndex;
    static loco_global<const void*, 0x00E4F0B4> _sessionCurrentlyDrawnItem;
    static loco_global<PaintEntry*, 0x00E0C404> _sessionEndOfPaintStructArray;
    static loco_global<PaintEntry*, 0x00E0C40C> _sessionNextFreePaintStruct;
    static loco_global<coord_t, 0x00E3F090> _sessionSpritePositionX;
    static loco_global<coord_t, 0x00E3F096> _sessionSpritePositionY;
    static loco_global<PaintStruct*, 0x00E40120> _sessionLastRootPS;
    static loco_global<InteractionItem, 0x00E3F0AC> _sessionType;
    static loco_global<PaintStringStruct*, 0x00E40118> _sessionPaintStringHead;
    static loco_global<PaintStringStruct*, 0x00E4011C> _sessionLastPaintString;
    static loco_global<Map::map_pos, 0x00E3F0B0> _sessionMapPosition;
    static loco_global<InteractionItem, 0x00E40104> _sessionInteractionInfoType;
    static loco_global<uint8_t, 0x00E40105> _sessionInteractionInfoBh; // Unk var_29 of paintstruct
    static loco_global<int16_t, 0x00E40108> _sessionInteractionInfoX;
    static loco_global<int16_t, 0x00E4010A> _sessionInteractionInfoY;
    static loco_global<uint32_t, 0x00E4010C> _sessionInteractionInfoValue; // tileElement or thing ptr
    static loco_global<uint32_t, 0x00E40110> _getMapCoordinatesFromPosFlags;
    PaintSession _session;

    // 0x0045A6CA
    PaintSession* allocateSession(Gfx::drawpixelinfo_t& dpi, uint16_t viewportFlags)
    {
        _sessionDpi = &dpi;
        _sessionNextFreePaintStruct = &_sessionPaintEntries[0];
        _sessionEndOfPaintStructArray = &_sessionPaintEntries[3998];
        _sessionLastRootPS = nullptr;
        for (auto& quadrant : _sessionQuadrants)
        {
            quadrant = nullptr;
        }
        _sessionQuadrantBackIndex = -1;
        _sessionQuadrantFrontIndex = 0;
        _sessionLastPaintString = 0;
        _sessionPaintStringHead = 0;
        return &_session;
    }

    // 0x004622A2
    void PaintSession::generate()
    {
        call(0x004622A2);
    }

    // 0x0045E7B5
    void PaintSession::arrangeStructs()
    {
        call(0x0045E7B5);
    }

    // 0x0045ED91
    [[nodiscard]] InteractionArg PaintSession::getNormalInteractionInfo(const uint32_t flags)
    {
        _sessionInteractionInfoType = InteractionItem::t_0;
        _getMapCoordinatesFromPosFlags = flags;
        call(0x0045ED91);
        return InteractionArg{ _sessionInteractionInfoX, _sessionInteractionInfoY, { _sessionInteractionInfoValue }, _sessionInteractionInfoType, _sessionInteractionInfoBh };
    }

    // 0x0048DDE4
    [[nodiscard]] InteractionArg PaintSession::getStationNameInteractionInfo(const uint32_t flags)
    {
        _getMapCoordinatesFromPosFlags = flags;
        registers regs;
        regs.edi = reinterpret_cast<uint32_t>(*_sessionDpi);
        call(0x0048DDE4, regs);
        return InteractionArg{ _sessionInteractionInfoX, _sessionInteractionInfoY, { _sessionInteractionInfoValue }, _sessionInteractionInfoType, _sessionInteractionInfoBh };
    }

    // 0x0049773D
    [[nodiscard]] InteractionArg PaintSession::getTownNameInteractionInfo(const uint32_t flags)
    {
        _getMapCoordinatesFromPosFlags = flags;
        registers regs;
        regs.edi = reinterpret_cast<uint32_t>(*_sessionDpi);
        call(0x0049773D, regs);
        return InteractionArg{ _sessionInteractionInfoX, _sessionInteractionInfoY, { _sessionInteractionInfoValue }, _sessionInteractionInfoType, _sessionInteractionInfoBh };
    }
}
