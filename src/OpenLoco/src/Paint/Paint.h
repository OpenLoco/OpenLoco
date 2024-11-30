#pragma once
#include "Graphics/ImageId.h"
#include "Types.hpp"
#include "Viewport.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/Ui/Point.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <array>
#include <sfl/static_vector.hpp>
#include <span>

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

    namespace
    {
        constexpr std::array<std::array<uint8_t, 4>, 4> kSegMap1 = {
            std::array<uint8_t, 4>{ 0, 1, 2, 3 },
            std::array<uint8_t, 4>{ 2, 0, 3, 1 },
            std::array<uint8_t, 4>{ 3, 2, 1, 0 },
            std::array<uint8_t, 4>{ 1, 3, 0, 2 },
        };
        constexpr std::array<std::array<uint8_t, 4>, 4> kSegMap2 = {
            std::array<uint8_t, 4>{ 0, 1, 2, 3 },
            std::array<uint8_t, 4>{ 1, 3, 0, 2 },
            std::array<uint8_t, 4>{ 3, 2, 1, 0 },
            std::array<uint8_t, 4>{ 2, 0, 3, 1 },
        };
    }

    constexpr SegmentFlags rotlSegmentFlags(SegmentFlags val, uint8_t rotation)
    {
        SegmentFlags ret = SegmentFlags::none;
        const auto _val = enumValue(val);
        for (auto i = 0U; i < 4; ++i)
        {
            if (_val & (1U << i))
            {
                ret |= static_cast<SegmentFlags>(1U << kSegMap1[rotation][i]);
            }
        }
        for (auto i = 5U; i < 9; ++i)
        {
            if (_val & (1U << i))
            {
                ret |= static_cast<SegmentFlags>(1U << (kSegMap2[rotation][i - 5] + 5));
            }
        }

        return ret | (val & SegmentFlags::x1y1);
    }

    static_assert(rotlSegmentFlags(SegmentFlags::x0y1 | SegmentFlags::x1y1 | SegmentFlags::x2y1, 0) == (SegmentFlags::x0y1 | SegmentFlags::x1y1 | SegmentFlags::x2y1));

    // Used by both AttachedPaintStruct and PaintStruct
    enum class PaintStructFlags : uint8_t
    {
        none = 0U,
        hasMaskedImage = 1U << 0,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(PaintStructFlags);

    struct AttachedPaintStruct
    {
        AttachedPaintStruct* next;
        ImageId imageId;
        ImageId maskedImageId;
        Ui::Point vpPos;
        PaintStructFlags flags;
    };

    struct PaintStringStruct
    {
        PaintStringStruct* next;
        const int8_t* yOffsets;
        uint16_t args[8];
        Ui::Point vpPos;
        StringId stringId;
        uint16_t colour;
    };

    struct PaintStructBoundBox
    {
        World::Pos3 mins;
        World::Pos3 maxs;
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
        PaintStructBoundBox bounds;
        AttachedPaintStruct* attachedPS;
        PaintStruct* nextQuadrantPS;
        union
        {
            World::TileElement* tileElement;
            EntityBase* entity;
        };
        ImageId imageId;
        ImageId maskedImageId;
        PaintStruct* children;
        Ui::Point vpPos;
        World::Pos2 mapPos;
        uint16_t quadrantIndex;
        uint16_t pad_2A;
        uint8_t modId; // used for track mods and signal sides
        PaintStructFlags flags;
        QuadrantFlags quadrantFlags;
        Ui::ViewportInteraction::InteractionItem type;

        constexpr bool hasQuadrantFlags(QuadrantFlags flagsToTest) const
        {
            return (quadrantFlags & flagsToTest) != QuadrantFlags::none;
        }
    };

    struct SupportHeight
    {
        uint16_t height;
        uint8_t slope;
        uint8_t var_03; // Used by general support height only
    };

    struct TunnelEntry
    {
        World::MicroZ height;
        uint8_t type;
        constexpr bool operator==(const TunnelEntry&) const = default;
    };

    struct BridgeEntry
    {
        ImageId imageBase;     // 0x00525CF2
        uint32_t padImage1;    // 0x00525CE8 used only in bridge paint here just to keep struct size
        uint32_t padImage2;    // 0x00525CEC used only in bridge paint here just to keep struct size
        uint16_t subType;      // 0x00525CE6
        int16_t height;        // 0x00525CE4
        uint8_t edgesQuarters; // 0x00525CF0
        uint8_t objectId;      // 0x00525CF1

        constexpr BridgeEntry() = default;
        constexpr BridgeEntry(coord_t _height, uint8_t _subType, uint8_t edges, uint8_t quarters, uint8_t _objectId, ImageId _imageBase)
            : imageBase(_imageBase)
            , padImage1(0)
            , padImage2(0)
            , subType(_subType)
            , height(_height)
            , edgesQuarters((edges << 4U) | quarters)
            , objectId(_objectId) {};

        bool isEmpty() const { return height == -1; }
    };

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

    static constexpr auto kMaxPaintEntries = 4000U;
    static constexpr auto kMaxPaintQuadrants = 1024;

    struct PaintSession
    {
    public:
        PaintSession(const Gfx::RenderTarget& rt, const SessionOptions& options);

        void generate();
        void arrangeStructs();
        void drawStructs(Gfx::DrawingContext& drawingCtx);
        void drawStringStructs(Gfx::DrawingContext& drawingCtx);

        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getNormalInteractionInfo(const Ui::ViewportInteraction::InteractionItemFlags flags);
        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getStationNameInteractionInfo(const Ui::ViewportInteraction::InteractionItemFlags flags);
        [[nodiscard]] Ui::ViewportInteraction::InteractionArg getTownNameInteractionInfo(const Ui::ViewportInteraction::InteractionItemFlags flags);
        const Gfx::RenderTarget* getRenderTarget() { return _renderTarget; }
        uint8_t getRotation() { return currentRotation; }
        void setRotation(uint8_t rotation) { currentRotation = rotation; }
        int16_t getMaxHeight() { return _maxHeight; }
        uint32_t getRoadExits() { return _roadMergeExits; }
        void setRoadExits(uint32_t value) { _roadMergeExits = value; }
        uint32_t getMergeRoadBaseImage() { return _roadMergeBaseImage; }
        void setMergeRoadBaseImage(uint32_t value) { _roadMergeBaseImage = value; }
        int16_t getMergeRoadHeight() { return _roadMergeHeight; }
        void setMergeRoadHeight(int16_t value) { _roadMergeHeight = value; }
        uint16_t getMergeRoadStreetlight() { return _roadMergeStreetlightType; }
        void setMergeRoadStreetlight(uint16_t value) { _roadMergeStreetlightType = value; }
        int16_t getAdditionSupportHeight() { return _trackRoadAdditionSupports.height; }
        const TrackRoadAdditionSupports& getAdditionSupport() { return _trackRoadAdditionSupports; }
        void setAdditionSupport(const TrackRoadAdditionSupports& newValue) { _trackRoadAdditionSupports = newValue; }
        const SupportHeight& getGeneralSupportHeight() { return _support; }
        const SupportHeight& getSupportHeight(uint8_t segment) { return _supportSegments[segment]; }
        const BridgeEntry& getBridgeEntry() { return _bridgeEntry; }
        SegmentFlags get525CF8() { return _525CF8; }
        int16_t getWaterHeight() { return _waterHeight; }
        int16_t getWaterHeight2() { return _waterHeight2; }
        int16_t getSurfaceHeight() { return _surfaceHeight; }
        uint8_t getSurfaceSlope() { return _surfaceSlope; }
        SegmentFlags getOccupiedAdditionSupportSegments() { return _trackRoadAdditionSupports.occupiedSegments; }
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
        void* getCurrentItem() { return _currentItem; }
        void setItemType(const Ui::ViewportInteraction::InteractionItem type) { _itemType = type; }
        Ui::ViewportInteraction::InteractionItem getItemType() { return _itemType; }
        void setTrackModId(const uint8_t mod) { _trackModId = mod; }
        void setEntityPosition(const World::Pos2& pos);
        void setMapPosition(const World::Pos2& pos);
        void setUnkPosition(const World::Pos2& pos);
        void setVpPosition(const Ui::Point& pos);
        void setUnkVpY(const uint16_t y) { _unkVpPositionY = y; }
        void setSegmentsSupportHeight(const SegmentFlags segments, const uint16_t height, const uint8_t slope);
        void setSegmentSupportHeight(const uint8_t segment, const uint16_t height, const uint8_t slope);
        void setGeneralSupportHeight(const uint16_t height, const uint8_t slope);
        void setMaxHeight(const World::Pos2& loc);
        void set525CF8(const SegmentFlags segments) { _525CF8 = segments; }
        void setOccupiedAdditionSupportSegments(const SegmentFlags newValue) { _trackRoadAdditionSupports.occupiedSegments = newValue; }
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
        void setDidPassSurface(bool value) { _didPassSurface = value; }
        void setSurfaceSlope(uint8_t slope) { _surfaceSlope = slope; }
        void setSurfaceHeight(int16_t height) { _surfaceHeight = height; }
        void setWaterHeight(int16_t height) { _waterHeight = height; }
        void setWaterHeight2(int16_t height) { _waterHeight2 = height; }
        PaintStruct* getLastPS() { return _lastPS; }
        void setLastPS(PaintStruct* ps) { _lastPS = ps; }

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
        PaintStruct* addToPlotList4FD200(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize);
        /*
         * @param imageId @<ebx>
         * @param offsetX @<ax>
         * @param offsetY @<cx>
         */
        AttachedPaintStruct* attachToPrevious(ImageId imageId, const Ui::Point& offset);

    private:
        void generateTilesAndEntities(GenerationParameters&& p);
        void finaliseOrdering(std::span<PaintStruct*> paintStructs);

        union PaintEntry
        {
            PaintStruct basic;
            AttachedPaintStruct attached;
            PaintStringStruct string;

            PaintEntry() {}
        };
        assert_struct_size(PaintEntry, 0x34);

        // Do not null-initialize this, its too expensive, this is storage.
        sfl::static_vector<PaintEntry, kMaxPaintEntries> _paintEntries;

        const Gfx::RenderTarget* _renderTarget{};
        PaintStruct* _paintHead{};
        coord_t _spritePositionX{};
        coord_t _unkPositionX{};
        int16_t _vpPositionX{};
        coord_t _spritePositionY{};
        coord_t _unkPositionY{};
        int16_t _vpPositionY{};
        int16_t _unkVpPositionY{};
        bool _didPassSurface{};
        World::Pos3 _boundingBoxOffset{};
        int16_t _foregroundCullingHeight{};
        Ui::ViewportInteraction::InteractionItem _itemType{};
        uint8_t _trackModId{};
        World::Pos2 _mapPosition{};
        void* _currentItem{};
        uint8_t currentRotation{}; // new field set from 0x00E3F0B8 but split out into this struct as separate item
        Ui::ViewportFlags _viewFlags{};
        std::array<PaintStruct*, kMaxPaintQuadrants> _quadrants;
        uint32_t _quadrantBackIndex;
        uint32_t _quadrantFrontIndex;
        std::array<PaintStruct*, 5> _trackRoadPaintStructs;
        std::array<PaintStruct*, 2> _trackRoadAdditionsPaintStructs;
        int32_t _E400EC{};
        int16_t _E400F0{};
        int16_t _E400F2{};
        int32_t _E400F4{};
        int32_t _E400F8{};
        int32_t _E400FC{};
        int32_t _E40100{};
        int16_t _E40104{};
        int32_t _E40108{};
        int32_t _E4010C{};
        int32_t _E40110{};
        PaintStringStruct* _paintStringHead{};
        PaintStringStruct* _lastPaintString{};
        PaintStruct* _lastPS{};

        // Different globals that don't really belong to PaintSession.
        std::array<uint8_t, 4> _tunnelCounts{};
        std::array<TunnelEntry, 33> _tunnels0{}; // There are only 32 entries but 33 and -1 are also writeable for marking the end/start
        std::array<TunnelEntry, 33> _tunnels1{}; // There are only 32 entries but 33 and -1 are also writeable for marking the end/start
        std::array<TunnelEntry, 33> _tunnels2{}; // There are only 32 entries but 33 and -1 are also writeable for marking the end/start
        std::array<TunnelEntry, 33> _tunnels3{}; // There are only 32 entries but 33 and -1 are also writeable for marking the end/start
        BridgeEntry _bridgeEntry{};
        SegmentFlags _525CF8{};
        const void* _currentlyDrawnItem{};
        int16_t _maxHeight{};
        TrackRoadAdditionSupports _trackRoadAdditionSupports{};
        SupportHeight _supportSegments[9]{};
        SupportHeight _support{};
        int16_t _waterHeight{};
        int16_t _waterHeight2{};
        uint8_t _surfaceSlope{};
        int16_t _surfaceHeight{};
        uint32_t _roadMergeBaseImage{};
        uint32_t _roadMergeExits{};
        int16_t _roadMergeHeight{};
        uint16_t _roadMergeStreetlightType{};

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
            static_assert(std::same_as<T, PaintStruct> || std::same_as<T, AttachedPaintStruct> || std::same_as<T, PaintStringStruct>);

            if (_paintEntries.full())
            {
                return nullptr;
            }

            auto& ps = _paintEntries.emplace_back();

            auto* specificPs = reinterpret_cast<T*>(&ps);
            *specificPs = {}; // Zero out the struct

            return specificPs;
        }
        void attachStringStruct(PaintStringStruct& psString);
        void addPSToQuadrant(PaintStruct& ps);
        PaintStruct* createNormalPaintStruct(ImageId imageId, const World::Pos3& offset, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize);
    };
}
