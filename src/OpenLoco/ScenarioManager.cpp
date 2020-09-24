#include "ScenarioManager.h"
#include "Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::ScenarioManager
{
    loco_global<ScenarioIndexEntry*, 0x0050AE8C> scenarioList;
    loco_global<int32_t, 0x0050AEA0> numScenarios;

    bool hasScenariosForCategory(uint8_t category)
    {
        for (auto i = 0; i < numScenarios; i++)
        {
            ScenarioIndexEntry& entry = scenarioList[i];
            if (!entry.hasFlag(ScenarioIndexFlags::flag_1))
                continue;

            if (entry.category == category)
                return true;
        }

        return false;
    }

    // 0x00443EF6, kind of
    uint16_t getNumScenariosByCategory(uint8_t category)
    {
        auto numScenariosInCategory = 0;
        for (auto i = 0; i < numScenarios; i++)
        {
            ScenarioIndexEntry& entry = scenarioList[i];
            if (entry.category != category || !entry.hasFlag(ScenarioIndexFlags::flag_1))
                continue;

            numScenariosInCategory++;
        }

        return numScenariosInCategory;
    }

    ScenarioIndexEntry* getNthScenarioFromCategory(uint8_t category, uint8_t index)
    {
        uint8_t j = 0;
        for (auto i = 0; i < numScenarios; i++)
        {
            ScenarioIndexEntry& entry = scenarioList[i];
            if (entry.category != category || !entry.hasFlag(ScenarioIndexFlags::flag_1))
                continue;

            if (j == index)
                return &entry;

            j++;
        }

        return nullptr;
    }

    // 0x0044452F
    void loadIndex(uint8_t al)
    {
        registers regs;
        regs.al = al;
        call(0x0044452F, regs);
    }
}
