#include "S5GameState.h"
#include "GameState.h"
#include "S5.h"
#include "ScenarioConstruction.h"

namespace OpenLoco::S5
{
    static S5::Construction exportConstruction(const OpenLoco::Scenario::Construction& src)
    {
        S5::Construction dst{};
        std::ranges::copy(src.signals, dst.signals);
        std::ranges::copy(src.bridges, dst.bridges);
        std::ranges::copy(src.trainStations, dst.trainStations);
        std::ranges::copy(src.trackMods, dst.trackMods);
        std::ranges::copy(src.var_17A, dst.var_17A);
        std::ranges::copy(src.roadStations, dst.roadStations);
        std::ranges::copy(src.roadMods, dst.roadMods);
        return dst;
    }

    static S5::GeneralState exportGeneralState(const OpenLoco::GameState& src)
    {
        S5::GeneralState dst{};
        dst.rng[0] = src.rng.srand_0();
        dst.rng[1] = src.rng.srand_1();
        dst.unkRng[0] = src.unkRng.srand_0();
        dst.unkRng[1] = src.unkRng.srand_1();
        dst.flags = enumValue(src.flags);
        dst.currentDay = src.currentDay;
        dst.dayCounter = src.dayCounter;
        dst.currentYear = src.currentYear;
        dst.currentMonth = src.currentMonth;
        dst.currentDayOfMonth = src.currentDayOfMonth;
        dst.savedViewX = src.savedViewX;
        dst.savedViewY = src.savedViewY;
        dst.savedViewZoom = src.savedViewZoom;
        dst.savedViewRotation = src.savedViewRotation;
        dst.playerCompanies[0] = enumValue(src.playerCompanies[0]);
        dst.playerCompanies[1] = enumValue(src.playerCompanies[1]);

        for (auto i = 0U; i < std::size(src.entityListHeads); ++i)
        {
            dst.entityListHeads[i] = enumValue(src.entityListHeads[i]);
        }
        std::ranges::copy(src.entityListCounts, dst.entityListCounts);
        std::ranges::copy(src.currencyMultiplicationFactor, dst.currencyMultiplicationFactor);
        std::ranges::copy(src.unusedCurrencyMultiplicationFactor, dst.unusedCurrencyMultiplicationFactor);
        dst.scenarioTicks = src.scenarioTicks;
        dst.var_014A = src.var_014A;
        dst.scenarioTicks2 = src.scenarioTicks2;
        dst.magicNumber = kMagicNumber; // Match implementation at 0x004437FC
        dst.numMapAnimations = src.numMapAnimations;
        dst.tileUpdateStartLocation = src.tileUpdateStartLocation;
        dst.scenarioConstruction = exportConstruction(src.scenarioConstruction);
        dst.lastRailroadOption = src.lastRailroadOption;
        dst.lastRoadOption = src.lastRoadOption;
        dst.lastAirport = src.lastAirport;
        dst.lastShipPort = src.lastShipPort;
        dst.trafficHandedness = src.trafficHandedness;
        dst.lastVehicleType = enumValue(src.lastVehicleType);
        dst.pickupDirection = src.pickupDirection;
        dst.lastTreeOption = src.lastTreeOption;
        dst.seaLevel = src.seaLevel;
        dst.currentSnowLine = src.currentSnowLine;
        dst.currentSeason = enumValue(src.currentSeason);
        dst.lastLandOption = src.lastLandOption;
        dst.maxCompetingCompanies = src.maxCompetingCompanies;
        dst.orderTableLength = src.orderTableLength;
        dst.roadObjectIdIsNotTram = src.roadObjectIdIsNotTram;
        dst.roadObjectIdIsFlag7 = src.roadObjectIdIsFlag7;
        dst.currentDefaultLevelCrossingType = src.currentDefaultLevelCrossingType;
        dst.lastTrackTypeOption = src.lastTrackTypeOption;
        dst.loanInterestRate = src.loanInterestRate;
        dst.lastIndustryOption = src.lastIndustryOption;
        dst.lastBuildingOption = src.lastBuildingOption;
        dst.lastMiscBuildingOption = src.lastMiscBuildingOption;
        dst.lastWallOption = src.lastWallOption;
        dst.produceAICompanyTimeout = src.produceAICompanyTimeout;
        dst.tickStartPrngState[0] = src.tickStartPrngState.srand_0();
        dst.tickStartPrngState[1] = src.tickStartPrngState.srand_1();
        std::ranges::copy(src.scenarioFileName, dst.scenarioFileName);
        std::ranges::copy(src.scenarioName, dst.scenarioName);
        std::ranges::copy(src.scenarioDetails, dst.scenarioDetails);
        dst.competitorStartDelay = src.competitorStartDelay;
        dst.preferredAIIntelligence = src.preferredAIIntelligence;
        dst.preferredAIAggressiveness = src.preferredAIAggressiveness;
        dst.preferredAICompetitiveness = src.preferredAICompetitiveness;
        dst.startingLoanSize = src.startingLoanSize;
        dst.maxLoanSize = src.maxLoanSize;
        dst.multiplayerPrng[0] = src.multiplayerPrng.srand_0();
        dst.multiplayerPrng[1] = src.multiplayerPrng.srand_1();
        dst.multiplayerChecksumA = src.multiplayerChecksumA;
        dst.multiplayerChecksumB = src.multiplayerChecksumB;
        dst.lastBuildVehiclesOption = enumValue(src.lastBuildVehiclesOption);
        dst.numberOfIndustries = src.numberOfIndustries;
        dst.vehiclePreviewRotationFrame = src.vehiclePreviewRotationFrame;
        dst.objectiveType = enumValue(src.scenarioObjective.type);
        dst.objectiveFlags = enumValue(src.scenarioObjective.flags);
        dst.objectiveCompanyValue = src.scenarioObjective.companyValue;
        dst.objectiveMonthlyVehicleProfit = src.scenarioObjective.monthlyVehicleProfit;
        dst.objectivePerformanceIndex = src.scenarioObjective.performanceIndex;
        dst.objectiveDeliveredCargoType = src.scenarioObjective.deliveredCargoType;
        dst.objectiveDeliveredCargoAmount = src.scenarioObjective.deliveredCargoAmount;
        dst.objectiveTimeLimitYears = src.scenarioObjective.timeLimitYears;
        dst.objectiveTimeLimitUntilYear = src.scenarioObjectiveProgress.timeLimitUntilYear;
        dst.objectiveMonthsInChallenge = src.scenarioObjectiveProgress.monthsInChallenge;
        dst.objectiveCompletedChallengeInMonths = src.scenarioObjectiveProgress.completedChallengeInMonths;
        dst.industryFlags = enumValue(src.industryFlags);
        dst.forbiddenVehiclesPlayers = src.forbiddenVehiclesPlayers;
        dst.forbiddenVehiclesCompetitors = src.forbiddenVehiclesCompetitors;
        dst.fixFlags = src.fixFlags;
        dst.companyRecords = exportRecords(src.companyRecords);
        dst.var_44C = src.var_44C;
        dst.var_450 = src.var_450;
        dst.var_454 = src.var_454;
        dst.var_458 = src.var_458;
        dst.var_45C = src.var_45C;
        dst.var_460 = src.var_460;
        dst.var_464 = src.var_464;
        dst.var_468 = src.var_468;
        dst.lastMapWindowFlags = enumValue(src.lastMapWindowAttributes.flags);
        dst.lastMapWindowSize[0] = src.lastMapWindowAttributes.size.width;
        dst.lastMapWindowSize[1] = src.lastMapWindowAttributes.size.height;
        dst.lastMapWindowVar88A = src.lastMapWindowAttributes.var88A;
        dst.lastMapWindowVar88C = src.lastMapWindowAttributes.var88C;

        dst.var_478 = src.var_478;
        dst.numMessages = src.numMessages;
        dst.activeMessageIndex = enumValue(src.activeMessageIndex);
        for (auto i = 0U; i < std::min<uint16_t>(std::size(src.messages), src.numMessages); i++)
        {
            dst.messages[i] = exportMessage(src.messages[i]);
        }
        dst.var_B95C = src.var_B95C;
        dst.var_B960 = src.var_B960;
        dst.var_B962 = src.var_B962;
        dst.var_B964 = src.var_B964;
        dst.var_B966 = src.var_B966;
        dst.currentRainLevel = src.currentRainLevel;
        return dst;
    }

