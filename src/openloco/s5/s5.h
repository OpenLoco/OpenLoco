#pragma once

#include "../objects/objectmgr.h"
#include <cstdint>

namespace openloco::s5
{
#pragma pack(push, 1)
    struct Header
    {
        std::byte var_0;
        std::byte var_1;
        uint16_t var_2;
        uint32_t var_4;
        uint32_t var_8;
        std::byte pad_C[20];
    };
#pragma pack(pop)
    static_assert(sizeof(Header) == 0x20);

#pragma pack(push, 1)
    struct Options
    {
        uint8_t editorStep;                        // 0x00
        uint8_t difficulty;                        // 0x01
        uint16_t scenarioStartYear;                // 0x02
        uint8_t pad_4[2];                          // 0x04
        uint16_t scenarioFlags;                    // 0x06
        uint8_t madeAnyChanges;                    // 0x08
        uint8_t pad_9[1];                          // 0x09
        uint8_t landDistributionPatterns[32];      // 0x0A
        char scenarioName[64];                     // 0x2A
        char scenarioDetails[256];                 // 0x6A
        objectmgr::header scenarioText;            // 0x16A
        uint16_t numberOfForests;                  // 0x17a
        uint8_t minForestRadius;                   // 0x17C
        uint8_t maxForestRadius;                   // 0x17D
        uint8_t minForestDensity;                  // 0x17E
        uint8_t maxForestDensity;                  // 0x17F
        uint16_t numberRandomTrees;                // 0x180
        uint8_t minAltitudeForTrees;               // 0x182
        uint8_t maxAltitudeForTrees;               // 0x183
        uint8_t minLandHeight;                     // 0x184
        uint8_t topographyStyle;                   // 0x185
        uint8_t hillDensity;                       // 0x186
        uint8_t numberOfTowns;                     // 0x187
        uint8_t maxTownSize;                       // 0x188
        uint8_t numberOfIndustries;                // 0x189
        uint8_t preview[128][128];                 // 0x18A
        uint8_t maxCompetingCompanies;             // 0x418A
        uint8_t competitorStartDelay;              // 0x418B
        uint8_t objectiveType;                     // 0x418C
        uint8_t objectiveFlags;                    // 0x418D
        uint32_t objectiveCompanyValue;            // 0x418E
        uint32_t objectiveMonthlyVehicleProfit;    // 0x4192
        uint8_t objectivePerformanceIndex;         // 0x4196
        uint8_t objectiveDeliveredCargoType;       // 0x4197
        uint32_t objectiveDeliveredCargoAmount;    // 0x4198
        uint8_t objectiveTimeLimitYears;           // 0x419C
        objectmgr::header objectiveDeliveredCargo; // 0x419D
        objectmgr::header currency;                // 0x41AD
        std::byte pad_41BD[349];
    };
#pragma pack(pop)

    static_assert(sizeof(Options) == 0x431A);
    static_assert(0x009C8714 + offsetof(Options, editorStep) == 0x009C8714);
    static_assert(0x009C8714 + offsetof(Options, difficulty) == 0x009C8715);
    static_assert(0x009C8714 + offsetof(Options, scenarioStartYear) == 0x009C8716);
    static_assert(0x009C8714 + offsetof(Options, scenarioFlags) == 0x009C871A);
    static_assert(0x009C8714 + offsetof(Options, madeAnyChanges) == 0x009C871C);
    static_assert(0x009C8714 + offsetof(Options, landDistributionPatterns) == 0x009C871E);
    static_assert(0x009C8714 + offsetof(Options, numberOfForests) == 0x009C888E);
    static_assert(0x009C8714 + offsetof(Options, minForestRadius) == 0x009C8890);
    static_assert(0x009C8714 + offsetof(Options, maxForestRadius) == 0x009C8891);
    static_assert(0x009C8714 + offsetof(Options, minForestDensity) == 0x009C8892);
    static_assert(0x009C8714 + offsetof(Options, maxForestDensity) == 0x009C8893);
    static_assert(0x009C8714 + offsetof(Options, numberRandomTrees) == 0x009C8894);
    static_assert(0x009C8714 + offsetof(Options, minAltitudeForTrees) == 0x009C8896);
    static_assert(0x009C8714 + offsetof(Options, maxAltitudeForTrees) == 0x009C8897);
    static_assert(0x009C8714 + offsetof(Options, minLandHeight) == 0x009C8898);
    static_assert(0x009C8714 + offsetof(Options, topographyStyle) == 0x009C8899);
    static_assert(0x009C8714 + offsetof(Options, hillDensity) == 0x009C889A);
    static_assert(0x009C8714 + offsetof(Options, numberOfTowns) == 0x009C889B);
    static_assert(0x009C8714 + offsetof(Options, maxTownSize) == 0x009C889C);
    static_assert(0x009C8714 + offsetof(Options, numberOfIndustries) == 0x009C889D);
    static_assert(0x009C8714 + offsetof(Options, scenarioName) == 0x009C873E);
    static_assert(0x009C8714 + offsetof(Options, scenarioDetails) == 0x009C877E);

    static_assert(0x009CCA54 + offsetof(Options, scenarioFlags) == 0x009CCA5A);
    static_assert(0x009CCA54 + offsetof(Options, preview) == 0x009CCBDE);

#pragma pack(push, 1)
    struct SaveDetails
    {
        char company[256];                   // 0x000
        char owner[256];                     // 0x100
        uint32_t date;                       // 0x200
        uint16_t performance_index;          // 0x204 (from [company.performance_index)
        char scenario[0x40];                 // 0x206
        uint8_t challenge_progress;          // 0x246
        std::byte pad_247;                   // 0x247
        uint8_t image[250 * 200];            // 0x248
        uint32_t challenge_flags;            // 0xC598 (from [company.challenge_flags])
        std::byte pad_C59C[0xC618 - 0xC59C]; // 0xC59C
    };
#pragma pack(pop)
    static_assert(sizeof(SaveDetails) == 0xC618);

    Options& getOptions();
    Options& getPreviewOptions();
}
