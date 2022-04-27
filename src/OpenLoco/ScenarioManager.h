#pragma once

#include "Objects/Object.h"
#include "Scenario.h"

#include <cstddef>
#include <cstdint>

namespace OpenLoco::ScenarioManager
{
    enum ScenarioIndexFlags : uint8_t
    {
        flag_0 = 1 << 0,
        completed = 1 << 1,
        hasPreviewImage = 1 << 2,
    };

    struct ScenarioIndexEntry
    {
        char filename[0x100];           // 0x000
        uint8_t category;               // 0x100
        uint8_t numCompetingCompanies;  // 0x101
        uint8_t competingCompanyDelay;  // 0x102
        uint8_t pad_103[0x120 - 0x103]; // 0x103
        uint16_t startYear;             // 0x120
        uint16_t completedMonths;       // 0x122
        char scenarioName[0x40];        // 0x124
        char description[0x100];        // 0x164
        uint32_t flags;                 // 0x264
        char highscoreName[0x100];      // 0x268
        char objective[0x100];          // 0x368
        ObjectHeader currency;          // 0x468
        uint8_t preview[128][128];      // 0x478

        bool hasFlag(ScenarioIndexFlags flag)
        {
            return (flags & flag) != 0;
        }
    };

    static_assert(offsetof(ScenarioIndexEntry, category) == 0x100);
    static_assert(offsetof(ScenarioIndexEntry, flags) == 0x264);
    static_assert(sizeof(ScenarioIndexEntry) == 0x4478);

    bool hasScenariosForCategory(uint8_t category);
    bool hasScenarioInCategory(uint8_t category, ScenarioIndexEntry* scenario);
    uint16_t getScenarioCountByCategory(uint8_t category);
    ScenarioIndexEntry* getNthScenarioFromCategory(uint8_t category, uint8_t index);
    void loadIndex(uint8_t al);

    // 0x00525F5E
    uint32_t getScenarioTicks();
    void setScenarioTicks(uint32_t ticks);

    // 0x00525F64
    uint32_t getScenarioTicks2();
    void setScenarioTicks2(uint32_t ticks);

    namespace Objective
    {
        // 0x00526230
        Scenario::ObjectiveType getObjectiveType();
        void setObjectiveType(Scenario::ObjectiveType type);

        // 0x00526231
        uint8_t getObjectiveFlags();
        void setObjectiveFlags(uint8_t flags);

        // 0x00526232
        uint32_t getObjectiveCompanyValue();
        void setObjectiveCompanyValue(uint32_t value);

        // 0x00526236
        uint32_t getObjectiveMonthlyVehicleProfit();
        void setObjectiveMonthlyVehicleProfit(uint32_t profit);

        // 0x0052623A
        uint8_t getObjectivePerformanceIndex();
        void setObjectivePerformanceIndex(uint8_t performanceIndex);

        // 0x0052623B
        uint8_t getObjectiveDeliveredCargoType();
        void setObjectiveDeliveredCargoType(uint8_t cargoType);

        // 0x0052623C
        uint32_t getObjectiveDeliveredCargoAmount();
        void setObjectiveDeliveredCargoAmount(uint32_t cargo);

        // 0x00526240
        uint8_t getObjectiveTimeLimitYears();
        void setObjectiveTimeLimitYears(uint8_t years);

        // 0x00526241
        uint16_t getObjectiveTimeLimitUntilYear();
        void setObjectiveTimeLimitUntilYear(uint16_t year);

        // 0x00526243
        uint16_t getObjectiveMonthsInChallenge();
        void setObjectiveMonthsInChallenge(uint16_t months);

        // 0x00526245
        uint16_t getObjectiveCompletedChallengeInMonths();
        void setObjectiveCompletedChallengeInMonths(uint16_t months);
    }
}
