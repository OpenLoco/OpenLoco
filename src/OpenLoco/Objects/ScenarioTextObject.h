#pragma once

#include "../Localisation/StringManager.h"

namespace openloco
{
#pragma pack(push, 1)
    struct scenario_text_object
    {
        string_id name;
        string_id details;
    };
#pragma pack(pop)
}
