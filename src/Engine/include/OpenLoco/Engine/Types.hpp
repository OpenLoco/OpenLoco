#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace OpenLoco
{
    // To be replaced with std::to_underlying in c++23
    template<typename TEnum>
    constexpr auto enumValue(TEnum enumerator) noexcept
    {
        return static_cast<std::underlying_type_t<TEnum>>(enumerator);
    }

    using coord_t = int16_t;
    using tile_coord_t = int16_t;
}
