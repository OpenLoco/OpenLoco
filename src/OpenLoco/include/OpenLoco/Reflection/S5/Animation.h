#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/S5/S5Animation.h>

namespace OpenLoco
{
    template<>
    struct Reflection<S5::Animation>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::Animation, baseZ),
            REFL_FIELD(S5::Animation, type),
            REFL_FIELD(S5::Animation, pos)>;
    };
}
