#pragma once

#include "Map.hpp"
#include "Types.hpp"
#include <array>
#include <cassert>

namespace OpenLoco::Map
{
    struct TileElement;

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

    static constexpr size_t kTileElementSize = 8;

    namespace ElementFlags
    {
        constexpr uint8_t ghost = 1 << 4;
        constexpr uint8_t flag_5 = 1 << 5;
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
            return reinterpret_cast<const TileElement*>(reinterpret_cast<const uint8_t*>(this) - kTileElementSize);
        }

        TileElement* prev()
        {
            return reinterpret_cast<TileElement*>(reinterpret_cast<uint8_t*>(this) - kTileElementSize);
        }

        const TileElement* next() const
        {
            return reinterpret_cast<const TileElement*>(reinterpret_cast<const uint8_t*>(this) + kTileElementSize);
        }

        TileElement* next()
        {
            return reinterpret_cast<TileElement*>(reinterpret_cast<uint8_t*>(this) + kTileElementSize);
        }
    };
#pragma pack(pop)
}
