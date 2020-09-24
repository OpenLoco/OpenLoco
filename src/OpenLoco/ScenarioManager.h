#pragma once

#include <cstddef>
#include <cstdint>

namespace OpenLoco::ScenarioManager
{
    enum ScenarioIndexFlags : uint8_t
    {
        flag_1 = 1 << 0,
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
        uint8_t pad_469[0x46C - 0x469]; // 0x469
        char currencyObjectId[8];       // 0x46C
        uint8_t pad_268[0x478 - 0x474]; // 0x474
        uint8_t preview[128][128];      // 0x478

        bool hasFlag(ScenarioIndexFlags flag)
        {
            return (flags & flag) != 0;
        }
    };

    static_assert(offsetof(ScenarioIndexEntry, category) == 0x100);
    static_assert(offsetof(ScenarioIndexEntry, flags) == 0x264);
    static_assert(sizeof(ScenarioIndexEntry) == 0x4478);

    uint16_t getNumScenariosByCategory(uint8_t category);
    ScenarioIndexEntry* getNthScenarioFromCategory(uint8_t category, uint8_t index);
    void loadIndex(uint8_t al);
}
