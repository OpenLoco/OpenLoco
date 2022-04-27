#include "ScenarioManager.h"
#include "GameState.h"
#include "Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::ScenarioManager
{
    loco_global<ScenarioIndexEntry*, 0x0050AE8C> scenarioList;
    loco_global<int32_t, 0x0050AEA0> scenarioCount;

    bool hasScenariosForCategory(uint8_t category)
    {
        for (auto i = 0; i < scenarioCount; i++)
        {
            ScenarioIndexEntry& entry = scenarioList[i];
            if (!entry.hasFlag(ScenarioIndexFlags::flag_0))
                continue;

            if (entry.category == category)
                return true;
        }

        return false;
    }

    bool hasScenarioInCategory(uint8_t category, ScenarioIndexEntry* scenario)
    {
        for (auto i = 0; i < scenarioCount; i++)
        {
            ScenarioIndexEntry* entry = &scenarioList[i];

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
        for (auto i = 0; i < scenarioCount; i++)
        {
            ScenarioIndexEntry& entry = scenarioList[i];
            if (entry.category != category || !entry.hasFlag(ScenarioIndexFlags::flag_0))
                continue;

            scenarioCountInCategory++;
        }

        return scenarioCountInCategory;
    }

    ScenarioIndexEntry* getNthScenarioFromCategory(uint8_t category, uint8_t index)
    {
        uint8_t j = 0;
        for (auto i = 0; i < scenarioCount; i++)
        {
            ScenarioIndexEntry& entry = scenarioList[i];
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

    namespace Objective
    {
        // 0x00526230
        Scenario::ObjectiveType getObjectiveType()
        {
            return getGameState().objectiveType;
        }

        void setObjectiveType(Scenario::ObjectiveType type)
        {
            getGameState().objectiveType = type;
        }

        // 0x00526231
        uint8_t getObjectiveFlags()
        {
            return getGameState().objectiveFlags;
        }
        void setObjectiveFlags(uint8_t flags)
        {
            getGameState().objectiveFlags = flags;
        }

        // 0x00526232
        uint32_t getObjectiveCompanyValue()
        {
            return getGameState().objectiveCompanyValue;
        }
        void setObjectiveCompanyValue(uint32_t value)
        {
            getGameState().objectiveCompanyValue = value;
        }

        // 0x00526236
        uint32_t getObjectiveMonthlyVehicleProfit()
        {
            return getGameState().objectiveMonthlyVehicleProfit;
        }
        void setObjectiveMonthlyVehicleProfit(uint32_t profit)
        {
            getGameState().objectiveMonthlyVehicleProfit = profit;
        }

        // 0x0052623A
        uint8_t getObjectivePerformanceIndex()
        {
            return getGameState().objectivePerformanceIndex;
        }
        void setObjectivePerformanceIndex(uint8_t performanceIndex)
        {
            getGameState().objectivePerformanceIndex = performanceIndex;
        }

        // 0x0052623B
        uint8_t getObjectiveDeliveredCargoType()
        {
            return getGameState().objectiveDeliveredCargoType;
        }
        void setObjectiveDeliveredCargoType(uint8_t cargoType)
        {
            getGameState().objectiveDeliveredCargoType = cargoType;
        }

        // 0x0052623C
        uint32_t getObjectiveDeliveredCargoAmount()
        {
            return getGameState().objectiveDeliveredCargoAmount;
        }
        void setObjectiveDeliveredCargoAmount(uint32_t cargo)
        {
            getGameState().objectiveDeliveredCargoAmount = cargo;
        }

        // 0x00526240
        uint8_t getObjectiveTimeLimitYears()
        {
            return getGameState().objectiveTimeLimitYears;
        }
        void setObjectiveTimeLimitYears(uint8_t years)
        {
            getGameState().objectiveTimeLimitYears = years;
        }

        // 0x00526241
        uint16_t getObjectiveTimeLimitUntilYear()
        {
            return getGameState().objectiveTimeLimitUntilYear;
        }
        void setObjectiveTimeLimitUntilYear(uint16_t year)
        {
            getGameState().objectiveTimeLimitUntilYear = year;
        }

        // 0x00526243
        uint16_t getObjectiveMonthsInChallenge()
        {
            return getGameState().objectiveMonthsInChallenge;
        }
        void setObjectiveMonthsInChallenge(uint16_t months)
        {
            getGameState().objectiveMonthsInChallenge = months;
        }

        // 0x00526245
        uint16_t getObjectiveCompletedChallengeInMonths()
        {
            return getGameState().objectiveCompletedChallengeInMonths;
        }
        void setObjectiveCompletedChallengeInMonths(uint16_t months)
        {
            getGameState().objectiveCompletedChallengeInMonths = months;
        }
    }
}
