#include "LandscapeOptions.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    static Interop::loco_global<Options, 0x009C8714> _activeOptions;

    Options& getOptions()
    {
        return _activeOptions;
    }
}
