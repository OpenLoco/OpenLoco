#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/S5/S5Wave.h>

namespace OpenLoco
{
    template<>
    struct Reflection<S5::Wave>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::Wave, loc),
            REFL_FIELD(S5::Wave, frame)>;
    };
}
