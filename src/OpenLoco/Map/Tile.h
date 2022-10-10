#pragma once

#include "../Types.hpp"
#include "../Viewport.hpp"
#include "Map.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace OpenLoco
{
    struct BuildingObject;
    struct Industry;
    enum class StationType : uint8_t;
}

namespace OpenLoco::Map
{
    struct TileHeight
    {
        coord_t landHeight;
        coord_t waterHeight;

        explicit operator coord_t() const
        {
            return waterHeight == 0 ? landHeight : waterHeight;
        }
    };

    // 0x004F9296, 0x4F9298
    constexpr Pos2 offsets[4] = { { 0, 0 }, { 0, 32 }, { 32, 32 }, { 32, 0 } };

    Ui::viewport_pos gameToScreen(const Pos3& loc, int rotation);

    enum class ElementType
    {
        surface,  // 0x00
        track,    // 0x04
        station,  // 0x08
        signal,   // 0x0C
        building, // 0x10
        tree,     // 0x14
        wall,     // 0x18
        road,     // 0x1C
        industry, // 0x20
    };

    namespace ElementFlags
    {
        constexpr uint8_t ghost = 1 << 4;
        constexpr uint8_t flag_5 = 1 << 5;
        constexpr uint8_t flag_6 = 1 << 6;
        constexpr uint8_t last = 1 << 7;
    }

#pragma pack(push, 1)
    struct TileElement;
    static constexpr size_t TileElementSize = 8;

    struct TileElementBase
    {
    protected:
        uint8_t _type;
        uint8_t _flags;
        uint8_t _baseZ;
        uint8_t _clearZ;

    public:
        // Temporary, use this to get fields easily before they are defined
        const uint8_t* data() const;
        ElementType type() const;
        void setType(ElementType t)
        {
            // Purposely clobers any other data in _type
            _type = enumValue(t) << 2;
        }
        uint8_t flags() const { return _flags; }
        SmallZ baseZ() const { return _baseZ; }
        int16_t baseHeight() const { return _baseZ * kSmallZStep; }
        SmallZ clearZ() const { return _clearZ; }
        int16_t clearHeight() const { return _clearZ * kSmallZStep; }

        bool isGhost() const { return _flags & ElementFlags::ghost; }
        void setGhost(bool state)
        {
            _flags &= ~ElementFlags::ghost;
            _flags |= state == true ? ElementFlags::ghost : 0;
        }
        bool isFlag5() const { return _flags & ElementFlags::flag_5; }
        bool isFlag6() const { return _flags & ElementFlags::flag_6; } // in tracks/roads indicates is last tile of multi tile
        void setFlag6(bool state)
        {
            _flags &= ~ElementFlags::flag_6;
            _flags |= state == true ? ElementFlags::flag_6 : 0;
        }
        void setBaseZ(uint8_t baseZ) { _baseZ = baseZ; }
        void setClearZ(uint8_t value) { _clearZ = value; }
        bool isLast() const;
        void setLastFlag(bool state)
        {
            _flags &= ~ElementFlags::last;
            _flags |= state == true ? ElementFlags::last : 0;
        }

        std::array<uint8_t, 8>& rawData()
        {
            auto array = reinterpret_cast<std::array<uint8_t, 8>*>(this);
            return *array;
        }

        template<typename TType>
        const TType* as() const
        {
            return type() == TType::kElementType ? reinterpret_cast<const TType*>(this) : nullptr;
        }

        template<typename TType>
        TType* as()
        {
            return type() == TType::kElementType ? reinterpret_cast<TType*>(this) : nullptr;
        }

        template<typename TType>
        const TType& get() const
        {
            assert(type() == TType::kElementType);
            return *reinterpret_cast<const TType*>(this);
        }

        template<typename TType>
        TType& get()
        {
            assert(type() == TType::kElementType);
            return *reinterpret_cast<TType*>(this);
        }

