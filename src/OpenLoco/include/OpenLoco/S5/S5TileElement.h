#pragma once

#include <cstdint>
#include <span>

namespace OpenLoco
{
    struct GameState;
}

namespace OpenLoco::World
{
    struct TileElementEntry;
}

namespace OpenLoco::S5
{
    enum class ElementType : uint8_t
    {
        surface,
        track,
        station,
        signal,
        building,
        tree,
        wall,
        road,
        industry,
    };

    static constexpr size_t kTileElementSize = 8;

    namespace ElementFlags
    {
        constexpr uint8_t ghost = 1 << 4;
        constexpr uint8_t aiAllocated = 1 << 5;
        constexpr uint8_t flag_6 = 1 << 6;
        constexpr uint8_t last = 1 << 7;
    }

#pragma pack(push, 1)
    struct TileElementBase
    {
    protected:
        uint8_t _type;
        uint8_t _flags;
        uint8_t _baseZ;
        uint8_t _clearZ;

    public:
        ElementType type() const { return static_cast<ElementType>((_type & 0x3C) >> 2); }
        void setType(ElementType t)
        {
            _type &= ~0x3C;
            _type |= (static_cast<uint8_t>(t) << 2) & 0x3C;
        }

        uint8_t flags() const { return _flags; }
        void setFlags(uint8_t flags) { _flags = flags; }
        uint8_t baseZ() const { return _baseZ; }
        void setBaseZ(uint8_t baseZ) { _baseZ = baseZ; }
        uint8_t clearZ() const { return _clearZ; }
        void setClearZ(uint8_t clearZ) { _clearZ = clearZ; }

        uint8_t occupiedQuarter() const { return _flags & 0xF; }
        void setOccupiedQuarter(uint8_t val)
        {
            _flags &= ~0xF;
            _flags |= val & 0xF;
        }
        bool isGhost() const { return _flags & ElementFlags::ghost; }
        void setGhost(bool state)
        {
            _flags &= ~ElementFlags::ghost;
            _flags |= state ? ElementFlags::ghost : 0;
        }
        bool isAiAllocated() const { return _flags & ElementFlags::aiAllocated; }
        void setAiAllocated(bool state)
        {
            _flags &= ~ElementFlags::aiAllocated;
            _flags |= state ? ElementFlags::aiAllocated : 0;
        }
        bool isFlag6() const { return _flags & ElementFlags::flag_6; }
        void setFlag6(bool state)
        {
            _flags &= ~ElementFlags::flag_6;
            _flags |= state ? ElementFlags::flag_6 : 0;
        }
        bool isLast() const { return (_flags & ElementFlags::last) != 0; }
        void setLast(bool state)
        {
            _flags &= ~ElementFlags::last;
            _flags |= state ? ElementFlags::last : 0;
        }

        std::span<uint8_t> rawData() { return std::span{ reinterpret_cast<uint8_t*>(this), kTileElementSize }; }
        std::span<const uint8_t> rawData() const { return std::span{ reinterpret_cast<const uint8_t*>(this), kTileElementSize }; }

        template<typename T>
        const T* as() const { return type() == T::kElementType ? reinterpret_cast<const T*>(this) : nullptr; }
        template<typename T>
        T* as() { return type() == T::kElementType ? reinterpret_cast<T*>(this) : nullptr; }
    };

    struct TileElement : public TileElementBase
    {
    private:
        uint8_t _pad[4];
    };
    static_assert(sizeof(TileElement) == kTileElementSize);

    struct SurfaceElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::surface;

    private:
        uint8_t _slope;
        uint8_t _water;
        uint8_t _terrain;
        uint8_t _7;

