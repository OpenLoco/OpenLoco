#include "../interop/interop.hpp"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    // 0x0046B490
    void map_open()
    {
        call(0x0046B490);
    }

    // 0x0046B5C0
    void map_center_on_view_point()
    {
        call(0x0046B5C0);
    }
}
