#include "../interop/interop.hpp"
#include "../windowmgr.h"

namespace openloco::ui::textinput
{
    // 0x004CE6F2
    void close()
    {
        windowmgr::close(window_type::text_input);
    }
}
