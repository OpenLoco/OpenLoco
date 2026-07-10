#pragma once

#include "Types.hpp"
#include <OpenLoco/Engine/World.hpp>
#include <cassert>
#include <span>

namespace OpenLoco::World
{
    enum class ElementType : uint8_t
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

    static constexpr size_t kTileElementSize = 8;

    namespace ElementFlags
    {
        constexpr uint8_t ghost = 1 << 4;
        constexpr uint8_t aiAllocated = 1 << 5;
        constexpr uint8_t flag_6 = 1 << 6;
        constexpr uint8_t last = 1 << 7;
    }

#pragma pack(push, 1)
    struct TileElement
    {
    protected:
        uint8_t _0;
        uint8_t _flags;
        uint8_t _baseZ;
        uint8_t _clearZ;

    public:
        const uint8_t* data() const { return reinterpret_cast<const uint8_t*>(this); }

        uint8_t flags() const { return _flags; }
        SmallZ baseZ() const { return _baseZ; }
        int16_t baseHeight() const { return _baseZ * kSmallZStep; }
        SmallZ clearZ() const { return _clearZ; }
        int16_t clearHeight() const { return _clearZ * kSmallZStep; }
        void setBaseZ(SmallZ baseZ) { _baseZ = baseZ; }
        void setClearZ(SmallZ value) { _clearZ = value; }

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

        std::span<uint8_t> rawData()
        {
            return std::span{ reinterpret_cast<uint8_t*>(this), kTileElementSize };
        }
        std::span<const uint8_t> rawData() const
        {
            return std::span{ reinterpret_cast<const uint8_t*>(this), kTileElementSize };
        }
    };
#pragma pack(pop)
    static_assert(sizeof(TileElement) == 4); // base; typed elements are kTileElementSize
}
