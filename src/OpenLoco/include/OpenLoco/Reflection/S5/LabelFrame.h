#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/S5/S5LabelFrame.h>

namespace OpenLoco
{
    template<>
    struct Reflection<S5::LabelFrame>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::LabelFrame, left),
            REFL_FIELD(S5::LabelFrame, right),
            REFL_FIELD(S5::LabelFrame, top),
            REFL_FIELD(S5::LabelFrame, bottom)>;
    };
}