        const TileElement* prev() const
        {
            return reinterpret_cast<const TileElement*>(reinterpret_cast<const uint8_t*>(this) - TileElementSize);
        }

        TileElement* prev()
        {
            return reinterpret_cast<TileElement*>(reinterpret_cast<uint8_t*>(this) - TileElementSize);
        }

        const TileElement* next() const
        {
            return reinterpret_cast<const TileElement*>(reinterpret_cast<const uint8_t*>(this) + TileElementSize);
        }

        TileElement* next()
        {
            return reinterpret_cast<TileElement*>(reinterpret_cast<uint8_t*>(this) + TileElementSize);
        }
    };

    struct TileElement : public TileElementBase
    {
    private:
        uint8_t pad[4];
    };
    static_assert(sizeof(TileElement) == TileElementSize);

    namespace SurfaceSlope
    {
        constexpr uint8_t flat = 0x00;

        namespace CornerUp
        {
            constexpr uint8_t all = 0x0F;
            constexpr uint8_t north = (1 << 0);
            constexpr uint8_t east = (1 << 1);
            constexpr uint8_t south = (1 << 2);
            constexpr uint8_t west = (1 << 3);
        }

        constexpr uint8_t doubleHeight = (1 << 4);

        namespace CornerDown
        {

            constexpr uint8_t west = CornerUp::all & ~CornerUp::west;
            constexpr uint8_t south = CornerUp::all & ~CornerUp::south;
            constexpr uint8_t east = CornerUp::all & ~CornerUp::east;
            constexpr uint8_t north = CornerUp::all & ~CornerUp::north;
        }

        namespace SideUp
        {

            constexpr uint8_t northeast = CornerUp::north | CornerUp::east;
            constexpr uint8_t southeast = CornerUp::south | CornerUp::east;
            constexpr uint8_t northwest = CornerUp::north | CornerUp::west;
            constexpr uint8_t southwest = CornerUp::south | CornerUp::west;
        }

        namespace Valley
        {

            constexpr uint8_t westeast = CornerUp::east | CornerUp::west;
            constexpr uint8_t northsouth = CornerUp::north | CornerUp::south;
        }
    }

    struct SurfaceElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::surface;

    private:
        uint8_t _slope;   // 0x4
        uint8_t _water;   // 0x5
        uint8_t _terrain; // 0x6
        uint8_t _7;       // 0x7 either variation or industry depending on high type flag

    public:
        SurfaceElement() = default;
        SurfaceElement(Map::SmallZ baseZ, Map::SmallZ clearZ, uint8_t quarterTile, bool highTypeFlag)
        {
            setType(kElementType);
            setBaseZ(baseZ);
            setClearZ(clearZ);
            _flags = quarterTile;
            setIsIndustrialFlag(highTypeFlag);
        }

        bool isSlopeDoubleHeight() const { return _slope & SurfaceSlope::doubleHeight; }
        uint8_t slopeCorners() const { return _slope & 0x0F; }
        uint8_t slope() const { return _slope & 0x1F; }
        uint8_t var_4_E0() const { return _slope & 0xE0; }
        MicroZ water() const { return _water & 0x1F; }
        int16_t waterHeight() const { return (_water & 0x1F) * kMicroZStep; }
        void setWater(MicroZ level) { _water = (_water & 0xE0) | (level & 0x1F); };
        void setVar5SLR5(uint8_t var5) { _water = (_water & 0x1F) | ((var5 << 5) & 0xE0); }
        uint8_t terrain() const { return _terrain & 0x1F; }
        void setTerrain(uint8_t terrain)
        {
            _terrain &= ~0x1F;
            _terrain |= terrain & 0x1F;
        }
        uint8_t var_6_SLR5() const { return _terrain >> 5; }
        void setVar6SLR5(uint8_t var6)
        {
            _terrain &= 0x1F;
            _terrain |= var6 << 5;
        }
        IndustryId industryId() const { return IndustryId(_7); }
        uint8_t variation() const { return _7; }
        void setIndustry(const IndustryId industry) { _7 = enumValue(industry); }
        void setVariation(const uint8_t variation) { _7 = variation; }
        void setType6Flag(bool state)
        {
            _type &= ~0x40;
            _type |= state ? 0x40 : 0;
        }
        bool hasType6Flag() const { return _type & 0x40; }

