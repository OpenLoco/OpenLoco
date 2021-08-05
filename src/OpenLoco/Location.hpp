#pragma once

#include <cstdint>

#include "Math/Vector.hpp"

namespace OpenLoco
{
    using xy32 = Math::Vector::TVector2<int32_t, 1, false>;

    namespace Location
    {
        constexpr int16_t null = (int16_t)0x8000u;
    }
}