    std::unique_ptr<S5::GameState> exportGameState(const OpenLoco::GameState& src)
    {
        auto dst = std::make_unique<S5::GameState>();
        dst->general = exportGeneralState(src);
        for (auto i = 0U; i < std::size(src.companies); i++)
        {
            dst->companies[i] = exportCompany(src.companies[i]);
        }
        for (auto i = 0U; i < std::size(src.towns); i++)
        {
            dst->towns[i] = exportTown(src.towns[i]);
        }
        for (auto i = 0U; i < std::size(src.industries); i++)
        {
            dst->industries[i] = exportIndustry(src.industries[i]);
        }
        for (auto i = 0U; i < std::size(src.stations); i++)
        {
            dst->stations[i] = exportStation(src.stations[i]);
        }
        for (auto i = 0U; i < std::size(src.entities); i++)
        {
            dst->entities[i] = exportEntity(src.entities[i]);
        }
        for (auto i = 0U; i < std::size(src.animations); i++)
        {
            dst->animations[i] = exportAnimation(src.animations[i]);
        }
        for (auto i = 0U; i < std::size(src.waves); i++)
        {
            dst->waves[i] = exportWave(src.waves[i]);
        }
        for (auto i = 0U; i < Limits::kMaxUserStrings; i++)
        {
            std::ranges::copy(src.userStrings[i], dst->userStrings[i]);
        }
        for (auto i = 0U; i < Limits::kMaxVehicles; i++)
        {
            std::ranges::copy(src.routings[i], dst->routings[i]);
        }
        std::ranges::copy(src.orders, dst->orders);
        return dst;
    }

