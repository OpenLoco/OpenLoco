#pragma once

#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct TownNamesObject
    {
        static constexpr auto kObjectType = ObjectType::townNames;

        string_id name;
    };
#pragma pack(pop)
}
