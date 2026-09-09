#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/S5/S5Message.h>

namespace OpenLoco
{
    template<>
    struct Reflection<S5::Message>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::Message, type),
            REFL_FIELD(S5::Message, messageString),
            REFL_FIELD(S5::Message, companyId),
            REFL_FIELD(S5::Message, timeActive),
            REFL_FIELD(S5::Message, itemSubjects),
            REFL_FIELD(S5::Message, date)>;
    };
}
