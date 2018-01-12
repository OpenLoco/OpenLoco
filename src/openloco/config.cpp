#include "interop/interop.hpp"
#include "config.h"

namespace openloco::config
{
    static loco_global<config_t, 0x0050AEB4> _config;
    static new_config _new_config;

    config_t& get()
    {
        return _config;
    }

    new_config& get_new()
    {
        return _new_config;
    }

    // 0x00441A6C
    void read()
    {
        LOCO_CALLPROC_X(0x00441A6C);
    }

    // 0x00441BB8
    void write()
    {
        LOCO_CALLPROC_X(0x00441BB8);
    }
}
