#include "Paint.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../Ui.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    static loco_global<InteractionItem, 0x00E40104> _sessionInteractionInfoType;
    static loco_global<uint8_t, 0x00E40105> _sessionInteractionInfoBh; // Unk var_29 of paintstruct
    static loco_global<int16_t, 0x00E40108> _sessionInteractionInfoX;
    static loco_global<int16_t, 0x00E4010A> _sessionInteractionInfoY;
    static loco_global<uint32_t, 0x00E4010C> _sessionInteractionInfoValue; // tileElement or thing ptr
    static loco_global<uint32_t, 0x00E40110> _getMapCoordinatesFromPosFlags;
    PaintSession _session;

    void PaintSession::init(Gfx::drawpixelinfo_t& dpi, const uint16_t viewportFlags)
    {
        _dpi = &dpi;
        _nextFreePaintStruct = &_paintEntries[0];
        _endOfPaintStructArray = &_paintEntries[3998];
        _lastPS = nullptr;
        for (auto& quadrant : _quadrants)
        {
            quadrant = nullptr;
        }
        _quadrantBackIndex = -1;
        _quadrantFrontIndex = 0;
        _lastPaintString = 0;
        _paintStringHead = 0;
    }

    // 0x0045A6CA
    PaintSession* allocateSession(Gfx::drawpixelinfo_t& dpi, uint16_t viewportFlags)
    {
        _session.init(dpi, viewportFlags);
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
        regs.edi = reinterpret_cast<uint32_t>(*_dpi);
        call(0x0048DDE4, regs);
        return InteractionArg{ _sessionInteractionInfoX, _sessionInteractionInfoY, { _sessionInteractionInfoValue }, _sessionInteractionInfoType, _sessionInteractionInfoBh };
    }

    // 0x0049773D
    [[nodiscard]] InteractionArg PaintSession::getTownNameInteractionInfo(const uint32_t flags)
    {
        _getMapCoordinatesFromPosFlags = flags;
        registers regs;
        regs.edi = reinterpret_cast<uint32_t>(*_dpi);
        call(0x0049773D, regs);
        return InteractionArg{ _sessionInteractionInfoX, _sessionInteractionInfoY, { _sessionInteractionInfoValue }, _sessionInteractionInfoType, _sessionInteractionInfoBh };
    }
}
