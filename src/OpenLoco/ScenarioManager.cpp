#include "ScenarioManager.h"
#include "Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::ScenarioManager
{
    loco_global<ScenarioIndexEntry*, 0x0050AE8C> scenarioList;
    loco_global<int32_t, 0x0050AEA0> numScenarios;

    // 0x00443EF6, kind of
    uint16_t getNumScenariosByCategory(uint8_t category)
    {
        auto numScenariosInCategory = 0;
        for (auto i = 0; i < numScenarios; i++)
        {
            ScenarioIndexEntry& entry = scenarioList[i];
            if (entry.category != category)
                continue;

            // test    dword ptr [ebx+264h], 1
            // jz      short loc_443F28

            numScenariosInCategory++;
        }

        return numScenariosInCategory;
    }

    // 0x0044452F
    void loadIndex(uint8_t al)
    {
        registers regs;
        regs.al = al;
        call(0x0044452F, regs);
    }
}
