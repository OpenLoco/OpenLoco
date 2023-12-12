#pragma once
#include "Graphics/ImageId.h"
#include "Types.hpp"
#include "Viewport.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/Ui/Point.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco::World
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
    enum class InteractionItemFlags : uint32_t;
}

namespace OpenLoco::Gfx
{
    struct RenderTarget;
}

namespace OpenLoco::Paint
{
    enum class SegmentFlags : uint16_t
    {
        none = 0U,
        _58 = 1U << 0, // TBC: 0
        _5C = 1U << 1, // TBC: 6
        _60 = 1U << 2, // TBC: 2
        _64 = 1U << 3, // TBC: 8
        _68 = 1U << 4, // TBC: 3
        _6C = 1U << 5, // TBC: 7
        _70 = 1U << 6, // TBC: 1
        _74 = 1U << 7, // TBC: 5
        _78 = 1U << 8, // TBC: 4

        all = _58 | _5C | _60 | _64 | _68 | _6C | _70 | _74 | _78,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(SegmentFlags);

    // Used by both AttachedPaintStruct and PaintStruct
    enum class PaintStructFlags : uint8_t
    {
        none = 0U,
        hasMaskedImage = 1U << 0,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(PaintStructFlags);

#pragma pack(push, 1)
    /* size 0x12 */
    struct AttachedPaintStruct
    {
        ImageId imageId;        // 0x00
        ImageId maskedImageId;  // 0x04
        Ui::Point vpPos;        // 0x08
        PaintStructFlags flags; // 0x0C
        uint8_t pad_0D;
        AttachedPaintStruct* next; // 0x0E
    };
    assert_struct_size(AttachedPaintStruct, 0x12);

    struct PaintStringStruct
    {
        StringId stringId;       // 0x00
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

    enum class QuadrantFlags : uint8_t
    {
        none = 0U,
        pendingVisit = 1U << 0,
        outsideQuadrant = 1U << 7,
        neighbour = 1U << 1,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(QuadrantFlags);

