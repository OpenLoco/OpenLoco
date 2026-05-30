#pragma once

#include "TileElement.h"
#include <cassert>
#include <cstdint>
#include <span>

namespace OpenLoco::World
{
    struct TileElement;

    struct TileElementEntry
    {
    public:
        static constexpr uint32_t kTypeBits = 4;
        static constexpr uint32_t kIsLastBits = 1;
        static constexpr uint32_t kIndexBits = 27;

        static constexpr uint32_t kTypeShift = 0;
        static constexpr uint32_t kIsLastShift = kTypeBits;
        static constexpr uint32_t kIndexShift = kTypeBits + kIsLastBits;

        static constexpr uint32_t kTypeMask = (1U << kTypeBits) - 1U;
        static constexpr uint32_t kIsLastMask = (1U << kIsLastBits) - 1U;
        static constexpr uint32_t kIndexMask = (1U << kIndexBits) - 1U;

        static constexpr uint32_t kInvalidIndex = kIndexMask;
        static constexpr uint32_t kEmptyRaw = 0xFFFFFFFFU;

    private:
        uint32_t _value = 0;

    public:
        constexpr TileElementEntry() = default;

        constexpr explicit TileElementEntry(uint32_t raw)
            : _value(raw)
        {
        }

        static constexpr TileElementEntry empty()
        {
            return TileElementEntry{ kEmptyRaw };
        }

        constexpr bool isEmpty() const
        {
            return _value == kEmptyRaw;
        }

        ElementType type() const
        {
            return static_cast<ElementType>((_value >> kTypeShift) & kTypeMask);
        }

        void setType(ElementType t)
        {
            _value = (_value & ~(kTypeMask << kTypeShift))
                | ((static_cast<uint32_t>(t) & kTypeMask) << kTypeShift);
        }

        bool isLast() const
        {
            return ((_value >> kIsLastShift) & kIsLastMask) != 0;
        }

        void setLastFlag(bool state)
        {
            _value = (_value & ~(kIsLastMask << kIsLastShift))
                | ((state ? 1U : 0U) << kIsLastShift);
        }

        uint32_t index() const
        {
            return (_value >> kIndexShift) & kIndexMask;
        }

        void setIndex(uint32_t i)
        {
            assert(i <= kIndexMask);
            _value = (_value & ~(kIndexMask << kIndexShift))
                | ((i & kIndexMask) << kIndexShift);
        }

        uint32_t raw() const
        {
            return _value;
        }

        void setRaw(uint32_t v)
        {
            _value = v;
        }

        uint8_t flags() const;
        SmallZ baseZ() const;
        void setBaseZ(SmallZ v);
        int16_t baseHeight() const;
        SmallZ clearZ() const;
        void setClearZ(SmallZ v);
        int16_t clearHeight() const;

        bool isGhost() const;
        void setGhost(bool state);
        bool isAiAllocated() const;
        void setAiAllocated(bool state);
        bool isFlag6() const;
        void setFlag6(bool state);
        uint8_t occupiedQuarter() const;
        void setOccupiedQuarter(uint8_t v);

        template<typename T>
        T* as() const
        {
            return type() == T::kElementType ? &this->template get<T>() : nullptr;
        }

        template<typename T>
        T& get() const;

        TileElement* operator->() const;
        TileElement& operator*() const;

        const uint8_t* data() const;
        std::span<uint8_t> rawData() const;

        TileElementEntry* prev()
        {
            return this - 1;
        }

        const TileElementEntry* prev() const
        {
            return this - 1;
        }

        TileElementEntry* next()
        {
            return this + 1;
        }

        const TileElementEntry* next() const
        {
            return this + 1;
        }

        constexpr bool operator==(const TileElementEntry& other) const = default;
    };
    static_assert(sizeof(TileElementEntry) == 4);
}
