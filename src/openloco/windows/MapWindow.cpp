#include "../interop/interop.hpp"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    // 0x0046B5C0
    void map_center_on_view_point()
    {
        call(0x0046B5C0);
    }
}