    struct PaintStruct
    {
        ImageId imageId;                               // 0x00
        ImageId maskedImageId;                         // 0x04
        PaintStructBoundBox bounds;                    // 0x08
        Ui::Point vpPos;                               // 0x14
        uint16_t quadrantIndex;                        // 0x18
        PaintStructFlags flags;                        // 0x1A
        QuadrantFlags quadrantFlags;                   // 0x1B
        AttachedPaintStruct* attachedPS;               // 0x1C
        PaintStruct* children;                         // 0x20
        PaintStruct* nextQuadrantPS;                   // 0x24
        Ui::ViewportInteraction::InteractionItem type; // 0x28
        uint8_t modId;                                 // 0x29 used for track mods and signal sides
        uint16_t pad_2A;
        World::Pos2 mapPos; // 0x2C
        union
        {
            World::TileElement* tileElement; // 0x30 (or entity pointer)
            EntityBase* entity;              // 0x30
        };
        constexpr bool hasQuadrantFlags(QuadrantFlags flagsToTest) const
        {
            return (quadrantFlags & flagsToTest) != QuadrantFlags::none;
        }
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
        Ui::ViewportFlags viewFlags;
        constexpr bool hasFlags(Ui::ViewportFlags flagsToTest) const
        {
            return (viewFlags & flagsToTest) != Ui::ViewportFlags::none;
        }
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
        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getNormalInteractionInfo(const Ui::ViewportInteraction::InteractionItemFlags flags);
        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getStationNameInteractionInfo(const Ui::ViewportInteraction::InteractionItemFlags flags);
        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getTownNameInteractionInfo(const Ui::ViewportInteraction::InteractionItemFlags flags);
        Gfx::RenderTarget* getRenderTarget() { return _renderTarget; }
        uint8_t getRotation() { return currentRotation; }
        int16_t getMaxHeight() { return _maxHeight; }
        uint32_t get112C300() { return _112C300; }
        uint16_t getF003F4() { return _F003F4; }
        const SupportHeight& getGeneralSupportHeight() { return _support; }
        uint16_t get525CE4(const uint8_t i) { return _525CE4[i]; }
        uint16_t get525CF8() { return _525CF8; }
        World::Pos2 getUnkPosition()
        {
            return World::Pos2{ _unkPositionX, _unkPositionY };
        }
        World::Pos2 getSpritePosition()
        {
            return World::Pos2{ _spritePositionX, _spritePositionY };
        }
        Ui::ViewportFlags getViewFlags() { return _viewFlags; }
        // TileElement or Entity
        void setCurrentItem(void* item) { _currentItem = item; }
        void setItemType(const Ui::ViewportInteraction::InteractionItem type) { _itemType = type; }
        void setTrackModId(const uint8_t mod) { _trackModId = mod; }
        void setEntityPosition(const World::Pos2& pos);
        void setMapPosition(const World::Pos2& pos);
        void setUnkPosition(const World::Pos2& pos);
        void setVpPosition(const Ui::Point& pos);
        void setUnkVpY(const uint16_t y) { _unkVpPositionY = y; }
        void setSegmentSupportHeight(const SegmentFlags segments, const uint16_t height, const uint8_t slope);
        void setGeneralSupportHeight(const uint16_t height, const uint8_t slope);
        void setMaxHeight(const World::Pos2& loc);
        void set525CF8(const uint16_t segments) { _525CF8 = segments; }
        void set525CF0(const uint8_t newValue) { _525CF0 = newValue; }
        void setF003F6(const uint16_t newValue) { _F003F6 = newValue; }
        void set525CE4(const uint8_t i, const uint16_t newValue) { _525CE4[i] = newValue; }
        void resetTileColumn(const Ui::Point& pos);
        void resetTunnels();
        void resetLastPS() { _lastPS = nullptr; }
        void setBoundingBoxOffset(const World::Pos3& bbox) { _boundingBoxOffset = bbox; }
        World::Pos3 getBoundingBoxOffset() { return _boundingBoxOffset; }

        /*
         * @param amount    @<eax>
         * @param stringId  @<bx>
         * @param z         @<dx>
         * @param xOffset   @<si>
         * @param yOffsets  @<edi>
         * @param rotation  @<ebp>
         * @param colour    @<0xE3F0A8>
         */
        PaintStringStruct* addToStringPlotList(const uint32_t amount, const StringId stringId, const uint16_t z, const int16_t xOffset, const int8_t* yOffsets, const uint16_t colour);

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
        PaintStruct* addToPlotListAsParent(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxSize);

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
        PaintStruct* addToPlotListAsParent(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize);

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
        void addToPlotList4FD150(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize);

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
        PaintStruct* addToPlotListAsChild(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize);

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
        void addToPlotList4FD180(ImageId imageId, uint32_t ecx, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize);

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
        void addToPlotList4FD200(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize);
        /*
         * @param imageId @<ebx>
         * @param offsetX @<ax>
         * @param offsetY @<cx>
         */
        AttachedPaintStruct* attachToPrevious(ImageId imageId, const Ui::Point& offset);

