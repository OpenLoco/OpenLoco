#include "stringmgr.h"
#include "../interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::stringmgr
{
    static loco_global<char * [0xFFFF], 0x005183FC> _strings;

    const char* get_string(string_id id)
    {
        return _strings[(size_t)id];
    }
}
