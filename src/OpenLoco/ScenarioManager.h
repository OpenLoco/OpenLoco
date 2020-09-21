#pragma once

#include <cstddef>
#include <cstdint>

namespace OpenLoco::ScenarioManager
{
    struct ScenarioIndexEntry
    {
        char filename[0x100];            // 0x000
        uint8_t category;                // 0x100
        uint8_t pad_101[0x124 - 0x101];  // 0x101
        char scenarioName[0x40];         // 0x124
        char description[0x100];         // 0x164
        uint32_t flags;                  // 0x264
        char highscoreName[0x100];       // 0x268
        char objective[0x100];           // 0x368
        uint8_t pad_469[0x46C - 0x469];  // 0x469
        char currencyObjectId[8];        // 0x46C
        uint8_t pad_268[0x4478 - 0x474]; // 0x474
    };

    static_assert(offsetof(ScenarioIndexEntry, category) == 0x100);
    static_assert(offsetof(ScenarioIndexEntry, flags) == 0x264);
    static_assert(sizeof(ScenarioIndexEntry) == 0x4478);

    uint16_t getNumScenariosByCategory(uint8_t category);
    void loadIndex(uint8_t al);
}
