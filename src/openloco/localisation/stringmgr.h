#pragma once

#include <cstdint>

namespace openloco
{
    using string_id = uint32_t;
}

namespace openloco::stringmgr
{
    const char * get(string_id id);
}
