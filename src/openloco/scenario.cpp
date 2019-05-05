#include "scenario.h"
#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::scenario
{
    void generateLandscape()
    {
        call(0x0043C90C);
    }
}
