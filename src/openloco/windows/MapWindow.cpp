#include "../interop/interop.hpp"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::windows::MapWindow
{
    // 0x0046B5C0
    void centerOnViewpoint()
    {
        call(0x0046B5C0);
    }
}
