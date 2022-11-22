#pragma once
#include "Graphics/ImageId.h"
#include "Interop/Interop.hpp"
#include "Map/Map.hpp"
#include "Types.hpp"
#include "Ui/Types.hpp"

namespace OpenLoco::Map
{
    struct TileElement;
}
namespace OpenLoco
{
    struct EntityBase;
}

namespace OpenLoco::Ui::ViewportInteraction
{
    struct InteractionArg;
    enum class InteractionItem : uint8_t;
}

namespace OpenLoco::Gfx
{
    struct RenderTarget;
}

namespace OpenLoco::Paint
{
    namespace Segment
    {
        constexpr uint16_t _58 = (1 << 0); // TBC: 0
        constexpr uint16_t _5C = (1 << 1); // TBC: 6
        constexpr uint16_t _60 = (1 << 2); // TBC: 2
        constexpr uint16_t _64 = (1 << 3); // TBC: 8
        constexpr uint16_t _68 = (1 << 4); // TBC: 3
        constexpr uint16_t _6C = (1 << 5); // TBC: 7
        constexpr uint16_t _70 = (1 << 6); // TBC: 1
        constexpr uint16_t _74 = (1 << 7); // TBC: 5
        constexpr uint16_t _78 = (1 << 8); // TBC: 4

        constexpr uint16_t all = _58 | _5C | _60 | _64 | _68 | _6C | _70 | _74 | _78;
    }

    // Used by both AttachedPaintStruct and PaintStruct
    namespace PaintStructFlags
    {
        constexpr uint8_t hasMaskedImage = (1 << 0);
    }

#pragma pack(push, 1)
    /* size 0x12 */
    struct AttachedPaintStruct
    {
        ImageId imageId;       // 0x00
        ImageId maskedImageId; // 0x04
        Ui::Point vpPos;       // 0x08
        uint8_t flags;         // 0x0C
        uint8_t pad_0D;
        AttachedPaintStruct* next; // 0x0E
    };
    assert_struct_size(AttachedPaintStruct, 0x12);

    struct PaintStringStruct
    {
        string_id stringId;      // 0x00
        PaintStringStruct* next; // 0x02
        Ui::Point vpPos;         // 0x06
        uint16_t args[8];        // 0x0A
        const int8_t* yOffsets;  // 0x1A
        uint16_t colour;         // 0x1E
    };
    assert_struct_size(PaintStringStruct, 0x20);

    struct PaintStructBoundBox
    {
        int16_t x;    // 0x08
        int16_t y;    // 0x0A
        int16_t z;    // 0x0C
        int16_t zEnd; // 0x0E NOTE: This order is important!
        int16_t xEnd; // 0x10
        int16_t yEnd; // 0x12
    };

    namespace QuadrantFlags
    {
        constexpr uint8_t pendingVisit = (1 << 0);
        constexpr uint8_t outsideQuadrant = (1 << 7);
        constexpr uint8_t neighbour = (1 << 1);
    };

    struct PaintStruct
    {
        ImageId imageId;                               // 0x00
        ImageId maskedImageId;                         // 0x04
        PaintStructBoundBox bounds;                    // 0x08
        Ui::Point vpPos;                               // 0x14
        uint16_t quadrantIndex;                        // 0x18
        uint8_t flags;                                 // 0x1A
        uint8_t quadrantFlags;                         // 0x1B
        AttachedPaintStruct* attachedPS;               // 0x1C
        PaintStruct* children;                         // 0x20
        PaintStruct* nextQuadrantPS;                   // 0x24
        Ui::ViewportInteraction::InteractionItem type; // 0x28
        uint8_t modId;                                 // 0x29 used for track mods and signal sides
        uint16_t pad_2A;
        Map::Pos2 mapPos; // 0x2C
        union
        {
            Map::TileElement* tileElement; // 0x30 (or entity pointer)
            EntityBase* entity;            // 0x30
        };
    };
    assert_struct_size(PaintStruct, 0x34);

    union PaintEntry
    {
        PaintStruct basic;
        AttachedPaintStruct attached;
        PaintStringStruct string;
    };
    assert_struct_size(PaintEntry, 0x34);

    struct SupportHeight
    {
        uint16_t height;
        uint8_t slope;
        uint8_t var_03; // Used by general support height only
    };
    assert_struct_size(SupportHeight, 4);

