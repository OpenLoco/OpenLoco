#pragma once
#include "../Types.hpp"

namespace OpenLoco::UI::ViewportInteraction
{
    enum class InteractionItem : uint8_t;
}

namespace OpenLoco::Map
{
    struct tile_element;
}
namespace OpenLoco
{
    struct Thing;
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
#pragma pack(pop)
}