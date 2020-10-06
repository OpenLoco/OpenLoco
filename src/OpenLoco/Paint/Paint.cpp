#include "Paint.h"
#include "../Interop/Interop.hpp"
#include "../Ui.h"
#include "../Map/Tile.h"

using namespace OpenLoco::Interop;

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
    static loco_global<UI::ViewportInteraction::InteractionItem, 0x00E3F0AC> _sessionType;
    static loco_global<PaintStringStruct*, 0x00E40118> _sessionPaintStringHead;
    static loco_global<PaintStringStruct*, 0x00E4011C> _sessionLastPaintString;
    static loco_global<Map::map_pos, 0x00E3F0B0> _sessionMapPosition;

}