    public:
        uint8_t slope() const { return _slope & 0x1F; }
        void setSlope(uint8_t v) { _slope = (_slope & ~0x1F) | (v & 0x1F); }
        uint8_t snowCoverage() const { return (_slope & 0xE0) >> 5; }
        void setSnowCoverage(uint8_t v) { _slope = (_slope & ~0xE0) | (v << 5); }
        uint8_t water() const { return _water & 0x1F; }
        void setWater(uint8_t v) { _water = (_water & 0xE0) | (v & 0x1F); }
        uint8_t updateTimer() const { return (_water & 0xE0) >> 5; }
        void setUpdateTimer(uint8_t v) { _water = (_water & 0x1F) | ((v << 5) & 0xE0); }
        uint8_t terrain() const { return _terrain & 0x1F; }
        void setTerrain(uint8_t v) { _terrain = (_terrain & ~0x1F) | (v & 0x1F); }
        uint8_t growthStage() const { return _terrain >> 5; }
        void setGrowthStage(uint8_t v) { _terrain = (_terrain & 0x1F) | (v << 5); }
        uint8_t var7() const { return _7; }
        void setVar7(uint8_t v) { _7 = v; }
        bool isIndustrial() const { return _type & 0x80; }
        void setIsIndustrial(bool v) { _type = (_type & ~0x80) | (v ? 0x80 : 0); }
        bool type6Flag() const { return _type & 0x40; }
        void setType6Flag(bool v) { _type = (_type & ~0x40) | (v ? 0x40 : 0); }
    };
    static_assert(sizeof(SurfaceElement) == kTileElementSize);

    struct TrackElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::track;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t rotation() const { return _type & 0x3; }
        void setRotation(uint8_t v) { _type = (_type & ~0x3) | (v & 0x3); }
        bool hasSignal() const { return _type & 0x40; }
        void setHasSignal(bool v) { _type = (_type & ~0x40) | (v ? 0x40 : 0); }
        bool hasStationElement() const { return _type & 0x80; }
        void setHasStationElement(bool v) { _type = (_type & ~0x80) | (v ? 0x80 : 0); }
        uint8_t trackId() const { return _4 & 0x3F; }
        void setTrackId(uint8_t v) { _4 = (_4 & ~0x3F) | (v & 0x3F); }
        bool hasGhostMods() const { return _4 & 0x40; }
        void setHasGhostMods(bool v) { _4 = (_4 & ~0x40) | (v ? 0x40 : 0); }
        bool hasBridge() const { return _4 & 0x80; }
        void setHasBridge(bool v) { _4 = (_4 & ~0x80) | (v ? 0x80 : 0); }
        uint8_t sequenceIndex() const { return _5 & 0xF; }
        void setSequenceIndex(uint8_t v) { _5 = (_5 & ~0xF) | (v & 0xF); }
        uint8_t trackObjectId() const { return _5 >> 4; }
        void setTrackObjectId(uint8_t v) { _5 = (_5 & ~0xF0) | ((v & 0xF) << 4); }
        bool hasLevelCrossing() const { return _6 & 0x10; }
        void setHasLevelCrossing(bool v) { _6 = (_6 & ~0x10) | (v ? 0x10 : 0); }
        uint8_t bridge() const { return _6 >> 5; }
        void setBridge(uint8_t v) { _6 = (_6 & ~0xE0) | ((v & 0x7) << 5); }
        uint8_t owner() const { return _7 & 0xF; }
        void setOwner(uint8_t v) { _7 = (_7 & 0xF0) | (v & 0xF); }
        uint8_t mods() const { return _7 >> 4; }
        void setMods(uint8_t v) { _7 = (_7 & 0x0F) | (v << 4); }
    };
    static_assert(sizeof(TrackElement) == kTileElementSize);

    struct StationElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::station;

    private:
        uint8_t _4;
        uint8_t _5;
        uint16_t _stationId;

    public:
        uint8_t rotation() const { return _type & 0x3; }
        void setRotation(uint8_t v) { _type = (_type & ~0x3) | (v & 0x3); }
        uint8_t sequenceIndex() const { return (_type >> 6) & 0x3; }
        void setSequenceIndex(uint8_t v) { _type = (_type & ~0xC0) | ((v & 0x3) << 6); }
        uint8_t owner() const { return _4 & 0xF; }
        void setOwner(uint8_t v) { _4 = (_4 & ~0xF) | (v & 0xF); }
        uint8_t unk4SLR4() const { return (_4 >> 4) & 0xF; }
        void setUnk4SLR4(uint8_t v) { _4 = (_4 & ~0xF0) | ((v & 0xF) << 4); }
        uint8_t objectId() const { return _5 & 0x1F; }
        void setObjectId(uint8_t v) { _5 = (_5 & ~0x1F) | (v & 0x1F); }
        uint8_t stationType() const { return _5 >> 5; }
        void setStationType(uint8_t v) { _5 = (_5 & ~0xE0) | ((v & 0x7) << 5); }
        uint16_t stationId() const { return _stationId & 0x3FF; }
        void setStationId(uint16_t v) { _stationId = (_stationId & ~0x3FF) | (v & 0x3FF); }
        uint8_t buildingType() const { return (_stationId & 0xFC00) >> 10; }
        void setBuildingType(uint8_t v) { _stationId = (_stationId & ~0xFC00) | ((v & 0x3F) << 10); }
    };
    static_assert(sizeof(StationElement) == kTileElementSize);

    struct SignalElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::signal;

        struct Side
        {
            uint8_t _4;
            uint8_t _5;
            uint8_t signalObjectId() const { return _4 & 0xF; }
            void setSignalObjectId(uint8_t v) { _4 = (_4 & ~0xF) | (v & 0xF); }
            uint8_t unk4() const { return (_4 >> 4) & 0x3; }
            void setUnk4(uint8_t v) { _4 = (_4 & ~0x30) | ((v & 0x3) << 4); }
            bool isOccupied() const { return _4 & 0x40; }
            void setIsOccupied(bool v) { _4 = (_4 & ~0x40) | (v ? 0x40 : 0); }
            bool hasSignal() const { return _4 & 0x80; }
            void setHasSignal(bool v) { _4 = (_4 & ~0x80) | (v ? 0x80 : 0); }
            uint8_t frame() const { return _5 & 0xF; }
            void setFrame(uint8_t v) { _5 = (_5 & ~0xF) | (v & 0xF); }
            uint8_t allLights() const { return _5 >> 4; }
            void setAllLights(uint8_t v) { _5 = (_5 & ~0xF0) | (v << 4); }
        };

    private:
        Side _sides[2];

    public:
        uint8_t rotation() const { return _type & 0x3; }
        void setRotation(uint8_t v) { _type = (_type & ~0x3) | (v & 0x3); }
        bool isRightGhost() const { return _type & 0x40; }
        void setRightGhost(bool v) { _type = (_type & ~0x40) | (v ? 0x40 : 0); }
        bool isLeftGhost() const { return _type & 0x80; }
        void setLeftGhost(bool v) { _type = (_type & ~0x80) | (v ? 0x80 : 0); }
        Side& left() { return _sides[0]; }
        const Side& left() const { return _sides[0]; }
        Side& right() { return _sides[1]; }
        const Side& right() const { return _sides[1]; }
    };
    static_assert(sizeof(SignalElement) == kTileElementSize);

    struct BuildingElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::building;

    private:
        uint8_t _4;
        uint8_t _5;
        uint16_t _6;

    public:
        uint8_t rotation() const { return _type & 0x3; }
        void setRotation(uint8_t v) { _type = (_type & ~0x3) | (v & 0x3); }
        bool isMiscBuilding() const { return _type & 0x40; }
        void setIsMiscBuilding(bool v) { _type = (_type & ~0x40) | (v ? 0x40 : 0); }
        bool isConstructed() const { return _type & 0x80; }
        void setConstructed(bool v) { _type = (_type & ~0x80) | (v ? 0x80 : 0); }
        uint8_t objectId() const { return _4; }
        void setObjectId(uint8_t v) { _4 = v; }
        uint8_t sequenceIndex() const { return _5 & 0x3; }
        void setSequenceIndex(uint8_t v) { _5 = (_5 & ~0x3) | (v & 0x3); }
        uint8_t unk5u() const { return _5 >> 5; }
        void setUnk5u(uint8_t v) { _5 = (_5 & ~0xE0) | (v << 5); }
        uint8_t age() const { return _6 & 0x3F; }
        void setAge(uint8_t v) { _6 = (_6 & ~0x3F) | (v & 0x3F); }
        uint8_t variation() const { return (_6 >> 6) & 0x1F; }
        void setVariation(uint8_t v) { _6 = (_6 & ~0x07C0) | ((v & 0x1F) << 6); }
        uint8_t colour() const { return _6 >> 11; }
        void setColour(uint8_t v) { _6 = (_6 & 0x7FF) | (v << 11); }
    };
    static_assert(sizeof(BuildingElement) == kTileElementSize);

    struct TreeElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::tree;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t rotation() const { return _type & 0x3; }
        void setRotation(uint8_t v) { _type = (_type & ~0x3) | (v & 0x3); }
        uint8_t quadrant() const { return (_type >> 6) & 0x3; }
        void setQuadrant(uint8_t v) { _type = (_type & ~0xC0) | ((v & 0x3) << 6); }
        uint8_t treeObjectId() const { return _4; }
        void setTreeObjectId(uint8_t v) { _4 = v; }
        uint8_t growth() const { return _5 & 0xF; }
        void setGrowth(uint8_t v) { _5 = (_5 & ~0xF) | (v & 0xF); }
        uint8_t unk5h() const { return (_5 >> 4) & 0xF; }
        void setUnk5h(uint8_t v) { _5 = (_5 & ~0xF0) | ((v & 0xF) << 4); }
        uint8_t colour() const { return _6 & 0x1F; }
        void setColour(uint8_t v) { _6 = (_6 & ~0x1F) | (v & 0x1F); }
        bool hasSnow() const { return _6 & 0x40; }
        void setSnow(bool v) { _6 = (_6 & ~0x40) | (v ? 0x40 : 0); }
        bool isDying() const { return _6 & 0x80; }
        void setIsDying(bool v) { _6 = (_6 & ~0x80) | (v ? 0x80 : 0); }
        uint8_t unk7l() const { return _7 & 0x7; }
        void setUnk7l(uint8_t v) { _7 = (_7 & ~0x7) | (v & 0x7); }
        uint8_t season() const { return (_7 >> 3) & 0x1F; }
        void setSeason(uint8_t v) { _7 = (_7 & ~0xF8) | ((v & 0x1F) << 3); }
    };
    static_assert(sizeof(TreeElement) == kTileElementSize);

    struct WallElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::wall;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t rotation() const { return _type & 0x3; }
        void setRotation(uint8_t v) { _type = (_type & ~0x3) | (v & 0x3); }
        uint8_t slopeFlags() const { return (_type & 0xC0) >> 6; }
        void setSlopeFlags(uint8_t v) { _type = (_type & ~0xC0) | ((v << 6) & 0xC0); }
        uint8_t wallObjectId() const { return _4; }
        void setWallObjectId(uint8_t v) { _4 = v; }
        uint8_t tertiaryColour() const { return _5; }
        void setTertiaryColour(uint8_t v) { _5 = v; }
        uint8_t primaryColour() const { return _6 & 0x1F; }
        void setPrimaryColour(uint8_t v) { _6 = (_6 & 0xE0) | (v & 0x1F); }
        // Secondary colour high bits are aliased into flags bits 5-6.
        uint8_t secondaryColour() const { return ((_6 >> 5) & 0x7) | ((_flags >> 2) & 0x18); }
        void setSecondaryColour(uint8_t v)
        {
            _6 = (_6 & 0x1F) | ((v & 0x7) << 5);
            _flags &= ~0x60;
            _flags |= (v & 0x18) << 2;
        }
        uint8_t var7() const { return _7; }
        void setVar7(uint8_t v) { _7 = v; }
    };
    static_assert(sizeof(WallElement) == kTileElementSize);

    struct RoadElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::road;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t rotation() const { return _type & 0x3; }
        void setRotation(uint8_t v) { _type = (_type & ~0x3) | (v & 0x3); }
        bool hasSignalElement() const { return _type & 0x40; }
        void setHasSignalElement(bool v) { _type = (_type & ~0x40) | (v ? 0x40 : 0); }
        bool hasStationElement() const { return _type & 0x80; }
        void setHasStationElement(bool v) { _type = (_type & ~0x80) | (v ? 0x80 : 0); }
        uint8_t roadId() const { return _4 & 0xF; }
        void setRoadId(uint8_t v) { _4 = (_4 & ~0xF) | (v & 0xF); }
        uint8_t laneOccupation() const { return (_4 & 0x30) >> 4; }
        void setLaneOccupation(uint8_t v) { _4 = (_4 & ~0x30) | ((v & 0x3) << 4); }
        bool hasGhostMods() const { return _4 & 0x40; }
        void setHasGhostMods(bool v) { _4 = (_4 & ~0x40) | (v ? 0x40 : 0); }
        bool hasBridge() const { return _4 & 0x80; }
        void setHasBridge(bool v) { _4 = (_4 & ~0x80) | (v ? 0x80 : 0); }
        uint8_t sequenceIndex() const { return _5 & 0x3; }
        void setSequenceIndex(uint8_t v) { _5 = (_5 & ~0x3) | (v & 0x3); }
        uint8_t levelCrossingObjectId() const { return (_5 >> 2) & 0x3; }
        void setLevelCrossingObjectId(uint8_t v) { _5 = (_5 & ~0xC) | ((v & 0x3) << 2); }
        uint8_t roadObjectId() const { return _5 >> 4; }
        void setRoadObjectId(uint8_t v) { _5 = (_5 & ~0xF0) | ((v & 0xF) << 4); }
        uint8_t unk6l() const { return _6 & 0xF; }
        void setUnk6l(uint8_t v) { _6 = (_6 & ~0xF) | (v & 0xF); }
        uint8_t bridge() const { return _6 >> 5; }
        void setBridge(uint8_t v) { _6 = (_6 & ~0xE0) | (v << 5); }
        uint8_t owner() const { return _7 & 0xF; }
        void setOwner(uint8_t v) { _7 = (_7 & ~0xF) | (v & 0xF); }
        bool unk7_10() const { return _7 & 0x10; }
        void setUnk7_10(bool v) { _7 = (_7 & ~0x10) | (v ? 0x10 : 0); }
        bool hasLevelCrossing() const { return _7 & 0x20; }
        void setHasLevelCrossing(bool v) { _7 = (_7 & ~0x20) | (v ? 0x20 : 0); }
        bool unk7_40() const { return _7 & 0x40; }
        void setUnk7_40(bool v) { _7 = (_7 & ~0x40) | (v ? 0x40 : 0); }
        bool unk7_80() const { return _7 & 0x80; }
        void setUnk7_80(bool v) { _7 = (_7 & ~0x80) | (v ? 0x80 : 0); }
    };
    static_assert(sizeof(RoadElement) == kTileElementSize);

    struct IndustryElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::industry;

    private:
        uint8_t _industryId;
        uint8_t _5;
        uint16_t _6;

    public:
        uint8_t rotation() const { return _type & 0x3; }
        void setRotation(uint8_t v) { _type = (_type & ~0x3) | (v & 0x3); }
        bool isConstructed() const { return _type & 0x80; }
        void setIsConstructed(bool v) { _type = (_type & ~0x80) | (v ? 0x80 : 0); }
        uint8_t industryId() const { return _industryId; }
        void setIndustryId(uint8_t v) { _industryId = v; }
        uint8_t sequenceIndex() const { return _5 & 0x3; }
        void setSequenceIndex(uint8_t v) { _5 = (_5 & ~0x3) | (v & 0x3); }
        uint8_t sectionProgress() const { return _5 >> 5; }
        void setSectionProgress(uint8_t v) { _5 = (_5 & ~0xE0) | (v << 5); }
        uint8_t var6_003F() const { return _6 & 0x3F; }
        void setVar6_003F(uint8_t v) { _6 = (_6 & ~0x3F) | (v & 0x3F); }
        uint8_t buildingType() const { return (_6 & 0x7C0) >> 6; }
        void setBuildingType(uint8_t v) { _6 = (_6 & ~0x7C0) | ((v & 0x1F) << 6); }
        uint8_t colour() const { return _6 >> 11; }
        void setColour(uint8_t v) { _6 = (_6 & ~0xF800) | (v << 11); }
    };
    static_assert(sizeof(IndustryElement) == kTileElementSize);
#pragma pack(pop)

    TileElement toSaveElement(const OpenLoco::GameState& gs, const World::TileElementEntry& entry);
}
