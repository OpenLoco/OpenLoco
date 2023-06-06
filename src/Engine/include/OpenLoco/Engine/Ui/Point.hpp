#pragma once

#include <OpenLoco/Math/Vector.hpp>
#include <cstdint>

namespace OpenLoco::Ui
{
    struct UISpaceTag
    {
    };
    using Point = Math::Vector::TVector2<int16_t, UISpaceTag>;
    using Point32 = Math::Vector::TVector2<int32_t, UISpaceTag>;

    // Until interop is removed this is a requirement (for global vars mainly)
    static_assert(sizeof(Point) == 4);
    static_assert(sizeof(Point32) == 8);
}
