#include "interop/interop.hpp"
#include "windowmgr.h"

namespace openloco::ui::windowmgr
{
    // 0x004392BD
    void resize()
    {
        LOCO_CALLPROC_X(0x004392BD);
    }
}

