#pragma once

#include "../localisation/stringmgr.h"

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
