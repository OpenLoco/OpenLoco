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
        }

        constexpr operator uint8_t() const
        {
            return level;
        }

        // Converts a value from screen space into world space.
        template<typename T>
        constexpr T applyTo(const T value) const
        {
            return static_cast<T>(value << level);
        }

        // Converts a value from world space into screen space.
        template<typename T>
        constexpr T applyInversedTo(const T value) const
        {
            return static_cast<T>(value >> level);
        }
    };
#pragma pack(pop)

    static_assert(sizeof(ZoomLevel) == sizeof(uint8_t));
}
