#pragma once

#include <cstddef>
#include <cstdint>

namespace OpenLoco::ScenarioManager
{
    struct ScenarioIndexEntry
    {
        uint8_t pad_00[0x100];           // 0x000
        uint8_t category;                // 0x100
        uint8_t pad_101[0x264 - 0x101];  // 0x101
        uint32_t var_264;                // 0x264
        uint8_t pad_268[0x4478 - 0x268]; // 0x268
    };

    static_assert(offsetof(ScenarioIndexEntry, category) == 0x100);
    static_assert(offsetof(ScenarioIndexEntry, var_264) == 0x264);
    static_assert(sizeof(ScenarioIndexEntry) == 0x4478);

    uint16_t getNumScenariosByCategory(uint8_t category);
    void loadIndex(uint8_t al);
}
