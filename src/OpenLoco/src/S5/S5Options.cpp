#include "S5Options.h"
#include "Scenario/ScenarioOptions.h"
#include <algorithm>

namespace OpenLoco::S5
{
    static OpenLoco::Scenario::Objective importObjective(const S5::Objective& src)
    {
        OpenLoco::Scenario::Objective dst{};
        dst.type = static_cast<OpenLoco::Scenario::ObjectiveType>(src.type);
        dst.flags = static_cast<OpenLoco::Scenario::ObjectiveFlags>(src.flags);
        dst.companyValue = src.companyValue;
        dst.monthlyVehicleProfit = src.monthlyVehicleProfit;
        dst.performanceIndex = src.performanceIndex;
        dst.deliveredCargoType = src.deliveredCargoType;
        dst.deliveredCargoAmount = src.deliveredCargoAmount;
        dst.timeLimitYears = src.timeLimitYears;
        return dst;
    }

    OpenLoco::Scenario::Options importOptions(const S5::Options& src)
    {
        OpenLoco::Scenario::Options dst{};
        dst.editorStep = static_cast<EditorController::Step>(src.editorStep);
        dst.difficulty = src.difficulty;
        dst.scenarioStartYear = src.scenarioStartYear;
        dst.scenarioFlags = static_cast<Scenario::ScenarioFlags>(src.scenarioFlags);
        dst.madeAnyChanges = src.madeAnyChanges;
        for (auto i = 0U; i < std::size(src.landDistributionPatterns); i++)
        {
            dst.landDistributionPatterns[i] = static_cast<Scenario::LandDistributionPattern>(src.landDistributionPatterns[i]);
        }
        std::ranges::copy(src.scenarioName, dst.scenarioName);
        std::ranges::copy(src.scenarioDetails, dst.scenarioDetails);
        dst.scenarioText = src.scenarioText;
        dst.numberOfForests = src.numberOfForests;
        dst.minForestRadius = src.minForestRadius;
        dst.maxForestRadius = src.maxForestRadius;
        dst.minForestDensity = src.minForestDensity;
        dst.maxForestDensity = src.maxForestDensity;
        dst.numberRandomTrees = src.numberRandomTrees;
        dst.minAltitudeForTrees = src.minAltitudeForTrees;
        dst.maxAltitudeForTrees = src.maxAltitudeForTrees;
        dst.minLandHeight = src.minLandHeight;
        dst.topographyStyle = static_cast<Scenario::TopographyStyle>(src.topographyStyle);
        dst.hillDensity = src.hillDensity;
        dst.numberOfTowns = src.numberOfTowns;
        dst.maxTownSize = src.maxTownSize;
        dst.numberOfIndustries = src.numberOfIndustries;
        std::memcpy(dst.preview, src.preview, sizeof(src.preview));
        dst.maxCompetingCompanies = src.maxCompetingCompanies;
        dst.competitorStartDelay = src.competitorStartDelay;
        dst.objective = importObjective(src.objective);
        dst.objectiveDeliveredCargo = src.objectiveDeliveredCargo;
        dst.currency = src.currency;

        dst.generator = static_cast<Scenario::LandGeneratorType>(src.generator);
        dst.numTerrainSmoothingPasses = src.numTerrainSmoothingPasses;
        dst.numRiverbeds = src.numRiverbeds;
        dst.minRiverWidth = src.minRiverWidth;
        dst.maxRiverWidth = src.maxRiverWidth;
        dst.riverbankWidth = src.riverbankWidth;
        dst.riverMeanderRate = src.riverMeanderRate;

        return dst;
    }

    static S5::Objective exportObjective(const OpenLoco::Scenario::Objective& src)
    {
        S5::Objective dst{};
        dst.type = static_cast<uint8_t>(src.type);
        dst.flags = static_cast<uint8_t>(src.flags);
        dst.companyValue = src.companyValue;
        dst.monthlyVehicleProfit = src.monthlyVehicleProfit;
        dst.performanceIndex = src.performanceIndex;
        dst.deliveredCargoType = src.deliveredCargoType;
        dst.deliveredCargoAmount = src.deliveredCargoAmount;
        dst.timeLimitYears = src.timeLimitYears;
        return dst;
    }

    S5::Options exportOptions(const OpenLoco::Scenario::Options& src)
    {
        S5::Options dst{};
        dst.editorStep = static_cast<uint8_t>(src.editorStep);
        dst.difficulty = src.difficulty;
        dst.scenarioStartYear = src.scenarioStartYear;
        dst.scenarioFlags = static_cast<uint16_t>(src.scenarioFlags);
        dst.madeAnyChanges = src.madeAnyChanges;
        for (auto i = 0U; i < std::size(src.landDistributionPatterns); i++)
        {
            dst.landDistributionPatterns[i] = static_cast<uint8_t>(src.landDistributionPatterns[i]);
        }
        std::ranges::copy(src.scenarioName, dst.scenarioName);
        std::ranges::copy(src.scenarioDetails, dst.scenarioDetails);
        dst.scenarioText = src.scenarioText;
        dst.numberOfForests = src.numberOfForests;
        dst.minForestRadius = src.minForestRadius;
        dst.maxForestRadius = src.maxForestRadius;
        dst.minForestDensity = src.minForestDensity;
        dst.maxForestDensity = src.maxForestDensity;
        dst.numberRandomTrees = src.numberRandomTrees;
        dst.minAltitudeForTrees = src.minAltitudeForTrees;
        dst.maxAltitudeForTrees = src.maxAltitudeForTrees;
        dst.minLandHeight = src.minLandHeight;
        dst.topographyStyle = static_cast<uint8_t>(src.topographyStyle);
        dst.hillDensity = src.hillDensity;
        dst.numberOfTowns = src.numberOfTowns;
        dst.maxTownSize = src.maxTownSize;
        dst.numberOfIndustries = src.numberOfIndustries;
        std::memcpy(dst.preview, src.preview, sizeof(src.preview));
        dst.maxCompetingCompanies = src.maxCompetingCompanies;
        dst.competitorStartDelay = src.competitorStartDelay;
        dst.objective = exportObjective(src.objective);
        dst.objectiveDeliveredCargo = src.objectiveDeliveredCargo;
        dst.currency = src.currency;

        dst.generator = static_cast<uint8_t>(src.generator);
        dst.numTerrainSmoothingPasses = src.numTerrainSmoothingPasses;
        dst.numRiverbeds = src.numRiverbeds;
        dst.minRiverWidth = src.minRiverWidth;
        dst.maxRiverWidth = src.maxRiverWidth;
        dst.riverbankWidth = src.riverbankWidth;
        dst.riverMeanderRate = src.riverMeanderRate;

        return dst;
    }

}
