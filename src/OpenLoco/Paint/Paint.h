#pragma once
#include "../Interop/Interop.hpp"
#include "../Types.hpp"

namespace OpenLoco::Map
{
    struct tile_element;
    struct map_pos;
}
namespace OpenLoco
{
    struct Thing;
}

namespace OpenLoco::Ui::ViewportInteraction
{
    struct InteractionArg;
    enum class InteractionItem : uint8_t;
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
        Ui::ViewportInteraction::InteractionItem type; // 0x28
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
    struct GenerationParameters;

    struct PaintSession
    {
    public:
        void generate();
        void arrangeStructs();
        void init(Gfx::drawpixelinfo_t& dpi, const uint16_t viewportFlags);
        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getNormalInteractionInfo(const uint32_t flags);
        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getStationNameInteractionInfo(const uint32_t flags);
        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getTownNameInteractionInfo(const uint32_t flags);
        Gfx::drawpixelinfo_t* getContext() { return _dpi; }

    private:
        void generateTilesAndEntities(GenerationParameters&& p);

        inline static Interop::loco_global<Gfx::drawpixelinfo_t*, 0x00E0C3E0> _dpi;
        inline static Interop::loco_global<PaintEntry[4000], 0x00E0C410> _paintEntries;
        inline static Interop::loco_global<PaintStruct* [1024], 0x00E3F0C0> _quadrants;
        inline static Interop::loco_global<uint32_t, 0x00E400C0> _quadrantBackIndex;
        inline static Interop::loco_global<uint32_t, 0x00E400C4> _quadrantFrontIndex;
        inline static Interop::loco_global<const void*, 0x00E4F0B4> _currentlyDrawnItem;
        inline static Interop::loco_global<PaintEntry*, 0x00E0C404> _endOfPaintStructArray;
        inline static Interop::loco_global<PaintEntry*, 0x00E0C40C> _nextFreePaintStruct;
        inline static Interop::loco_global<coord_t, 0x00E3F090> _spritePositionX;
        inline static Interop::loco_global<coord_t, 0x00E3F096> _spritePositionY;
        inline static Interop::loco_global<PaintStruct*, 0x00E40120> _lastPS;
        inline static Interop::loco_global<Ui::ViewportInteraction::InteractionItem, 0x00E3F0AC> _type;
        inline static Interop::loco_global<PaintStringStruct*, 0x00E40118> _paintStringHead;
        inline static Interop::loco_global<PaintStringStruct*, 0x00E4011C> _lastPaintString;
        inline static Interop::loco_global<Map::map_pos, 0x00E3F0B0> _mapPosition;
        uint8_t currentRotation; // new field set from 0x00E3F0B8 but split out into this struct as seperate item

        // From OpenRCT2 equivalent fields not found yet or new
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

    PaintSession* allocateSession(Gfx::drawpixelinfo_t& dpi, const uint16_t viewportFlags);

    void registerHooks();
}
