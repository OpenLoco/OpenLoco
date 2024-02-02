#pragma once
#include "Graphics/ImageId.h"
#include "Types.hpp"
#include "Viewport.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/Ui/Point.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <span>
#include <variant>

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
        x0y0 = 1U << 0, // 0: (x:0, y:0)
        x2y0 = 1U << 1, // 2: (x:2, y:0)
        x0y2 = 1U << 2, // 6: (x:0, y:2)
        x2y2 = 1U << 3, // 8: (x:2, y:2)
        x1y1 = 1U << 4, // 4: (x:1, y:1)
        x1y0 = 1U << 5, // 1: (x:1, y:0)
        x0y1 = 1U << 6, // 3: (x:0, y:1)
        x2y1 = 1U << 7, // 5: (x:2, y:1)
        x1y2 = 1U << 8, // 7: (x:1, y:2)

        all = x0y0 | x2y0 | x0y2 | x2y2 | x1y1 | x1y0 | x0y1 | x2y1 | x1y2,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(SegmentFlags);

    // Handy array for converting a bit index to the flag
    constexpr std::array<SegmentFlags, 9> kSegmentOffsets = { SegmentFlags::x0y0, SegmentFlags::x2y0, SegmentFlags::x0y2, SegmentFlags::x2y2, SegmentFlags::x1y1, SegmentFlags::x1y0, SegmentFlags::x0y1, SegmentFlags::x2y1, SegmentFlags::x1y2 };

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
        World::MicroZ height;
        uint8_t type;
        constexpr bool operator==(const TunnelEntry&) const = default;
    };
    assert_struct_size(TunnelEntry, 2);

    struct BridgeEntry
    {
        int16_t height;        // 0x00525CE4
        uint16_t subType;      // 0x00525CE6
        uint32_t padImage1;    // 0x00525CE8 used only in bridge paint here just to keep struct size
        uint32_t padImage2;    // 0x00525CEC used only in bridge paint here just to keep struct size
        uint8_t edgesQuarters; // 0x00525CF0
        uint8_t objectId;      // 0x00525CF1
        ImageId imageBase;     // 0x00525CF2

        constexpr BridgeEntry() = default;
        constexpr BridgeEntry(coord_t _height, uint8_t _subType, uint8_t edges, uint8_t quarters, uint8_t _objectId, ImageId _imageBase)
            : height(_height)
            , subType(_subType)
            , padImage1(0)
            , padImage2(0)
            , edgesQuarters((edges << 4U) | quarters)
            , objectId(_objectId)
            , imageBase(_imageBase){};

        bool isEmpty() const { return height == -1; }
    };
    assert_struct_size(BridgeEntry, 18);

    constexpr auto kNullBridgeEntry = BridgeEntry(-1, 0, 0, 0, 0, ImageId(0));

    struct TrackRoadAdditionSupports
    {
        int16_t height;                                                     // 0x00F003F4 support height 0 == no supports at all
        SegmentFlags occupiedSegments;                                      // 0x00F003F6 segments that the support can't be placed
        uint32_t segmentImages[9];                                          // 0x00F003F8 0 == no support here
        uint8_t segmentFrequency[9];                                        // 0x00F0041C fewer bits set == higher frequency of supports
        Ui::ViewportInteraction::InteractionItem segmentInteractionType[9]; // 0x00F00425 why isn't mod id set???
        void* segmentInteractionItem[9];                                    // 0x00F0042E
    };
    assert_struct_size(TrackRoadAdditionSupports, 0x5E);

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

    struct TestPaint
    {
        struct Track3
        {
            std::array<std::array<uint32_t, 3>, 4> imageIds;
            std::array<uint8_t, 3> priority;
            std::array<World::Pos3, 4> offsets;
            std::array<World::Pos3, 4> boundingBoxOffsets;
            std::array<World::Pos3, 4> boundingBoxSizes;
            std::array<uint8_t, 4> bridgeEdges;
            std::array<uint8_t, 4> bridgeQuarters;
            std::array<uint8_t, 4> tunnelEdges;
            std::array<SegmentFlags, 4> segments;
            std::array<uint32_t, 4> callOffset;
            std::array<std::array<int8_t, 4>, 4> tunnelHeight;
            std::array<uint8_t, 4> bridgeType;
        };

        struct Track1
        {
            std::array<uint32_t, 4> imageIds;
            std::array<World::Pos3, 4> offsets;
            std::array<World::Pos3, 4> boundingBoxOffsets;
            std::array<World::Pos3, 4> boundingBoxSizes;
            std::array<uint8_t, 4> bridgeEdges;
            std::array<uint8_t, 4> bridgeQuarters;
            std::array<uint8_t, 4> tunnelEdges;
            std::array<SegmentFlags, 4> segments;
            std::array<uint32_t, 4> callOffset;
            std::array<std::array<int8_t, 4>, 4> tunnelHeight;
            std::array<uint8_t, 4> bridgeType;
        };

        using Track = std::variant<std::monostate, Track1, Track3>;
        struct CurrentTileTrack
        {
            int16_t height;
            uint32_t baseImageId;
            uint8_t rotation;
            uint8_t trackId;
            uint8_t index;
            uint8_t callCount;
            uint32_t callOffset;
            bool isTrack;
            Track track;
            std::array<uint8_t, 4> tunnelsCount;
        };

        std::array<std::vector<Track>, 44> tracks;
        CurrentTileTrack currentTileTrack;

        struct TrackAddition
        {
            std::array<uint32_t, 4> imageIds;
            std::array<World::Pos3, 4> offsets;
            std::array<World::Pos3, 4> boundingBoxOffsets;
            std::array<World::Pos3, 4> boundingBoxSizes;
            uint8_t priority;
            uint8_t callType;
            std::array<uint32_t, 4> supportImageId;
            std::array<int16_t, 4> supportHeight;
            std::array<uint8_t, 4> supportFrequency;
            std::array<uint16_t, 4> supportSegment;
            std::array<uint32_t, 4> callOffset;
        };

        struct CurrentTileTrackAddition
        {
            int16_t height;
            uint32_t baseImageId;
            uint8_t rotation;
            uint8_t trackId;
            uint8_t index;
            uint8_t paintStyle;
            uint8_t callCount;
            uint32_t callOffset;
            bool isTrackAddition;
            TrackAddition ta;
        };
        std::array<std::array<std::vector<TrackAddition>, 44>, 2> trackAdditions;
        CurrentTileTrackAddition currentTileTrackAddition;
    };

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
        void setRotation(uint8_t rotation) { currentRotation = rotation; }
        int16_t getMaxHeight() { return _maxHeight; }
        uint32_t get112C300() { return _112C300; }
        int16_t getAdditionSupportHeight() { return (*_trackRoadAdditionSupports).height; }
        const SupportHeight& getGeneralSupportHeight() { return _support; }
        const BridgeEntry& getBridgeEntry() { return _bridgeEntry; }
        SegmentFlags get525CF8() { return _525CF8; }
        SegmentFlags getOccupiedAdditionSuportSegments() { return (*_trackRoadAdditionSupports).occupiedSegments; }
        TrackRoadAdditionSupports& getAdditionSupports() { return _trackRoadAdditionSupports; }
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
        void set525CF8(const SegmentFlags segments) { _525CF8 = segments; }
        void setOccupiedAdditionSupportSegments(const SegmentFlags newValue) { (*_trackRoadAdditionSupports).occupiedSegments = newValue; }
        void setBridgeEntry(const BridgeEntry newValue) { _bridgeEntry = newValue; }
        void resetTileColumn(const Ui::Point& pos);
        void resetTunnels();
        void resetLastPS() { _lastPS = nullptr; }
        void setBoundingBoxOffset(const World::Pos3& bbox) { _boundingBoxOffset = bbox; }
        World::Pos3 getBoundingBoxOffset() const { return _boundingBoxOffset; }
        void finaliseTrackRoadOrdering();
        void finaliseTrackRoadAdditionsOrdering();
        std::span<TunnelEntry> getTunnels(uint8_t edge);
        void insertTunnel(coord_t z, uint8_t tunnelType, uint8_t edge);
        void insertTunnels(const std::array<int16_t, 4>& tunnelHeights, coord_t height, uint8_t tunnelType);

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
        PaintStruct* addToPlotList4FD150(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize);

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
         * @param priority @<ecx>
         * @param offsetZ @<dx>
         * @param boundBoxLengthX @<di>
         * @param boundBoxLengthY @<si>
         * @param boundBoxLengthZ @<ah>
         * @param boundBoxOffsetX @<0xE3F0A0>
         * @param boundBoxOffsetY @<0xE3F0A2>
         * @param boundBoxOffsetZ @<0xE3F0A4>
         */
        PaintStruct* addToPlotListTrackRoad(ImageId imageId, uint32_t priority, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize);

        /*
         * @param rotation @<ebp>
         * @param imageId  @<ebx>
         * @param priority @<ecx>
         * @param offsetZ @<dx>
         * @param boundBoxLengthX @<di>
         * @param boundBoxLengthY @<si>
         * @param boundBoxLengthZ @<ah>
         * @param boundBoxOffsetX @<0xE3F0A0>
         * @param boundBoxOffsetY @<0xE3F0A2>
         * @param boundBoxOffsetZ @<0xE3F0A4>
         */
        PaintStruct* addToPlotListTrackRoadAddition(ImageId imageId, uint32_t priority, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize);

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

        static TestPaint tp;

        static void printTP();

    private:
        void generateTilesAndEntities(GenerationParameters&& p);
        void finaliseOrdering(std::span<PaintStruct*> paintStructs);

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
        inline static Interop::loco_global<PaintStruct* [5], 0x00E400D0> _trackRoadPaintStructs;
        inline static Interop::loco_global<PaintStruct* [2], 0x00E400E4> _trackRoadAdditionsPaintStructs;
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
        inline static Interop::loco_global<TunnelEntry[33], 0x0050C077> _tunnels0; // There are only 32 entries but 33 and -1 are also writable for marking the end/start
        inline static Interop::loco_global<TunnelEntry[33], 0x0050C0BB> _tunnels1; // There are only 32 entries but 33 and -1 are also writable for marking the end/start
        inline static Interop::loco_global<TunnelEntry[33], 0x0050C0FF> _tunnels2; // There are only 32 entries but 33 and -1 are also writable for marking the end/start
        inline static Interop::loco_global<TunnelEntry[33], 0x0050C143> _tunnels3; // There are only 32 entries but 33 and -1 are also writable for marking the end/start
        inline static Interop::loco_global<BridgeEntry, 0x00525CE4> _bridgeEntry;
        inline static Interop::loco_global<SegmentFlags, 0x00525CF8> _525CF8;
        inline static Interop::loco_global<const void*, 0x00E4F0B4> _currentlyDrawnItem;
        inline static Interop::loco_global<int16_t, 0x00F00152> _maxHeight;
        inline static Interop::loco_global<TrackRoadAdditionSupports, 0x00F003F4> _trackRoadAdditionSupports;
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
