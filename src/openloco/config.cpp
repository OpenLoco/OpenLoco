#include "interop/interop.hpp"
#include "config.h"

namespace openloco::config
{
    loco_global<config_t, 0x0050AEB4> _config;

    config_t& get()
    {
        return _config;
    }

    // 0x00441A6C
    void read()
    {
        LOCO_CALLPROC_X(0x00441A6C);
    }
}
