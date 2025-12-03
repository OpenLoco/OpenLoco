#include "Scenario/ScenarioOptions.h"

namespace OpenLoco::Scenario
{
    static Options _activeOptions; // 0x009C8714

    Options& getOptions()
    {
        return _activeOptions;
    }
}
