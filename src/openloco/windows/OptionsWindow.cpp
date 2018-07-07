#include "../interop/interop.hpp"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::windows::OptionsWindow
{
    void open()
    {
        call(0x004BF7B9);
    }
}
