#include "../interop/interop.hpp"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::options
{
    void open()
    {
        call(0x004BF7B9);
    }
}
