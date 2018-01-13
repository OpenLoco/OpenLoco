#include "../interop/interop.hpp"
#include "objectmgr.h"

using namespace openloco::interop;

namespace openloco::objectmgr
{
    // 0x00470F3C
    void load_index()
    {
        call(0x00470F3C);
    }
}
