#pragma once

#include <cstdint>

namespace openloco
{
    using string_id = uint16_t;

    namespace string_ids
    {
        constexpr string_id null = 0xFFFF;
    }
}

namespace openloco::stringmgr
{
    const char* get_string(string_id id);
}
