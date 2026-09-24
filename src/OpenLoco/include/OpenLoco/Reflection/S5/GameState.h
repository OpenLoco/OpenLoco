#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/Reflection/Economy/Currency.h>
#include <OpenLoco/Reflection/Engine/World.h>
#include <OpenLoco/Reflection/S5/Animation.h>
#include <OpenLoco/Reflection/S5/Company.h>
#include <OpenLoco/Reflection/S5/Entity.h>
#include <OpenLoco/Reflection/S5/Industry.h>
#include <OpenLoco/Reflection/S5/LabelFrame.h>
#include <OpenLoco/Reflection/S5/Message.h>
#include <OpenLoco/Reflection/S5/Station.h>
#include <OpenLoco/Reflection/S5/Town.h>
#include <OpenLoco/Reflection/S5/Wave.h>
#include <OpenLoco/S5/S5GameState.h>

namespace OpenLoco
{
    template<>
    struct Reflection<S5::Construction>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::Construction, signals),
            REFL_FIELD(S5::Construction, bridges),
            REFL_FIELD(S5::Construction, trainStations),
            REFL_FIELD(S5::Construction, trackMods),
            REFL_FIELD(S5::Construction, var_17A),
            REFL_FIELD(S5::Construction, roadStations),
            REFL_FIELD(S5::Construction, roadMods)>;
    };

    template<>
    struct Reflection<S5::GeneralState>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::GeneralState, rng),
            REFL_FIELD(S5::GeneralState, unkRng),
            REFL_FIELD(S5::GeneralState, flags),
            REFL_FIELD(S5::GeneralState, currentDay),
            REFL_FIELD(S5::GeneralState, dayCounter),
            REFL_FIELD(S5::GeneralState, currentYear),
            REFL_FIELD(S5::GeneralState, currentMonth),
            REFL_FIELD(S5::GeneralState, currentDayOfMonth),
            REFL_FIELD(S5::GeneralState, savedViewX),
            REFL_FIELD(S5::GeneralState, savedViewY),
            REFL_FIELD(S5::GeneralState, savedViewZoom),
            REFL_FIELD(S5::GeneralState, savedViewRotation),
            REFL_FIELD(S5::GeneralState, playerCompanies),
            REFL_FIELD(S5::GeneralState, entityListHeads),
            REFL_FIELD(S5::GeneralState, entityListCounts),
            REFL_FIELD(S5::GeneralState, pad_0042),
            REFL_FIELD(S5::GeneralState, currencyMultiplicationFactor),
            REFL_FIELD(S5::GeneralState, unusedCurrencyMultiplicationFactor),
            REFL_FIELD(S5::GeneralState, scenarioTicks),
            REFL_FIELD(S5::GeneralState, var_014A),
            REFL_FIELD(S5::GeneralState, scenarioTicks2),
            REFL_FIELD(S5::GeneralState, magicNumber),
            REFL_FIELD(S5::GeneralState, numMapAnimations),
            REFL_FIELD(S5::GeneralState, tileUpdateStartLocation),
            REFL_FIELD(S5::GeneralState, scenarioConstruction),
            REFL_FIELD(S5::GeneralState, defaultRailroadObjectId),
            REFL_FIELD(S5::GeneralState, defaultRoadObjectId),
            REFL_FIELD(S5::GeneralState, lastAirport),
            REFL_FIELD(S5::GeneralState, lastShipPort),
            REFL_FIELD(S5::GeneralState, trafficHandedness),
            REFL_FIELD(S5::GeneralState, lastVehicleType),
            REFL_FIELD(S5::GeneralState, pickupDirection),
            REFL_FIELD(S5::GeneralState, defaultTreeObjectId),
            REFL_FIELD(S5::GeneralState, seaLevel),
            REFL_FIELD(S5::GeneralState, currentSnowLine),
            REFL_FIELD(S5::GeneralState, currentSeason),
            REFL_FIELD(S5::GeneralState, defaultLandObjectId),
            REFL_FIELD(S5::GeneralState, maxCompetingCompanies),
            REFL_FIELD(S5::GeneralState, orderTableLength),
            REFL_FIELD(S5::GeneralState, roadObjectIdIsAnyRoadTypeCompatible),
            REFL_FIELD(S5::GeneralState, roadObjectIdIsUsableByAllCompanies),
            REFL_FIELD(S5::GeneralState, currentDefaultLevelCrossingType),
            REFL_FIELD(S5::GeneralState, defaultTrackTypeObjectId),
            REFL_FIELD(S5::GeneralState, loanInterestRate),
            REFL_FIELD(S5::GeneralState, defaultIndustryObjectId),
            REFL_FIELD(S5::GeneralState, defaultBuildingObjectId),
            REFL_FIELD(S5::GeneralState, defaultMiscBuildingObjectId),
            REFL_FIELD(S5::GeneralState, defaultWallObjectId),
            REFL_FIELD(S5::GeneralState, produceAICompanyTimeout),
            REFL_FIELD(S5::GeneralState, tickStartPrngState),
            REFL_FIELD(S5::GeneralState, scenarioFileName),
            REFL_FIELD(S5::GeneralState, scenarioName),
            REFL_FIELD(S5::GeneralState, scenarioDetails),
            REFL_FIELD(S5::GeneralState, competitorStartDelay),
            REFL_FIELD(S5::GeneralState, preferredAIIntelligence),
            REFL_FIELD(S5::GeneralState, preferredAIAggressiveness),
            REFL_FIELD(S5::GeneralState, preferredAICompetitiveness),
            REFL_FIELD(S5::GeneralState, startingLoanSize),
            REFL_FIELD(S5::GeneralState, maxLoanSize),
            REFL_FIELD(S5::GeneralState, multiplayerPrng),
            REFL_FIELD(S5::GeneralState, multiplayerChecksumA),
            REFL_FIELD(S5::GeneralState, multiplayerChecksumB),
            REFL_FIELD(S5::GeneralState, defaultBuildVehicleType),
            REFL_FIELD(S5::GeneralState, numberOfIndustries),
            REFL_FIELD(S5::GeneralState, vehiclePreviewRotationFrame),
            REFL_FIELD(S5::GeneralState, objectiveType),
            REFL_FIELD(S5::GeneralState, objectiveFlags),
            REFL_FIELD(S5::GeneralState, objectiveCompanyValue),
            REFL_FIELD(S5::GeneralState, objectiveMonthlyVehicleProfit),
            REFL_FIELD(S5::GeneralState, objectivePerformanceIndex),
            REFL_FIELD(S5::GeneralState, objectiveDeliveredCargoType),
            REFL_FIELD(S5::GeneralState, objectiveDeliveredCargoAmount),
            REFL_FIELD(S5::GeneralState, objectiveTimeLimitYears),
            REFL_FIELD(S5::GeneralState, objectiveTimeLimitUntilYear),
            REFL_FIELD(S5::GeneralState, objectiveMonthsInChallenge),
            REFL_FIELD(S5::GeneralState, objectiveCompletedChallengeInMonths),
            REFL_FIELD(S5::GeneralState, industryFlags),
            REFL_FIELD(S5::GeneralState, forbiddenVehiclesPlayers),
            REFL_FIELD(S5::GeneralState, forbiddenVehiclesCompetitors),
            REFL_FIELD(S5::GeneralState, fixFlags),
            REFL_FIELD(S5::GeneralState, companyRecords),
            REFL_FIELD(S5::GeneralState, var_44C),
            REFL_FIELD(S5::GeneralState, var_450),
            REFL_FIELD(S5::GeneralState, var_454),
            REFL_FIELD(S5::GeneralState, var_458),
            REFL_FIELD(S5::GeneralState, var_45C),
            REFL_FIELD(S5::GeneralState, var_460),
            REFL_FIELD(S5::GeneralState, var_464),
            REFL_FIELD(S5::GeneralState, var_468),
            REFL_FIELD(S5::GeneralState, lastMapWindowFlags),
            REFL_FIELD(S5::GeneralState, lastMapWindowSize),
            REFL_FIELD(S5::GeneralState, lastMapWindowVar88A),
            REFL_FIELD(S5::GeneralState, lastMapWindowVar88C),
            REFL_FIELD(S5::GeneralState, var_478),
            REFL_FIELD(S5::GeneralState, pad_047C),
            REFL_FIELD(S5::GeneralState, numMessages),
            REFL_FIELD(S5::GeneralState, activeMessageIndex),
            REFL_FIELD(S5::GeneralState, messages),
            REFL_FIELD(S5::GeneralState, pad_B95A),
            REFL_FIELD(S5::GeneralState, var_B95C),
            REFL_FIELD(S5::GeneralState, pad_B95D),
            REFL_FIELD(S5::GeneralState, var_B960),
            REFL_FIELD(S5::GeneralState, pad_B961),
            REFL_FIELD(S5::GeneralState, var_B962),
            REFL_FIELD(S5::GeneralState, pad_B963),
            REFL_FIELD(S5::GeneralState, var_B964),
            REFL_FIELD(S5::GeneralState, pad_B965),
            REFL_FIELD(S5::GeneralState, var_B966),
            REFL_FIELD(S5::GeneralState, pad_B967),
            REFL_FIELD(S5::GeneralState, currentRainLevel),
            REFL_FIELD(S5::GeneralState, pad_B969)>;
    };

    template<>
    struct Reflection<S5::GameState>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::GameState, general),
            REFL_FIELD(S5::GameState, companies),
            REFL_FIELD(S5::GameState, towns),
            REFL_FIELD(S5::GameState, industries),
            REFL_FIELD(S5::GameState, stations),
            REFL_FIELD(S5::GameState, entities),
            REFL_FIELD(S5::GameState, animations),
            REFL_FIELD(S5::GameState, waves),
            REFL_FIELD(S5::GameState, userStrings),
            REFL_FIELD(S5::GameState, routings),
            REFL_FIELD(S5::GameState, orders)>;
    };
}