    struct TunnelEntry
    {
        uint8_t height;
        uint8_t type;
    };
    assert_struct_size(TunnelEntry, 2);
#pragma pack(pop)
    struct GenerationParameters;

    struct SessionOptions
    {
        uint8_t rotation;
        int16_t foregroundCullHeight;
        uint16_t viewFlags;
    };

    static constexpr auto kMaxPaintQuadrants = 1024;

    struct PaintSession
    {
    public:
        void generate();
        void arrangeStructs();
        void drawStructs();
        void drawStringStructs();
        void init(Gfx::RenderTarget& rt, const SessionOptions& options);
        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getNormalInteractionInfo(const uint32_t flags);
        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getStationNameInteractionInfo(const uint32_t flags);
        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getTownNameInteractionInfo(const uint32_t flags);
        Gfx::RenderTarget* getRenderTarget() { return _renderTarget; }
        uint8_t getRotation() { return currentRotation; }
        int16_t getMaxHeight() { return _maxHeight; }
        uint32_t get112C300() { return _112C300; }
        uint16_t getF003F4() { return _F003F4; }
        const SupportHeight& getGeneralSupportHeight() { return _support; }
        uint16_t get525CE4(const uint8_t i) { return _525CE4[i]; }
        uint16_t get525CF8() { return _525CF8; }
        Map::Pos2 getUnkPosition()
        {
            return Map::Pos2{ _unkPositionX, _unkPositionY };
        }
        Map::Pos2 getSpritePosition()
        {
            return Map::Pos2{ _spritePositionX, _spritePositionY };
        }
        uint16_t getViewFlags() { return _viewFlags; }
        // TileElement or Entity
        void setCurrentItem(void* item) { _currentItem = item; }
        void setItemType(const Ui::ViewportInteraction::InteractionItem type) { _itemType = type; }
        void setTrackModId(const uint8_t mod) { _trackModId = mod; }
        void setEntityPosition(const Map::Pos2& pos);
        void setMapPosition(const Map::Pos2& pos);
        void setUnkPosition(const Map::Pos2& pos);
        void setVpPosition(const Ui::Point& pos);
        void setUnkVpY(const uint16_t y) { _unkVpPositionY = y; }
        void setSegmentSupportHeight(const uint16_t segments, const uint16_t height, const uint8_t slope);
        void setGeneralSupportHeight(const uint16_t height, const uint8_t slope);
        void setMaxHeight(const Map::Pos2& loc);
        void set525CF8(const uint16_t segments) { _525CF8 = segments; }
        void set525CF0(const uint8_t newValue) { _525CF0 = newValue; }
        void setF003F6(const uint16_t newValue) { _F003F6 = newValue; }
        void set525CE4(const uint8_t i, const uint16_t newValue) { _525CE4[i] = newValue; }
        void resetTileColumn(const Ui::Point& pos);
        void resetTunnels();
        void resetLastPS() { _lastPS = nullptr; }

        /*
         * @param amount    @<eax>
         * @param stringId  @<bx>
         * @param z         @<dx>
         * @param xOffset   @<si>
         * @param yOffsets  @<edi>
         * @param rotation  @<ebp>
         * @param colour    @<0xE3F0A8>
         */
        PaintStringStruct* addToStringPlotList(const uint32_t amount, const string_id stringId, const uint16_t z, const int16_t xOffset, const int8_t* yOffsets, const uint16_t colour);

        /*
         * @param rotation @<ebp>
         * @param imageId  @<ebx>
         * @param offsetX @<al>
         * @param offsetY @<cl>
         * @param offsetZ @<dx>
         * @param boundBoxLengthX @<di>
         * @param boundBoxLengthY @<si>
         * @param boundBoxLengthZ @<ah>
         */
        PaintStruct* addToPlotListAsParent(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxSize);

        /*
         * @param rotation @<ebp>
         * @param imageId  @<ebx>
         * @param offsetX @<al>
         * @param offsetY @<cl>
         * @param offsetZ @<dx>
         * @param boundBoxLengthX @<di>
         * @param boundBoxLengthY @<si>
         * @param boundBoxLengthZ @<ah>
         * @param boundBoxOffsetX @<0xE3F0A0>
         * @param boundBoxOffsetY @<0xE3F0A2>
         * @param boundBoxOffsetZ @<0xE3F0A4>
         */
        PaintStruct* addToPlotListAsParent(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize);

