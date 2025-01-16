#pragma once
#include "EditorController.h"
#include "Objects/Object.h"
#include "ScenarioObjective.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <cstddef>

namespace OpenLoco
{
    enum class ScenarioFlags : uint16_t
    {
        none = 0U,
        landscapeGenerationDone = (1U << 0),
        hillsEdgeOfMap = (1U << 1),
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ScenarioFlags);

    enum class TopographyStyle : uint8_t
    {
        flatLand,
        smallHills,
        mountains,
        halfMountainsHills,
        halfMountainsFlat,
    };

    enum class LandGeneratorType : uint8_t
    {
        Original,
        Simplex,
        PngHeightMap,
    };

    enum class LandDistributionPattern : uint8_t
    {
        everywhere,
        nowhere,
        farFromWater,
        nearWater,
        onMountains,
        farFromMountains,
        inSmallRandomAreas,
        inLargeRandomAreas,
        aroundCliffs,
    };

#pragma pack(push, 1)
    struct Options
    {
        EditorController::Step editorStep;                    // 0x00
        uint8_t difficulty;                                   // 0x01
        uint16_t scenarioStartYear;                           // 0x02
        uint8_t pad_4[2];                                     // 0x04
        ScenarioFlags scenarioFlags;                          // 0x06
        uint8_t madeAnyChanges;                               // 0x08
        uint8_t pad_9[1];                                     // 0x09
        LandDistributionPattern landDistributionPatterns[32]; // 0x0A
        char scenarioName[64];                                // 0x2A
        char scenarioDetails[256];                            // 0x6A
        ObjectHeader scenarioText;                            // 0x16A
        uint16_t numberOfForests;                             // 0x17A
        uint8_t minForestRadius;                              // 0x17C
        uint8_t maxForestRadius;                              // 0x17D
        uint8_t minForestDensity;                             // 0x17E
        uint8_t maxForestDensity;                             // 0x17F
        uint16_t numberRandomTrees;                           // 0x180
        uint8_t minAltitudeForTrees;                          // 0x182
        uint8_t maxAltitudeForTrees;                          // 0x183
        uint8_t minLandHeight;                                // 0x184
        TopographyStyle topographyStyle;                      // 0x185
        uint8_t hillDensity;                                  // 0x186
        uint8_t numberOfTowns;                                // 0x187
        uint8_t maxTownSize;                                  // 0x188
        uint8_t numberOfIndustries;                           // 0x189
        uint8_t preview[128][128];                            // 0x18A
        uint8_t maxCompetingCompanies;                        // 0x418A
        uint8_t competitorStartDelay;                         // 0x418B
        Scenario::Objective objective;                        // 0x418C
        ObjectHeader objectiveDeliveredCargo;                 // 0x419D
        ObjectHeader currency;                                // 0x41AD

        // new fields:
        LandGeneratorType generator;
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

    static_assert(0x009C8714 + offsetof(Options, objective.type) == 0x009CC8A0);
    static_assert(0x009C8714 + offsetof(Options, objective.flags) == 0x009CC8A1);
    static_assert(0x009C8714 + offsetof(Options, objective.companyValue) == 0x009CC8A2);
    static_assert(0x009C8714 + offsetof(Options, objective.monthlyVehicleProfit) == 0x009CC8A6);
    static_assert(0x009C8714 + offsetof(Options, objective.performanceIndex) == 0x009CC8AA);
    static_assert(0x009C8714 + offsetof(Options, objective.deliveredCargoType) == 0x009CC8AB);
    static_assert(0x009C8714 + offsetof(Options, objective.deliveredCargoAmount) == 0x009CC8AC);
    static_assert(0x009C8714 + offsetof(Options, objective.timeLimitYears) == 0x009CC8B0);

    static_assert(0x009C8714 + offsetof(Options, objectiveDeliveredCargo) == 0x009CC8B1);
    static_assert(0x009C8714 + offsetof(Options, currency) == 0x009CC8C1);

    static_assert(0x009CCA54 + offsetof(Options, scenarioFlags) == 0x009CCA5A);
    static_assert(0x009CCA54 + offsetof(Options, preview) == 0x009CCBDE);

    Options& getOptions();
}
