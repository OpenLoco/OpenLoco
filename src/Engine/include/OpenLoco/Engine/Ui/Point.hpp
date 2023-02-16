#pragma once

#include <OpenLoco/Math/Vector.hpp>
#include <cstdint>

namespace OpenLoco::Ui
{
    using Point = Math::Vector::TVector2<int16_t, 1, false>;
    using Point32 = Math::Vector::TVector2<int32_t, 1, false>;

    // Until interop is removed this is a requirement (for global vars mainly)
    static_assert(sizeof(Point) == 4);
    static_assert(sizeof(Point32) == 8);
}