    std::unique_ptr<S5::GameState> importGameStateType2(const S5::GameStateType2& src)
    {
        auto dst = std::make_unique<S5::GameState>();
        dst->general = src.general;
        for (auto i = 0U; i < std::size(src.companies); i++)
        {
            dst->companies[i] = importCompanyType2(src.companies[i]);
        }
        std::ranges::copy(src.towns, dst->towns);
        std::ranges::copy(src.industries, dst->industries);
        std::ranges::copy(src.stations, dst->stations);
        std::ranges::copy(src.entities, dst->entities);
        std::ranges::copy(src.animations, dst->animations);
        std::ranges::copy(src.waves, dst->waves);
        for (auto i = 0U; i < Limits::kMaxUserStrings; i++)
        {
            std::ranges::copy(src.userStrings[i], dst->userStrings[i]);
        }
        for (auto i = 0U; i < Limits::kMaxVehicles; i++)
        {
            std::ranges::copy(src.routings[i], dst->routings[i]);
        }
        std::ranges::copy(src.orders, dst->orders);
        return dst;
    }

    static OpenLoco::Scenario::Construction importConstruction(const S5::Construction& src)
    {
        OpenLoco::Scenario::Construction dst{};
        std::ranges::copy(src.signals, dst.signals);
        std::ranges::copy(src.bridges, dst.bridges);
        std::ranges::copy(src.trainStations, dst.trainStations);
        std::ranges::copy(src.trackMods, dst.trackMods);
        std::ranges::copy(src.var_17A, dst.var_17A);
        std::ranges::copy(src.roadStations, dst.roadStations);
        std::ranges::copy(src.roadMods, dst.roadMods);

        return dst;
    }

