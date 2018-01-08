#include "../interop/interop.hpp"
#include "objectmgr.h"

namespace openloco::objectmgr
{
    // 0x00470F3C
    void load_index()
    {
        LOCO_CALLPROC_X(0x00470F3C);
    }
}
