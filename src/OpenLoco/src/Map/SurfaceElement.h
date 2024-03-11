#pragma once

#include "TileElementBase.h"

namespace OpenLoco::World
{
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
        constexpr uint8_t requiresHeightAdjustment = (1 << 5);

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

#pragma pack(push, 1)
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
        SurfaceElement(World::SmallZ baseZ, World::SmallZ clearZ, uint8_t quarterTile, bool highTypeFlag)
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
        void setSlope(uint8_t slope)
        {
            uint8_t var = snowCoverage();
            _slope = var | (slope & 0x1F);
        }
        uint8_t snowCoverage() const { return _slope & 0xE0; }
        void setSnowCoverage(uint8_t var4)
        {
            _slope &= 0x1F;
            _slope |= var4 << 5;
        }
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
        void removeIndustry(const World::Pos2& pos);

        void setVar7(uint8_t value) { _7 = value; }
    };
#pragma pack(pop)
    static_assert(sizeof(SurfaceElement) == kTileElementSize);

}
