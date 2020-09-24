#include "../Interop/Interop.hpp"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ToolbarBottom::Editor
{
    // 0x0043CCCD
    void open()
    {
        call(0x0043CCCD);
        // Note call to window_resize_gui_scenario_editor not part of this function
        // move when implemented.
    }
}
