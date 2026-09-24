#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/Economy/Currency.h>

namespace OpenLoco
{
    template<>
    struct Reflection<currency48_t>
    {
        using Fields = FieldList<
            REFL_FIELD(currency48_t, var_00),
            REFL_FIELD(currency48_t, var_04)>;
    };
}
