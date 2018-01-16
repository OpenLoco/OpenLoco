#pragma once

#include <cstdint>

namespace openloco
{
    using string_id = uint16_t;
}

namespace openloco::stringmgr
{
    const char * get(string_id id);
}
