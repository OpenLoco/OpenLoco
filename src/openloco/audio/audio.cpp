#include "../interop/interop.hpp"
#include "audio.h"

namespace openloco::audio
{
    // 0x00404E53
    void initialise_dsound()
    {
        LOCO_CALLPROC_X(0x00404E53);
    }

    // 0x00404E58
    void dispose_dsound()
    {
        LOCO_CALLPROC_X(0x00404E58);
    }

    // 0x004899E4
    void initialise()
    {
        LOCO_CALLPROC_X(0x004899E4);
    }
}
