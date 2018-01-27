#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    // 0x0043B26C
    void open_about_window()
    {
        call(0x0043B26C);
    }
}
