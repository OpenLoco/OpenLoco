#pragma once
#include "EditorController.h"
#include "Objects/Object.h"
#include "ScenarioObjective.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <cstddef>

namespace OpenLoco::Scenario
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

    struct Options
    {
        EditorController::Step editorStep;                    // 0x00
        uint8_t difficulty;                                   // 0x01
        uint16_t scenarioStartYear;                           // 0x02
        ScenarioFlags scenarioFlags;                          // 0x06
        uint8_t madeAnyChanges;                               // 0x08
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
    };

    Options& getOptions();
}
