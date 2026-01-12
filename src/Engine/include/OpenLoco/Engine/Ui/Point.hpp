#pragma once

#include <OpenLoco/Math/Vector.hpp>
#include <cstdint>

namespace OpenLoco::Ui
{
    struct UISpaceTag
    {
    };
    using Point = Math::Vector::TVector2<int32_t, UISpaceTag>;
}
