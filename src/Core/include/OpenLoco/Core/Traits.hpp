#pragma once

#include <type_traits>

namespace OpenLoco::Traits
{
    // C++20 deprecated std::is_pod, we consider a structure POD if we can use std::memcpy.
    template<typename T>
    struct IsPOD
    {
        static constexpr auto value = std::is_trivially_copyable_v<T>;
    };
}
