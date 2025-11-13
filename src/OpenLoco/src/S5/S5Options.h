#include "Objects/Object.h"

namespace OpenLoco::Scenario
{
    struct Options;
}

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Objective
    {
        uint8_t type;                  // 0x000418 (0x00526230)
        uint8_t flags;                 // 0x000419 (0x00526231)
        uint32_t companyValue;         // 0x00041A (0x00526232)
        uint32_t monthlyVehicleProfit; // 0x00041E (0x00526236)
        uint8_t performanceIndex;      // 0x000422 (0x0052623A)
        uint8_t deliveredCargoType;    // 0x000423 (0x0052623B)
        uint32_t deliveredCargoAmount; // 0x000424 (0x0052623C)
        uint8_t timeLimitYears;        // 0x000428 (0x00526240)
    };
    static_assert(sizeof(Objective) == 0x11);

    struct Options
    {
        uint8_t editorStep;                   // 0x00
        uint8_t difficulty;                   // 0x01
        uint16_t scenarioStartYear;           // 0x02
        uint8_t pad_4[2];                     // 0x04
        uint16_t scenarioFlags;               // 0x06
        uint8_t madeAnyChanges;               // 0x08
        uint8_t pad_9[1];                     // 0x09
        uint8_t landDistributionPatterns[32]; // 0x0A
        char scenarioName[64];                // 0x2A
        char scenarioDetails[256];            // 0x6A
        ObjectHeader scenarioText;            // 0x16A
        uint16_t numberOfForests;             // 0x17A
        uint8_t minForestRadius;              // 0x17C
        uint8_t maxForestRadius;              // 0x17D
        uint8_t minForestDensity;             // 0x17E
        uint8_t maxForestDensity;             // 0x17F
        uint16_t numberRandomTrees;           // 0x180
        uint8_t minAltitudeForTrees;          // 0x182
        uint8_t maxAltitudeForTrees;          // 0x183
        uint8_t minLandHeight;                // 0x184
        uint8_t topographyStyle;              // 0x185
        uint8_t hillDensity;                  // 0x186
        uint8_t numberOfTowns;                // 0x187
        uint8_t maxTownSize;                  // 0x188
        uint8_t numberOfIndustries;           // 0x189
        uint8_t preview[128][128];            // 0x18A
        uint8_t maxCompetingCompanies;        // 0x418A
        uint8_t competitorStartDelay;         // 0x418B
        Objective objective;                  // 0x418C
        ObjectHeader objectiveDeliveredCargo; // 0x419D
        ObjectHeader currency;                // 0x41AD

        // new fields:
        uint8_t generator;
        uint8_t numTerrainSmoothingPasses;
        uint8_t numRiverbeds;
        uint8_t minRiverWidth;
        uint8_t maxRiverWidth;
        uint8_t riverbankWidth;
        uint8_t riverMeanderRate;

        std::byte pad_41BD[342];
    };
#pragma pack(pop)
    static_assert(sizeof(Options) == 0x431A);

    OpenLoco::Scenario::Options importOptions(const S5::Options& src);
    S5::Options exportOptions(const OpenLoco::Scenario::Options& src);
}