    static void importGeneralState(OpenLoco::GameState& dst, const S5::GeneralState& src)
    {
        dst.rng = Core::Prng(src.rng[0], src.rng[1]);
        dst.unkRng = Core::Prng(src.unkRng[0], src.unkRng[1]);
        dst.flags = static_cast<GameStateFlags>(src.flags);
        dst.currentDay = src.currentDay;
        dst.dayCounter = src.dayCounter;
        dst.currentYear = src.currentYear;
        dst.currentMonth = src.currentMonth;
        dst.currentDayOfMonth = src.currentDayOfMonth;
        dst.savedViewX = src.savedViewX;
        dst.savedViewY = src.savedViewY;
        dst.savedViewZoom = src.savedViewZoom;
        dst.savedViewRotation = src.savedViewRotation;
        for (auto i = 0U; i < std::size(dst.playerCompanies); ++i)
        {
            dst.playerCompanies[i] = static_cast<CompanyId>(src.playerCompanies[i]);
        }
        for (auto i = 0U; i < std::size(src.entityListHeads); ++i)
        {
            dst.entityListHeads[i] = static_cast<EntityId>(src.entityListHeads[i]);
        }
        std::ranges::copy(src.entityListCounts, dst.entityListCounts);
        std::ranges::copy(src.currencyMultiplicationFactor, dst.currencyMultiplicationFactor);
        std::ranges::copy(src.unusedCurrencyMultiplicationFactor, dst.unusedCurrencyMultiplicationFactor);
        dst.scenarioTicks = src.scenarioTicks;
        dst.var_014A = src.var_014A;
        dst.scenarioTicks2 = src.scenarioTicks2;
        dst.magicNumber = src.magicNumber;
        dst.numMapAnimations = src.numMapAnimations;
        dst.tileUpdateStartLocation = src.tileUpdateStartLocation;
        dst.scenarioConstruction = importConstruction(src.scenarioConstruction);
        dst.lastRailroadOption = src.lastRailroadOption;
        dst.lastRoadOption = src.lastRoadOption;
        dst.lastAirport = src.lastAirport;
        dst.lastShipPort = src.lastShipPort;
        dst.trafficHandedness = src.trafficHandedness;
        dst.lastVehicleType = static_cast<VehicleType>(src.lastVehicleType);
        dst.pickupDirection = src.pickupDirection;
        dst.lastTreeOption = src.lastTreeOption;
        dst.seaLevel = src.seaLevel;
        dst.currentSnowLine = src.currentSnowLine;
        dst.currentSeason = static_cast<Scenario::Season>(src.currentSeason);
        dst.lastLandOption = src.lastLandOption;
        dst.maxCompetingCompanies = src.maxCompetingCompanies;
        dst.orderTableLength = src.orderTableLength;
        dst.roadObjectIdIsNotTram = src.roadObjectIdIsNotTram;
        dst.roadObjectIdIsFlag7 = src.roadObjectIdIsFlag7;
        dst.currentDefaultLevelCrossingType = src.currentDefaultLevelCrossingType;
        dst.lastTrackTypeOption = src.lastTrackTypeOption;
        dst.loanInterestRate = src.loanInterestRate;
        dst.lastIndustryOption = src.lastIndustryOption;
        dst.lastBuildingOption = src.lastBuildingOption;
        dst.lastMiscBuildingOption = src.lastMiscBuildingOption;
        dst.lastWallOption = src.lastWallOption;
        dst.produceAICompanyTimeout = src.produceAICompanyTimeout;
        dst.tickStartPrngState = Core::Prng(src.tickStartPrngState[0], src.tickStartPrngState[1]);
        std::ranges::copy(src.scenarioFileName, dst.scenarioFileName);
        std::ranges::copy(src.scenarioName, dst.scenarioName);
        std::ranges::copy(src.scenarioDetails, dst.scenarioDetails);
        dst.competitorStartDelay = src.competitorStartDelay;
        dst.preferredAIIntelligence = src.preferredAIIntelligence;
        dst.preferredAIAggressiveness = src.preferredAIAggressiveness;
        dst.preferredAICompetitiveness = src.preferredAICompetitiveness;
        dst.startingLoanSize = src.startingLoanSize;
        dst.maxLoanSize = src.maxLoanSize;
        dst.multiplayerPrng = Core::Prng(src.multiplayerPrng[0], src.multiplayerPrng[1]);
        dst.multiplayerChecksumA = src.multiplayerChecksumA;
        dst.multiplayerChecksumB = src.multiplayerChecksumB;
        dst.lastBuildVehiclesOption = static_cast<VehicleType>(src.lastBuildVehiclesOption);
        dst.numberOfIndustries = src.numberOfIndustries;
        dst.vehiclePreviewRotationFrame = src.vehiclePreviewRotationFrame;
        dst.scenarioObjective.type = static_cast<Scenario::ObjectiveType>(src.objectiveType);
        dst.scenarioObjective.flags = static_cast<Scenario::ObjectiveFlags>(src.objectiveFlags);
        dst.scenarioObjective.companyValue = src.objectiveCompanyValue;
        dst.scenarioObjective.monthlyVehicleProfit = src.objectiveMonthlyVehicleProfit;
        dst.scenarioObjective.performanceIndex = src.objectivePerformanceIndex;
        dst.scenarioObjective.deliveredCargoType = src.objectiveDeliveredCargoType;
        dst.scenarioObjective.deliveredCargoAmount = src.objectiveDeliveredCargoAmount;
        dst.scenarioObjective.timeLimitYears = src.objectiveTimeLimitYears;
        dst.scenarioObjectiveProgress.timeLimitUntilYear = src.objectiveTimeLimitUntilYear;
        dst.scenarioObjectiveProgress.monthsInChallenge = src.objectiveMonthsInChallenge;
        dst.scenarioObjectiveProgress.completedChallengeInMonths = src.objectiveCompletedChallengeInMonths;
        dst.industryFlags = static_cast<IndustryManager::Flags>(src.industryFlags);
        dst.forbiddenVehiclesPlayers = src.forbiddenVehiclesPlayers;
        dst.forbiddenVehiclesCompetitors = src.forbiddenVehiclesCompetitors;
        dst.fixFlags = src.fixFlags;
        dst.companyRecords = importRecords(src.companyRecords);
        dst.var_44C = src.var_44C;
        dst.var_450 = src.var_450;
        dst.var_454 = src.var_454;
        dst.var_458 = src.var_458;
        dst.var_45C = src.var_45C;
        dst.var_460 = src.var_460;
        dst.var_464 = src.var_464;
        dst.var_468 = src.var_468;
        dst.lastMapWindowAttributes.flags = static_cast<Ui::WindowFlags>(src.lastMapWindowFlags);
        dst.lastMapWindowAttributes.size.width = src.lastMapWindowSize[0];
        dst.lastMapWindowAttributes.size.height = src.lastMapWindowSize[1];
        dst.lastMapWindowAttributes.var88A = src.lastMapWindowVar88A;
        dst.lastMapWindowAttributes.var88C = src.lastMapWindowVar88C;
        dst.var_478 = src.var_478;
        dst.numMessages = src.numMessages;
        dst.activeMessageIndex = static_cast<MessageId>(src.activeMessageIndex);
        for (auto i = 0U; i < std::min<uint16_t>(std::size(src.messages), dst.numMessages); i++)
        {
            dst.messages[i] = importMessage(src.messages[i]);
        }
        dst.var_B95C = src.var_B95C;
        dst.var_B960 = src.var_B960;
        dst.var_B962 = src.var_B962;
        dst.var_B964 = src.var_B964;
        dst.var_B966 = src.var_B966;
        dst.currentRainLevel = src.currentRainLevel;
    }

