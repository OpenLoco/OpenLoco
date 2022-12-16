#include "ScenarioManager.h"
#include "CompanyManager.h"
#include "GameState.h"
#include "Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::ScenarioManager
{
    loco_global<ScenarioIndexEntry*, 0x0050AE8C> _scenarioList;
    loco_global<int32_t, 0x0050AEA0> _scenarioCount;

    bool hasScenariosForCategory(uint8_t category)
    {
        for (auto i = 0; i < _scenarioCount; i++)
        {
            ScenarioIndexEntry& entry = _scenarioList[i];
            if (!entry.hasFlag(ScenarioIndexFlags::flag_0))
                continue;

            if (entry.category == category)
                return true;
        }

        return false;
    }

    bool hasScenarioInCategory(uint8_t category, ScenarioIndexEntry* scenario)
    {
        for (auto i = 0; i < _scenarioCount; i++)
        {
            ScenarioIndexEntry* entry = &_scenarioList[i];

            if (entry->category != category || !entry->hasFlag(ScenarioIndexFlags::flag_0))
            {
                if (entry == scenario)
                    return false;
                else
                    continue;
            }

            if (entry == scenario)
                return true;
        }

        return false;
    }

    // 0x00443EF6, kind of
    uint16_t getScenarioCountByCategory(uint8_t category)
    {
        auto scenarioCountInCategory = 0;
        for (auto i = 0; i < _scenarioCount; i++)
        {
            ScenarioIndexEntry& entry = _scenarioList[i];
            if (entry.category != category || !entry.hasFlag(ScenarioIndexFlags::flag_0))
                continue;

            scenarioCountInCategory++;
        }

        return scenarioCountInCategory;
    }

    ScenarioIndexEntry* getNthScenarioFromCategory(uint8_t category, uint8_t index)
    {
        uint8_t j = 0;
        for (auto i = 0; i < _scenarioCount; i++)
        {
            ScenarioIndexEntry& entry = _scenarioList[i];
            if (entry.category != category || !entry.hasFlag(ScenarioIndexFlags::flag_0))
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

    // 0x00438959
    void saveNewScore(Scenario::ObjectiveProgress& progress, const CompanyId companyId)
    {
        auto* company = CompanyManager::get(companyId);
        registers regs;
        regs.esi = X86Pointer(company);
        call(0x00438959, regs);
    }

    // 0x00525F5E
    uint32_t getScenarioTicks()
    {
        return getGameState().scenarioTicks;
    }
    void setScenarioTicks(uint32_t ticks)
    {
        getGameState().scenarioTicks = ticks;
    }

    // 0x00525F64
    uint32_t getScenarioTicks2()
    {
        return getGameState().scenarioTicks2;
    }
    void setScenarioTicks2(uint32_t ticks)
    {
        getGameState().scenarioTicks2 = ticks;
    }
}
