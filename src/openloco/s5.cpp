#include "s5.h"

namespace openloco::s5
{
    static loco_global<ScenarioConfiguration, 0x009C8714> _config;
    static loco_global<ScenarioHeader, 0x009CCA34> _header;
    static loco_global<ScenarioConfiguration, 0x009CCA54> _config;
}