    private:
        void generateTilesAndEntities(GenerationParameters&& p);

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
        inline static Interop::loco_global<World::Pos3, 0x00E3F0A0> _boundingBoxOffset;
        inline static Interop::loco_global<int16_t, 0x00E3F0A6> _foregroundCullingHeight;
        inline static Interop::loco_global<Ui::ViewportInteraction::InteractionItem, 0x00E3F0AC> _itemType;
        inline static Interop::loco_global<uint8_t, 0x00E3F0AD> _trackModId;
        // 2 byte align.
        inline static Interop::loco_global<World::Pos2, 0x00E3F0B0> _mapPosition;
        inline static Interop::loco_global<void*, 0x00E3F0B4> _currentItem;
        uint8_t currentRotation; // new field set from 0x00E3F0B8 but split out into this struct as separate item
        inline static Interop::loco_global<Ui::ViewportFlags, 0x00E3F0BC> _viewFlags;
        // 2 byte align.
        inline static Interop::loco_global<PaintStruct* [kMaxPaintQuadrants], 0x00E3F0C0> _quadrants;
        inline static Interop::loco_global<uint32_t, 0x00E400C0> _quadrantBackIndex;
        inline static Interop::loco_global<uint32_t, 0x00E400C4> _quadrantFrontIndex;
        inline static Interop::loco_global<PaintStruct*, 0x00E400C8> _savedPSCur;  // Unused.
        inline static Interop::loco_global<PaintStruct*, 0x00E400CC> _savedPSCur2; // Unused.
        inline static Interop::loco_global<PaintEntry* [5], 0x00E400D0> _trackRoadPaintStructs;
        inline static Interop::loco_global<int32_t, 0x00E400D4> _E400D4;
        inline static Interop::loco_global<int32_t, 0x00E400D8> _E400D8;
        inline static Interop::loco_global<int32_t, 0x00E400DC> _E400DC;
        inline static Interop::loco_global<int32_t, 0x00E400E0> _E400E0;
        inline static Interop::loco_global<PaintEntry* [2], 0x00E400E4> _trackRoadAdditionsPaintStructs;
        inline static Interop::loco_global<int32_t, 0x00E400E8> _E400E8;
        inline static Interop::loco_global<int32_t, 0x00E400EC> _E400EC;
        inline static Interop::loco_global<int16_t, 0x00E400F0> _E400F0;
        inline static Interop::loco_global<int16_t, 0x00E400F2> _E400F2;
        inline static Interop::loco_global<int32_t, 0x00E400F4> _E400F4;
        inline static Interop::loco_global<int32_t, 0x00E400F8> _E400F8;
        inline static Interop::loco_global<int32_t, 0x00E400FC> _E400FC;
        inline static Interop::loco_global<int32_t, 0x00E40100> _E40100;
        inline static Interop::loco_global<int16_t, 0x00E40104> _E40104;
        // 2 byte align
        inline static Interop::loco_global<int32_t, 0x00E40108> _E40108;
        inline static Interop::loco_global<int32_t, 0x00E4010C> _E4010C;
        inline static Interop::loco_global<int32_t, 0x00E40110> _E40110;
        // byte_00E40114 _interactionResult
        // 3 byte align
        inline static Interop::loco_global<PaintStringStruct*, 0x00E40118> _paintStringHead;
        inline static Interop::loco_global<PaintStringStruct*, 0x00E4011C> _lastPaintString;
        inline static Interop::loco_global<PaintStruct*, 0x00E40120> _lastPS;
        // dword_E40124
        // word_E40128
        // word_E4012A
        // 00E4012C dw interactionResult
        // 00E4012E dw interactionCoordY
        // 00E40130 dw interactionCoordX
        // byte_E40132

        // Different globals that don't really belong to PaintSession.
        inline static Interop::loco_global<uint8_t[4], 0x0050C185> _tunnelCounts;
        inline static Interop::loco_global<TunnelEntry[32], 0x0050C077> _tunnels0;
        inline static Interop::loco_global<TunnelEntry[32], 0x0050C0BB> _tunnels1;
        inline static Interop::loco_global<TunnelEntry[32], 0x0050C0FF> _tunnels2;
        inline static Interop::loco_global<TunnelEntry[32], 0x0050C143> _tunnels3;
        inline static Interop::loco_global<uint16_t[2], 0x00525CE4> _525CE4;
        inline static Interop::loco_global<uint8_t, 0x00525CF0> _525CF0;
        inline static Interop::loco_global<uint16_t, 0x00525CF8> _525CF8;
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
        PaintStruct* createNormalPaintStruct(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize);
    };

    PaintSession* allocateSession(Gfx::RenderTarget& rt, const SessionOptions& options);

    void registerHooks();
}
