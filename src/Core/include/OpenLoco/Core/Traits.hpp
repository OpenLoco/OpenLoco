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

    // TODO: Move this into Core/Traits.hpp, however Core depends on Utility so we can't do that yet.
    template<typename T, typename = void>
    struct UnderlyingType
    {
        using type = T;
    };

    template<typename T>
    struct UnderlyingType<T, std::enable_if_t<std::is_enum_v<T>>>
    {
        using type = typename std::underlying_type<T>::type;
    };
}