    std::unique_ptr<OpenLoco::GameState> importGameState(const S5::GameState& src)
    {
        auto dst = std::make_unique<OpenLoco::GameState>();
        importGeneralState(*dst, src.general);
        for (auto i = 0U; i < std::size(src.companies); i++)
        {
            dst->companies[i] = importCompany(src.companies[i]);
        }
        for (auto i = 0U; i < std::size(src.towns); i++)
        {
            dst->towns[i] = importTown(src.towns[i]);
        }
        for (auto i = 0U; i < std::size(src.industries); i++)
        {
            dst->industries[i] = importIndustry(src.industries[i]);
        }
        for (auto i = 0U; i < std::size(src.stations); i++)
        {
            dst->stations[i] = importStation(src.stations[i]);
        }
        for (auto i = 0U; i < std::size(src.entities); i++)
        {
            dst->entities[i] = importEntity(src.entities[i]);
        }
        for (auto i = 0U; i < std::size(src.animations); i++)
        {
            dst->animations[i] = importAnimation(src.animations[i]);
        }
        for (auto i = 0U; i < std::size(src.waves); i++)
        {
            dst->waves[i] = importWave(src.waves[i]);
        }
        for (auto i = 0U; i < Limits::kMaxUserStrings; i++)
        {
            std::ranges::copy(src.userStrings[i], dst->userStrings[i]);
        }
        for (auto i = 0U; i < Limits::kMaxVehicles; i++)
        {
            std::ranges::copy(src.routings[i], dst->routings[i]);
        }
        std::ranges::copy(src.orders, dst->orders);
        return dst;
    }
}