        /*
         * @param rotation @<ebp>
         * @param imageId  @<ebx>
         * @param offsetZ @<dx>
         * @param boundBoxLengthX @<di>
         * @param boundBoxLengthY @<si>
         * @param boundBoxLengthZ @<ah>
         * @param boundBoxOffsetX @<0xE3F0A0>
         * @param boundBoxOffsetY @<0xE3F0A2>
         * @param boundBoxOffsetZ @<0xE3F0A4>
         */
        void addToPlotList4FD150(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize);

        /*
         * @param rotation @<ebp>
         * @param imageId  @<ebx>
         * @param offsetX @<al>
         * @param offsetY @<cl>
         * @param offsetZ @<dx>
         * @param boundBoxLengthX @<di>
         * @param boundBoxLengthY @<si>
         * @param boundBoxLengthZ @<ah>
         * @param boundBoxOffsetX @<0xE3F0A0>
         * @param boundBoxOffsetY @<0xE3F0A2>
         * @param boundBoxOffsetZ @<0xE3F0A4>
         */
        PaintStruct* addToPlotListAsChild(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize);

        /*
         * @param rotation @<ebp>
         * @param imageId  @<ebx>
         * @param ecx @<ecx>
         * @param offsetZ @<dx>
         * @param boundBoxLengthX @<di>
         * @param boundBoxLengthY @<si>
         * @param boundBoxLengthZ @<ah>
         * @param boundBoxOffsetX @<0xE3F0A0>
         * @param boundBoxOffsetY @<0xE3F0A2>
         * @param boundBoxOffsetZ @<0xE3F0A4>
         */
        void addToPlotList4FD180(ImageId imageId, uint32_t ecx, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize);

        /*
         * @param rotation @<ebp>
         * @param imageId  @<ebx>
         * @param offsetX @<al>
         * @param offsetY @<cl>
         * @param offsetZ @<dx>
         * @param boundBoxLengthX @<di>
         * @param boundBoxLengthY @<si>
         * @param boundBoxLengthZ @<ah>
         * @param boundBoxOffsetX @<0xE3F0A0>
         * @param boundBoxOffsetY @<0xE3F0A2>
         * @param boundBoxOffsetZ @<0xE3F0A4>
         */
        void addToPlotList4FD200(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize);
        /*
         * @param imageId @<ebx>
         * @param offsetX @<ax>
         * @param offsetY @<cx>
         */
        AttachedPaintStruct* attachToPrevious(ImageId imageId, const Ui::Point& offset);

    private:
        void generateTilesAndEntities(GenerationParameters&& p);

