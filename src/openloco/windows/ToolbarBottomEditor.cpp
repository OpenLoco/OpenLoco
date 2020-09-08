#include "../Interop/interop.hpp"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::toolbar_bottom::editor
{
    // 0x0043CCCD
    void open()
    {
        call(0x0043CCCD);
        // Note call to window_resize_gui_scenario_editor not part of this function
        // move when implemented.
    }
}
