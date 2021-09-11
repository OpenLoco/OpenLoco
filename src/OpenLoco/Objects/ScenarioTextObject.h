#pragma once

#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct ScenarioTextObject
    {
        static constexpr auto _objectType = ObjectType::scenarioText;

        string_id name;
        string_id details;
    };
#pragma pack(pop)
}