        inline static Interop::loco_global<uint8_t[4], 0x0050C185> _tunnelCounts;
        inline static Interop::loco_global<TunnelEntry[32], 0x0050C077> _tunnels0;
        inline static Interop::loco_global<TunnelEntry[32], 0x0050C0BB> _tunnels1;
        inline static Interop::loco_global<TunnelEntry[32], 0x0050C0FF> _tunnels2;
        inline static Interop::loco_global<TunnelEntry[32], 0x0050C143> _tunnels3;
        inline static Interop::loco_global<uint16_t[2], 0x00525CE4> _525CE4;
        inline static Interop::loco_global<uint8_t, 0x00525CF0> _525CF0;
        inline static Interop::loco_global<uint16_t, 0x00525CF8> _525CF8;
        inline static Interop::loco_global<Gfx::RenderTarget*, 0x00E0C3E0> _renderTarget;
        inline static Interop::loco_global<PaintEntry*, 0x00E0C404> _endOfPaintStructArray;
        inline static Interop::loco_global<PaintEntry*, 0x00E0C408> _paintHead;
        inline static Interop::loco_global<PaintEntry*, 0x00E0C40C> _nextFreePaintStruct;
        inline static Interop::loco_global<PaintEntry[4000], 0x00E0C410> _paintEntries;
        inline static Interop::loco_global<coord_t, 0x00E3F090> _spritePositionX;
        inline static Interop::loco_global<coord_t, 0x00E3F092> _unkPositionX;
        inline static Interop::loco_global<int16_t, 0x00E3F094> _vpPositionX;
        inline static Interop::loco_global<coord_t, 0x00E3F096> _spritePositionY;
        inline static Interop::loco_global<coord_t, 0x00E3F098> _unkPositionY;
        inline static Interop::loco_global<int16_t, 0x00E3F09A> _vpPositionY;
        inline static Interop::loco_global<int16_t, 0x00E3F09C> _unkVpPositionY;
        inline static Interop::loco_global<bool, 0x00E3F09E> _didPassSurface;
        inline static Interop::loco_global<Ui::ViewportInteraction::InteractionItem, 0x00E3F0AC> _itemType;
        inline static Interop::loco_global<uint8_t, 0x00E3F0AD> _trackModId;
        inline static Interop::loco_global<Map::Pos2, 0x00E3F0B0> _mapPosition;
        inline static Interop::loco_global<void*, 0x00E3F0B4> _currentItem;
        inline static Interop::loco_global<PaintStruct* [kMaxPaintQuadrants], 0x00E3F0C0> _quadrants;
        inline static Interop::loco_global<uint32_t, 0x00E400C0> _quadrantBackIndex;
        inline static Interop::loco_global<uint32_t, 0x00E400C4> _quadrantFrontIndex;
        inline static Interop::loco_global<PaintEntry* [2], 0x00E400E4> _E400E4;
        inline static Interop::loco_global<PaintEntry* [5], 0x00E400D0> _E400D0;
        inline static Interop::loco_global<PaintStringStruct*, 0x00E40118> _paintStringHead;
        inline static Interop::loco_global<PaintStringStruct*, 0x00E4011C> _lastPaintString;
        inline static Interop::loco_global<PaintStruct*, 0x00E40120> _lastPS;
        inline static Interop::loco_global<const void*, 0x00E4F0B4> _currentlyDrawnItem;
        inline static Interop::loco_global<int16_t, 0x00F00152> _maxHeight;
        inline static Interop::loco_global<uint16_t, 0x00F003F4> _F003F4;
        inline static Interop::loco_global<uint16_t, 0x00F003F6> _F003F6;
        inline static Interop::loco_global<uint32_t[9], 0x00F003F8> _unkSegments;
        inline static Interop::loco_global<SupportHeight[9], 0x00F00458> _supportSegments;
        inline static Interop::loco_global<SupportHeight, 0x00F0047C> _support;
        inline static Interop::loco_global<int16_t, 0x00F00480> _waterHeight;
        inline static Interop::loco_global<uint32_t, 0x0112C300> _112C300;
        inline static Interop::loco_global<uint16_t, 0x0112C306> _112C306;
        inline static Interop::loco_global<uint16_t, 0x00E3F0BC> _viewFlags;
        uint8_t currentRotation; // new field set from 0x00E3F0B8 but split out into this struct as seperate item

        // From OpenRCT2 equivalent fields not found yet or new
        // AttachedPaintStruct* unkF1AD2C;              // no equivalent
        // PaintStruct* woodenSupportsPrependTo;
        // uint8_t verticalTunnelHeight;
        // const Map::TileElement* surfaceElement;
        // Map::TileElement* pathElementOnSameHeight;
        // Map::TileElement* trackElementOnSameHeight;
        // uint8_t unk141E9DB;
        // uint32_t trackColours[4];
        template<typename T>
        T* allocatePaintStruct()
        {
            auto* ps = *_nextFreePaintStruct;
            if (ps >= *_endOfPaintStructArray)
            {
                return nullptr;
            }
            *_nextFreePaintStruct = reinterpret_cast<PaintEntry*>(reinterpret_cast<uintptr_t>(*_nextFreePaintStruct) + sizeof(T));
            auto* specificPs = reinterpret_cast<T*>(ps);
            *specificPs = {}; // Zero out the struct
            return specificPs;
        }
        void attachStringStruct(PaintStringStruct& psString);
        void addPSToQuadrant(PaintStruct& ps);
        PaintStruct* createNormalPaintStruct(ImageId imageId, const Map::Pos3& offset, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize);
    };

    PaintSession* allocateSession(Gfx::RenderTarget& rt, const SessionOptions& options);

    void registerHooks();
}