        // Note: Also used for other means for boats
        bool isIndustrial() const { return _type & 0x80; }

        // Note: Also used for other means for boats
        void setIsIndustrialFlag(bool state)
        {
            _type &= ~0x80;
            _type |= state ? 0x80 : 0;
        }
        void removeIndustry(const Map::Pos2& pos);
    };
    static_assert(sizeof(SurfaceElement) == TileElementSize);

    struct StationElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::station;

    private:
        uint8_t _4;
        uint8_t _5;
        uint16_t _stationId;

    public:
        CompanyId owner() const { return CompanyId(_4 & 0xF); } // _4l
        uint8_t objectId() const { return _5 & 0x1F; }
        StationType stationType() const;
        uint8_t rotation() const { return _type & 0x3; }
        uint8_t multiTileIndex() const { return (_type >> 6) & 3; }
        StationId stationId() const { return StationId(_stationId & 0x3FF); }
    };
    static_assert(sizeof(StationElement) == TileElementSize);

    struct BuildingElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::building;

    private:
        uint8_t _4;
        uint8_t _5;
        uint16_t _6;

    public:
        uint8_t rotation() const { return _type & 0x03; }
        bool has_40() const { return (_type & 0x40) != 0; }
        bool isConstructed() const { return (_type & 0x80) != 0; }
        void setConstructed(bool state)
        {
            _type &= ~0x80;
            _type |= state ? 0x80 : 0;
        }
        Colour colour() const { return static_cast<Colour>(_6 >> 11); }
        void setColour(Colour colour) { _6 = (_6 & 0x7FF) | (enumValue(colour) << 11); }
        uint8_t objectId() const { return _4; }
        const BuildingObject* getObject() const;
        uint8_t multiTileIndex() const { return _5 & 3; }
        uint8_t unk5u() const { return _5 >> 5; } // likely age related as well (higher precision)
        void setUnk5u(uint8_t value)
        {
            _5 &= ~0xE0;
            _5 |= value << 5;
        }
        uint8_t variation() const { return (_6 >> 6) & 0x1F; }
        void setVariation(uint8_t variation)
        {
            _6 &= ~0x07C0;
            _6 |= (variation & 0x1F) << 6;
        }
        uint8_t age() const { return _6 & 0x3F; } // 6l
        void setAge(uint8_t value)                // 6l
        {
            _6 &= ~0x3F;
            _6 |= value & 0x3F;
        }
        bool update(const Map::Pos2& loc);
    };
    static_assert(sizeof(BuildingElement) == TileElementSize);

    struct TreeElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::tree;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t treeObjectId() const { return _4; } // _4
        void setTreeObjectId(uint8_t type)
        {
            _4 = type;
        }
        uint8_t rotation() const { return _type & 0x03; }
        void setRotation(uint8_t rotation)
        {
            _type &= ~0x3;
            _type |= rotation & 0x3;
        }
        uint8_t quadrant() const { return (_type >> 6) & 0x03; } // _0_C0
        void setQuadrant(uint8_t quad)
        {
            _type &= ~0xC0;
            _type |= (quad & 0x3) << 6;
        }
        uint8_t unk5l() const { return _5 & 0xF; }
        void setUnk5l(uint8_t unk)
        {
            _5 &= ~0xF;
            _5 |= unk & 0xF;
        }
        void setUnk5h(uint8_t unk)
        {
            _5 &= ~0xF0;
            _5 |= (unk & 0xF) << 4;
        }
        Colour colour() const { return static_cast<Colour>(_6 & 0x1F); } //_6l
        void setColour(Colour colour)
        {
            _6 &= ~0x1F;
            _6 |= enumValue(colour) & 0x1F;
        }
        bool hasSnow() const { return _6 & 0x40; } //_6_40
        void setSnow(bool hasSnow)
        {
            _6 &= ~0x40;
            _6 |= hasSnow ? 0x40 : 0;
        }
        void setUnk6_80(bool unk)
        {
            _6 &= ~0x80;
            _6 |= unk ? 0x80 : 0;
        }
        uint8_t unk7l() const { return _7 & 0x7; }
        void setUnk7l(uint8_t unk)
        {
            _7 &= ~0x7;
            _7 |= unk & 0x7;
        }
        uint8_t season() const { return (_7 >> 3) & 0x7; } // unsure of &0x7
        void setSeason(uint8_t season)
        {
            _7 &= ~0xF8;
            _7 |= (season & 0x1F) << 3; // unsure of & 0x1F
        }
    };
    static_assert(sizeof(TreeElement) == TileElementSize);

    struct WallElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::wall;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t wallObjectId() const { return _4; }      // _4
        uint8_t rotation() const { return _type & 0x3; } // _0
    };
    static_assert(sizeof(WallElement) == TileElementSize);

    struct TrackElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::track;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        TrackElement() = default;
        TrackElement(Map::SmallZ baseZ, Map::SmallZ clearZ, uint8_t direction, uint8_t quarterTile, uint8_t sequenceIndex, uint8_t trackObjId, uint8_t trackId, std::optional<uint8_t> bridge, CompanyId owner, uint8_t mods);

        bool hasStationElement() const { return (_type & 0x80) != 0; }
        bool hasSignal() const { return (_type & 0x40) != 0; }
        uint8_t unkDirection() const { return _type & 0x03; }
        uint8_t trackId() const { return _4 & 0x3F; } // _4
        bool hasBridge() const { return (_4 & 0x80) != 0; }
        bool hasGhostMods() const { return (_4 & 0x40) != 0; }
        uint8_t trackObjectId() const { return _5 >> 4; }  // _5u
        uint8_t sequenceIndex() const { return _5 & 0xF; } // _5l
        bool has_6_10() const { return (_6 & 0x10) != 0; }
        uint8_t bridge() const { return _6 >> 5; }              // _6u
        CompanyId owner() const { return CompanyId(_7 & 0xF); } // _7l
        void setOwner(CompanyId newOwner) { _7 = (_7 & 0xF0) | (enumValue(newOwner) & 0xF); }
        bool hasMod(uint8_t mod) const { return _7 & (1 << (4 + mod)); } // _7u
        uint8_t mods() const { return _7 >> 4; }                         // _7u
    };
    static_assert(sizeof(TrackElement) == TileElementSize);

    struct SignalElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::signal;

        struct Side
        {
        private:
            uint8_t _4;
            uint8_t _5;

        public:
            bool hasSignal() const { return _4 & 0x80; }
            bool hasUnk4_40() const { return _4 & (1 << 6); }
            void setUnk4_40(bool newState)
            {
                _4 &= ~(1 << 6);
                _4 |= newState ? (1 << 6) : 0;
            }
            void setUnk4(uint8_t newUnk4)
            {
                _4 &= ~0x30;
                _4 |= (newUnk4 & 0x3) << 4;
            }
            uint8_t signalObjectId() const { return _4 & 0xF; } // _4l
            uint8_t frame() const { return _5 & 0xF; }          // _5l
            uint8_t allLights() const { return _5 >> 4; }       // _5u
            void setAllLights(uint8_t newLights)
            {
                _5 &= ~(0xF0);
                _5 |= newLights << 4;
            }
            bool hasRedLight() const { return _5 & 0x40; }    // TBC colours
            bool hasRedLight2() const { return _5 & 0x10; }   // TBC colours
            bool hasGreenLight() const { return _5 & 0x80; }  // TBC colours
            bool hasGreenLight2() const { return _5 & 0x20; } // TBC colours
        };

    private:
        Side sides[2];

    public:
        uint8_t rotation() const { return _type & 0x3; }
        bool isLeftGhost() const { return _type & 0x80; }
        bool isRightGhost() const { return _type & 0x40; }
        Side& getLeft() { return sides[0]; }
        Side& getRight() { return sides[1]; }
        const Side& getLeft() const { return sides[0]; }
        const Side& getRight() const { return sides[1]; }
    };
    static_assert(sizeof(SignalElement) == TileElementSize);

    struct RoadElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::road;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t unkDirection() const { return _type & 0x03; }
        uint8_t roadId() const { return _4 & 0xF; } // _4l
        bool hasBridge() const { return (_4 & 0x80) != 0; }
        uint8_t unk4u() const { return (_4 & 0x30) >> 4; }
        void setUnk4u(uint8_t newUnk4u)
        {
            _4 &= ~(0x30);
            _4 |= (newUnk4u & 0x3) << 4;
        }
        uint8_t roadObjectId() const { return _5 >> 4; }   // _5u
        uint8_t sequenceIndex() const { return _5 & 0x3; } // _5l
        uint8_t bridge() const { return _6 >> 5; }         // _6u
        bool hasStationElement() const { return (_type & 0x80) != 0; }
        bool hasUnk7_10() const { return _7 & (1 << 4); } // _7u (bit 4) level crossing related
        void setUnk7_10(bool newState)                    // _7u (bit 4) level crossing related
        {
            _7 &= ~(1 << 4);
            _7 |= newState ? (1 << 4) : 0;
        }
        bool hasLevelCrossing() const { return _7 & (1 << 5); }
        void setUnk7_40(bool newState) // NOTE: This is the same field as mods!
        {
            _7 &= ~(1 << 6);
            _7 |= newState ? (1 << 6) : 0;
        }
        bool hasMod(uint8_t mod) const { return _7 & (1 << (mod + 6)); } // _7u (bits 6 and 7)
        uint8_t mods() const { return _7 >> 6; }                         // _7u
        CompanyId owner() const { return CompanyId(_7 & 0xF); }          // _7l
        void setOwner(CompanyId newOwner) { _7 = (_7 & 0xF0) | (enumValue(newOwner) & 0xF); }
        bool update(const Map::Pos2& loc);
    };
    static_assert(sizeof(RoadElement) == TileElementSize);

    struct IndustryElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::industry;

    private:
        IndustryId _industryId;
        uint8_t _5;
        uint16_t _6;

    public:
        OpenLoco::IndustryId industryId() const { return _industryId; }
        OpenLoco::Industry* industry() const;
        uint8_t buildingType() const;
        uint8_t rotation() const { return _type & 0x3; }
        bool hasHighTypeFlag() const { return _type & 0x80; } // isConstructed?
    };
    static_assert(sizeof(IndustryElement) == TileElementSize);
#pragma pack(pop)

    struct Tile
    {
    private:
        TileElement* const _data;

    public:
        static constexpr size_t npos = std::numeric_limits<size_t>().max();

        const TilePos2 pos;

        Tile(const TilePos2& tPos, TileElement* data);
        bool isNull() const;
        TileElement* begin();
        TileElement* begin() const;
        TileElement* end();
        TileElement* end() const;
        size_t size();
        TileElement* operator[](size_t i);

        size_t indexOf(const TileElementBase* element) const;
        SurfaceElement* surface() const;
        StationElement* trackStation(uint8_t trackId, uint8_t direction, uint8_t baseZ) const;
        StationElement* roadStation(uint8_t roadId, uint8_t direction, uint8_t baseZ) const;
    };
}
