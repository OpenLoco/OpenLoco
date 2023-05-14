#pragma once

#include "Objects/Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>

#include <cstddef>
#include <cstdint>

namespace OpenLoco::Scenario
{
    struct ObjectiveProgress;
}

namespace OpenLoco::ScenarioManager
{
    enum class ScenarioIndexFlags : uint32_t
    {
        none = 0U,
        flag_0 = 1U << 0,
        completed = 1U << 1,
        hasPreviewImage = 1U << 2,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ScenarioIndexFlags);

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
        ScenarioIndexFlags flags;       // 0x264
        char highscoreName[0x100];      // 0x268
        char objective[0x100];          // 0x368
        ObjectHeader currency;          // 0x468
        uint8_t preview[128][128];      // 0x478

        bool hasFlag(ScenarioIndexFlags flag)
        {
            return (flags & flag) != ScenarioIndexFlags::none;
        }
    };

    static_assert(offsetof(ScenarioIndexEntry, category) == 0x100);
    static_assert(offsetof(ScenarioIndexEntry, flags) == 0x264);
    static_assert(sizeof(ScenarioIndexEntry) == 0x4478);

    bool hasScenariosForCategory(uint8_t category);
    bool hasScenarioInCategory(uint8_t category, ScenarioIndexEntry* scenario);
    uint16_t getScenarioCountByCategory(uint8_t category);
    ScenarioIndexEntry* getNthScenarioFromCategory(uint8_t category, uint8_t index);
    void loadIndex(const bool forceReload = false);
    void saveNewScore(Scenario::ObjectiveProgress& progress, const CompanyId companyId);

    // 0x00525F5E
    uint32_t getScenarioTicks();
    void setScenarioTicks(uint32_t ticks);

    // 0x00525F64
    uint32_t getScenarioTicks2();
    void setScenarioTicks2(uint32_t ticks);
}
