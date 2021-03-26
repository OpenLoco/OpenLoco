#pragma once

#include <cassert>
#include <cstdint>

namespace OpenLoco
{
#pragma pack(push, 1)
    class ZoomLevel
    {
        uint8_t level{};

    public:
        static constexpr uint8_t full = 0;
        static constexpr uint8_t half = 1;
        static constexpr uint8_t quarter = 2;
        static constexpr uint8_t eighth = 3;
        static constexpr uint8_t max = 4;

        constexpr ZoomLevel() = default;
        constexpr ZoomLevel(uint8_t zoom)
            : level(zoom)
        {
            assert(zoom < max);
        }

        constexpr operator uint8_t()
        {
            return level;
        }

        constexpr bool operator==(const ZoomLevel other) const
        {
            return level == other.level;
        }

        constexpr bool operator!=(const ZoomLevel other) const
        {
            return level != other.level;
        }
    };
#pragma pack(pop)

    static_assert(sizeof(ZoomLevel) == sizeof(uint8_t));
}
