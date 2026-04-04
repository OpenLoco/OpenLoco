#pragma once

#include <sfl/static_flat_map.hpp>

namespace OpenLoco::Utility
{

    template<typename K, typename V, std::size_t N>
    constexpr auto buildLookupTable(std::pair<K, V> const (&arr)[N])
    {
        return sfl::static_flat_map<K, V, N>{ std::begin(arr), std::end(arr) };
    }

}
