#pragma once
#include "../Types.hpp"

namespace OpenLoco::UI::ViewportInteraction
{
    enum class InteractionItem : uint8_t;
}

namespace OpenLoco::Map
{
    struct tile_element;
    struct map_pos;
}
namespace OpenLoco
{
    struct Thing;
}

namespace OpenLoco::Gfx
{
    struct drawpixelinfo_t;
}

namespace OpenLoco::Paint
{

#pragma pack(push, 1)
    /* size 0x12 */
    struct AttachedPaintStruct
    {
        uint32_t imageId; // 0x00
        union
        {
            uint32_t tertiaryColour; // 0x04 maybe unused in loco
            // If masked imageId is maskedId
            uint32_t colourImageId; // 0x04
        };
        int16_t x;     // 0x08
        int16_t y;     // 0x0A
        uint8_t flags; // 0x0C
        uint8_t pad_0D;
        AttachedPaintStruct* next; // 0x0E
    };
    static_assert(sizeof(AttachedPaintStruct) == 0x12);

    struct PaintStringStruct
    {
        string_id stringId;      // 0x00
        PaintStringStruct* next; // 0x02
        int16_t x;               // 0x06
        int16_t y;               // 0x08
        uint16_t args[7];        // 0x0A
        uint8_t pad_18[0x1A - 0x18];
        uint8_t* yOffsets; // 0x1A
        uint16_t colour;   // 0x1E
    };
    static_assert(sizeof(PaintStringStruct) == 0x20);

    struct PaintStructBoundBox
    {
        uint16_t x;    // 0x08
        uint16_t y;    // 0x0A
        uint16_t z;    // 0x0C
        uint16_t xEnd; // 0x0E
        uint16_t yEnd; // 0x10
        uint16_t zEnd; // 0x12
    };

    struct PaintStruct
    {
        uint32_t imageId; // 0x00
        union
        {
            uint32_t tertiaryColour; // 0x04
            // If masked image_id is masked_id
            uint32_t colourImageId; // 0x04
        };
        PaintStructBoundBox bounds; // 0x08
        int16_t x;                  // 0x14
        int16_t y;                  // 0x16
        uint16_t quadrantIndex;
        uint8_t flags;
        uint8_t quadrantFlags;
        AttachedPaintStruct* attachedPS; // 0x1C
        PaintStruct* children;
        PaintStruct* nextQuadrantPS;                   // 0x24
        UI::ViewportInteraction::InteractionItem type; // 0x28
        uint8_t var_29;
        uint16_t pad_2A;
        coord_t map_x; // 0x2C
        coord_t map_y; // 0x2E
        union
        {
            Map::tile_element* tileElement; // 0x30 (or thing pointer)
            Thing* thing;                   // 0x30
        };
    };
    static_assert(sizeof(PaintStruct) == 0x34);

    union PaintEntry
    {
        PaintStruct basic;
        AttachedPaintStruct attached;
        PaintStringStruct string;
    };
    static_assert(sizeof(PaintEntry) == 0x34);
#pragma pack(pop)

    struct PaintSession
    {
    public:
        Gfx::drawpixelinfo_t* dpi();
        PaintEntry* allocateEntry();
        void setQuadrant(PaintEntry& p);

    private:
        //Gfx::drawpixelinfo_t* dpi;                     // 0xE0C3E0
        //PaintEntry paintStructs[4000];                 // 0xE0C410 -> 0xE3F050
        //PaintStruct* quadrants[1024];                  // 0xE3F0C0 -> 0xE400C0
        //uint32_t quadrantBackIndex;                    // 0xE400C0
        //uint32_t quadrantFrontIndex;                   // 0xE400C4
        //const void* currentlyDrawnItem;                // 0xE4F0B4
        //PaintEntry* endOfPaintStructArray;             // 0xE0C404
        //PaintEntry* nextFreePaintStruct;               // 0xE0C40C
        //Map::map_pos spritePosition;                   // 0xE3F090 and 0xE3F096
        //PaintStruct* lastRootPS;                       // 0xE40120
        //UI::ViewportInteraction::InteractionItem type; // 0xE3F0AC
        //PaintStringStruct* paintStringHead;            // 0xE40118
        //PaintStringStruct* lastPaintString;            // 0xE4011C
        //Map::map_pos mapPosition;                      // 0xE3F0B0

        // From OpenRCT2 equivalent fields not found yet or new
        //uint8_t currentRotation;                     // new field
        //PaintStruct paintHead;                       // new field
        //uint32_t viewFlags;                          // new field might not be needed tbc
        //AttachedPaintStruct* unkF1AD2C;              // no equivalent
        //support_height supportSegments[9];
        //support_height support;
        //PaintStruct* woodenSupportsPrependTo;
        //tunnel_entry leftTunnels[TUNNEL_MAX_COUNT];
        //uint8_t leftTunnelCount;
        //tunnel_entry rightTunnels[TUNNEL_MAX_COUNT];
        //uint8_t rightTunnelCount;
        //uint8_t verticalTunnelHeight;
        //const Map::tile_element* surfaceElement;
        //Map::tile_element* pathElementOnSameHeight;
        //Map::tile_element* trackElementOnSameHeight;
        //bool didPassSurface;
        //uint8_t unk141E9DB;
        //uint16_t waterHeight;
        //uint32_t trackColours[4];
    };

    PaintSession* allocateSession(Gfx::drawpixelinfo_t& dpi, uint16_t viewportFlags);
}
