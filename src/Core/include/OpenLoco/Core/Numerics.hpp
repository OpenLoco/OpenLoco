#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace OpenLoco::Numerics
{
    int32_t bitScanForward(uint32_t source);

    int32_t bitScanReverse(uint32_t source);

    template<typename T>
    constexpr T setMask(T x, T mask, bool value)
    {
        return (value ? (x | mask) : (x & ~mask));
    }

    constexpr uint8_t rotl4bit(uint8_t val, uint8_t rotation)
    {
        return ((val << rotation) | (val >> (4 - rotation))) & 0xF;
    }

    constexpr uint8_t rotr4bit(uint8_t val, uint8_t rotation)
    {
        return ((val >> rotation) | (val << (4 - rotation))) & 0xF;
    }

    // Alignment requires to be power of 2.
    template<typename T>
    constexpr T floor2(T value, size_t alignment)
    {
        // Ensure alignment is power of two or 0.
        assert(alignment > 0 && std::has_single_bit(alignment));

        return value & ~(alignment - 1);
    }

    // Alignment requires to be power of 2.
    template<typename T>
    constexpr T ceil2(T value, size_t alignment)
    {
        return floor2(static_cast<T>(value + alignment - 1), alignment);
    }
}
