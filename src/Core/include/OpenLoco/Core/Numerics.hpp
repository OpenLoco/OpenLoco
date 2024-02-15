#pragma once

#include "Traits.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

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
}
