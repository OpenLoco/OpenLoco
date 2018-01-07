#include "../interop/interop.hpp"
#include "audio.h"

namespace openloco::audio
{
    // 0x004899E4
    void initialise()
    {
        LOCO_CALLPROC_X(0x004899E4);
    }
}
