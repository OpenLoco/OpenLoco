#include "GameSaveCompare.h"

#include <string>

#include "Effects/EffectsManager.h"
#include "GameState.h"
#include "Logging.h"
#include "OpenLoco.h"
#include "S5/Limits.h"
#include "S5/S5.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::GameSaveCompare
{
    std::string getVehicleSubType(const Vehicles::VehicleEntityType vehicleSubType);
    std::string getEffectSubType(const EffectType effectSubType);
    void logVehicleTypeAndSubType(int offset, const OpenLoco::Entity& entity);
    void logEffectType(int offset, const OpenLoco::Entity& entity);
    long logDivergentEntityOffset(const S5::Entity& lhs, const S5::Entity& rhs, int offset, bool displayAllDivergences, long divergentBytesTotal);
    bool compareGameStates(S5::GameState& gameState1, S5::GameState& gameState2, bool displayAllDivergences);
    bool isLoggedDivergenceRoutings(OpenLoco::S5::GameState& gameState1, OpenLoco::S5::GameState& gameState2, bool displayAllDivergences);
    bool compareElements(const std::vector<S5::TileElement>& tileElements1, const std::vector<S5::TileElement>& tileElements2, bool displayAllDivergences);

    template<typename T>
    std::span<const std::byte> getBytesSpan(const T& item)
    {
        return std::span<const std::byte>{ reinterpret_cast<const std::byte*>(std::addressof(item)), sizeof(T) };
    }

    template<typename T>
    auto bitWiseEqual(const T& lhs, const T& rhs)
    {
        std::span<const std::byte> bytesSpanLhs = getBytesSpan(lhs);
        std::span<const std::byte> bytesSpanRhs = getBytesSpan(rhs);

        return std::equal(bytesSpanLhs.begin(), bytesSpanLhs.end(), bytesSpanRhs.begin(), bytesSpanRhs.end());
    }

    template<typename T1, typename T2>
    auto bitWiseEqual(const T1& lhs, const T2& rhs)
    {
        std::span<const std::byte> bytesSpanLhs = getBytesSpan(lhs);
        std::span<const std::byte> bytesSpanRhs = getBytesSpan(rhs);

        return std::equal(bytesSpanLhs.begin(), bytesSpanLhs.end(), bytesSpanRhs.begin(), bytesSpanRhs.end());
    }

    template<typename T1, typename T2>
    long bitWiseLogDivergence(const std::string type, const T1& lhs, const T2& rhs, bool displayAllDivergences, long divergentBytesTotal)
    {
        static_assert(sizeof(T1) == sizeof(T2));

        std::span<const std::byte> bytesSpanLhs = getBytesSpan(lhs);
        std::span<const std::byte> bytesSpanRhs = getBytesSpan(rhs);

        size_t size = bytesSpanLhs.size();

        for (size_t offset = 0; offset < size; offset++)
        {
            if (bytesSpanLhs[offset] != bytesSpanRhs[offset])
            {
                if (divergentBytesTotal == 0)
                {
                    Logging::info("DIVERGENCE");
                    Logging::info("TYPE: {}", type);
                }
                if (displayAllDivergences || divergentBytesTotal == 0)
                {
                    Logging::info("    OFFSET: {}", offset);
                    Logging::info("    LHS: {:#x}", bytesSpanLhs[offset]);
                    Logging::info("    RHS: {:#x}", bytesSpanRhs[offset]);
                }
                divergentBytesTotal++;
            }
        }
        return divergentBytesTotal;
    }

    template<typename T1, typename T2>
    bool isLoggedDivergence(const std::string type, const T1& lhs, const T2& rhs, bool displayAllDivergences)
    {
        long divergentBytesTotal = 0;
        for (unsigned int offset = 0; offset < sizeof(lhs); offset++)
        {
            divergentBytesTotal = bitWiseLogDivergence(type + " [" + std::to_string(offset) + "]", lhs[offset], rhs[offset], displayAllDivergences, divergentBytesTotal);
        }
        if (!displayAllDivergences && divergentBytesTotal > 1)
        {
            Logging::info(" {} other diverging bytes omitted", divergentBytesTotal);
        }
        return divergentBytesTotal > 0;
    }

    template<typename T1, typename T2>
    long logDivergence(const std::string type, const T1& lhs, const T2& rhs, int arraySize, bool displayAllDivergences)
    {
        long divergentBytesTotal = 0;
        for (int offset = 0; offset < arraySize; offset++)
        {
            divergentBytesTotal = bitWiseLogDivergence(type + " [" + std::to_string(offset) + "]", lhs[offset], rhs[offset], displayAllDivergences, divergentBytesTotal);
        }
        if (!displayAllDivergences && divergentBytesTotal > 0)
        {
            Logging::info(" {} other diverging bytes omitted", divergentBytesTotal);
        }
        return divergentBytesTotal;
    }

    template<typename T1, typename T2>
    bool isLoggedDivergence(const std::string type, const T1& lhs, const T2& rhs, int arraySize, bool displayAllDivergences)
    {
        long divergentBytesTotal = 0;
        for (int offset = 0; offset < arraySize; offset++)
        {
            divergentBytesTotal = bitWiseLogDivergence(type + " [" + std::to_string(offset) + "]", lhs[offset], rhs[offset], displayAllDivergences, divergentBytesTotal);
        }
        if (!displayAllDivergences && divergentBytesTotal > 1)
        {
            Logging::info(" {} other diverging bytes omitted", divergentBytesTotal);
        }
        return divergentBytesTotal > 0;
    }

    template<typename T>
    bool isLoggedDivergentGameStateField(const std::string type, int offset, const T& lhs, const T& rhs)
    {
        bool loggedDivergence = false;
        if (!bitWiseEqual(lhs, rhs))
        {
            Logging::info("DIVERGENCE");
            Logging::info("TYPE: {}", type);
            Logging::info("    OFFSET: {}", offset);
            Logging::info("    LHS: {:#x}", lhs);
            Logging::info("    RHS: {:#x}", rhs);
            loggedDivergence = true;
        }
        return loggedDivergence;
    }

    template<typename T>
    bool isLoggedDivergentGameStateFlag(const std::string type, const T& lhs, const T& rhs)
    {
        bool loggedDivergence = false;
        if (lhs != rhs)
        {
            auto flags1 = static_cast<uint32_t>(lhs);
            auto flags2 = static_cast<uint32_t>(rhs);
            Logging::info("DIVERGENCE");
            Logging::info("TYPE: {}", type);
            Logging::info("    OFFSET: {}", 0);
            Logging::info("    LHS: {:#x}", flags1);
            Logging::info("    LHS: {:#x}", flags2);
            loggedDivergence = true;
        }
        return loggedDivergence;
    }

    template<typename T>
    bool isLoggedDivergentGameStateFieldNoHeader(int offset, const T& lhs, const T& rhs)
    {
        bool loggedDivergence = false;
        if (!bitWiseEqual(lhs, rhs))
        {
            Logging::info("    OFFSET: {}", offset);
            Logging::info("    LHS: {:#x}", lhs);
            Logging::info("    RHS: {:#x}", rhs);
            loggedDivergence = true;
        }
        return loggedDivergence;
    }

    std::string getVehicleSubType(const Vehicles::VehicleEntityType vehicleSubType)
    {
        auto vehicleSubTypeName = "";
        switch (vehicleSubType)
        {
            case Vehicles::VehicleEntityType::head:
                vehicleSubTypeName = "VEHICLE_HEAD";
                break;
            case Vehicles::VehicleEntityType::vehicle_1:
                vehicleSubTypeName = "VEHICLE_1";
                break;
            case Vehicles::VehicleEntityType::vehicle_2:
                vehicleSubTypeName = "VEHICLE_2";
                break;
            case Vehicles::VehicleEntityType::bogie:
                vehicleSubTypeName = "BOGIE";
                break;
            case Vehicles::VehicleEntityType::body_start:
                vehicleSubTypeName = "BODY_START";
                break;
            case Vehicles::VehicleEntityType::body_continued:
                vehicleSubTypeName = "BODY_CONTINUED";
                break;
            case Vehicles::VehicleEntityType::tail:
                vehicleSubTypeName = "VEHICLE_TAIL";
                break;
            default:
                Logging::info("Unknown Vehicle sub-type!");
                break;
        }
        return vehicleSubTypeName;
    }

    std::string getEffectSubType(const EffectType effectSubType)
    {
        auto effectSubTypeName = "effect";
        switch (effectSubType)
        {
            case EffectType::exhaust:
                effectSubTypeName = "EXHAUST";
                break;
            case EffectType::redGreenCurrency:
                effectSubTypeName = "RED_GREEN_CURRENCY";
                break;
            case EffectType::windowCurrency:
                effectSubTypeName = "WINDOW_CURRENCY";
                break;
            case EffectType::vehicleCrashParticle:
                effectSubTypeName = "VEHICLE_CRASH_PARTICLE";
                break;
            case EffectType::explosionCloud:
                effectSubTypeName = "EXPLOSION_CLOUD";
                break;
            case EffectType::splash:
                effectSubTypeName = "SPLASH";
                break;
            case EffectType::fireball:
                effectSubTypeName = "FIREBALL";
                break;
            case EffectType::explosionSmoke:
                effectSubTypeName = "EXPLOSION_SMOKE";
                break;
            case EffectType::smoke:
                effectSubTypeName = "SMOKE";
                break;
            default:
                Logging::info("Unknown Effect sub-type!");
                break;
        }
        return effectSubTypeName;
    }

    void logVehicleTypeAndSubType(int offset, const OpenLoco::Entity& entity)
    {
        auto vehicleTypeName = "TYPE: ENTITY [" + std::to_string(offset) + "] VEHICLE";
        char* entityBase = (char*)(&entity);
        Vehicles::VehicleBase* vehicleBase = reinterpret_cast<Vehicles::VehicleBase*>(entityBase);
        auto vechicleSubTypeName = getVehicleSubType(vehicleBase->getSubType());
        Logging::info("{} {}", vehicleTypeName, vechicleSubTypeName);
    }

    void logEffectType(int offset, const OpenLoco::Entity& entity)
    {
        auto effectTypeName = "TYPE: ENTITY [" + std::to_string(offset) + "] EFFECT";
        char* effect = (char*)(&entity);
        EffectEntity* effectEntity = reinterpret_cast<EffectEntity*>(effect);
        auto effectSubTYpeName = getEffectSubType(effectEntity->getSubType());
        Logging::info("{} {}", effectTypeName, effectSubTYpeName);
    }

    long logDivergentEntityOffset(const S5::Entity& lhs, const S5::Entity& rhs, int offset, bool displayAllDivergences, long divergentBytesTotal)
    {
        if (!bitWiseEqual(lhs, rhs))
        {
            if (divergentBytesTotal == 0)
            {
                Logging::info("DIVERGENCE");
            }

            char* lhsEntityChar = (char*)(&lhs);
            OpenLoco::Entity* lhsEntity = reinterpret_cast<OpenLoco::Entity*>(lhsEntityChar);

            char* rhsEntityChar = (char*)(&rhs);
            OpenLoco::Entity* rhsEntity = reinterpret_cast<OpenLoco::Entity*>(rhsEntityChar);

            if (lhsEntity->baseType == EntityBaseType::vehicle)
            {
                if (displayAllDivergences || divergentBytesTotal == 0)
                {
                    logVehicleTypeAndSubType(offset, *lhsEntity);
                }
                divergentBytesTotal += sizeof(lhsEntity->baseType);
            }
            if (lhsEntity->baseType != rhsEntity->baseType)
            {
                if (rhsEntity->baseType == EntityBaseType::vehicle)
                {
                    if (displayAllDivergences || divergentBytesTotal)
                    {
                        logVehicleTypeAndSubType(offset, *rhsEntity);
                    }
                    divergentBytesTotal += sizeof(rhsEntity);
                }
            }
            if (lhsEntity->baseType == EntityBaseType::effect)
            {
                if (displayAllDivergences || divergentBytesTotal == 0)
                {
                    logEffectType(offset, *lhsEntity);
                }
                else
                {
                    divergentBytesTotal += sizeof(lhsEntity->baseType);
                }
            }
            if (lhsEntity->baseType != rhsEntity->baseType)
            {
                if (rhsEntity->baseType == EntityBaseType::effect)
                {
                    if (displayAllDivergences || divergentBytesTotal == 0)
                    {
                        logEffectType(offset, *rhsEntity);
                    }
                    else
                    {
                        divergentBytesTotal += sizeof(rhsEntity);
                    }
                }
            }
            if (lhsEntity->baseType == EntityBaseType::null)
            {
                if (displayAllDivergences || divergentBytesTotal == 0)
                {
                    Logging::info("TYPE: ENTITY [{}] NULL", offset);
                }
                else
                {
                    divergentBytesTotal += sizeof(lhsEntity->baseType);
                }
            }
            else
            {
                divergentBytesTotal += sizeof(lhsEntity->baseType);
            }
            if (lhsEntity->baseType != rhsEntity->baseType)
            {
                if (rhsEntity->baseType == EntityBaseType::null)
                {

                    if (displayAllDivergences || divergentBytesTotal == 0)
                    {
                        Logging::info("TYPE: ENTITY [{}] NULL", offset);
                    }
                    else
                    {
                        divergentBytesTotal += sizeof(lhsEntity->baseType);
                    }
                }
                else
                {
                    divergentBytesTotal += sizeof(rhsEntity);
                }
            }
            divergentBytesTotal = bitWiseLogDivergence("", lhs, rhs, displayAllDivergences, divergentBytesTotal);
        }
        return divergentBytesTotal;
    }

    template<typename T1, typename T2>
    bool logDivergentEntity(const T1& lhs, const T2& rhs, int arraySize, bool displayAllDivergences)
    {
        long divergentBytesTotal = 0;
        for (int offset = 0; offset < arraySize; offset++)
        {
            divergentBytesTotal = logDivergentEntityOffset(lhs[offset], rhs[offset], offset, displayAllDivergences, divergentBytesTotal);
        }
        if (!displayAllDivergences && divergentBytesTotal > 1)
        {
            Logging::info(" {} other diverging bytes omitted", divergentBytesTotal);
        }
        return divergentBytesTotal > 0;
    }

    bool compareGameStates(S5::GameState& gameState1, S5::GameState& gameState2, bool displayAllDivergences)
    {
        if (displayAllDivergences)
            Logging::info("display all divergences!");

        bool foundDivergence = false;

        foundDivergence |= isLoggedDivergence("rng", gameState1.rng, gameState2.rng, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("unkRng", gameState1.unkRng, gameState2.unkRng, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateFlag("flags", gameState1.flags, gameState2.flags);
        foundDivergence |= isLoggedDivergentGameStateField("currentDay", 0, gameState1.currentDay, gameState2.currentDay);
        foundDivergence |= isLoggedDivergentGameStateField("dayCounter", 0, gameState1.dayCounter, gameState2.dayCounter);
        foundDivergence |= isLoggedDivergentGameStateField("currentYear", 0, gameState1.currentYear, gameState2.currentYear);
        foundDivergence |= isLoggedDivergentGameStateField("currentMonth", 0, gameState1.currentMonth, gameState2.currentMonth);
        foundDivergence |= isLoggedDivergentGameStateField("currentDayOfMonth", 0, gameState1.currentDayOfMonth, gameState2.currentDayOfMonth);
        foundDivergence |= isLoggedDivergentGameStateField("savedViewX", 0, gameState1.savedViewX, gameState2.savedViewX);
        foundDivergence |= isLoggedDivergentGameStateField("savedViewY", 0, gameState1.savedViewY, gameState2.savedViewY);
        foundDivergence |= isLoggedDivergentGameStateField("savedViewZoom", 0, gameState1.savedViewZoom, gameState2.savedViewZoom);
        foundDivergence |= isLoggedDivergentGameStateField("savedViewRotation", 0, gameState1.savedViewRotation, gameState2.savedViewRotation);
        foundDivergence |= isLoggedDivergence("playerCompanies", gameState1.playerCompanies, gameState2.playerCompanies, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("entityListHeads", gameState1.entityListHeads, gameState2.entityListHeads, Limits::kNumEntityLists, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("entityListCounts", gameState1.entityListCounts, gameState2.entityListCounts, Limits::kNumEntityLists, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("pad_0042", gameState1.pad_0042, gameState2.pad_0042, 0x046 - 0x042, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("currencyMultiplicationFactor", gameState1.currencyMultiplicationFactor, gameState2.currencyMultiplicationFactor, 32, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("unusedCurrencyMultiplicationFactor[", gameState1.unusedCurrencyMultiplicationFactor, gameState2.unusedCurrencyMultiplicationFactor, 32, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("scenarioTicks", 0, gameState1.scenarioTicks, gameState2.scenarioTicks);
        foundDivergence |= isLoggedDivergentGameStateField("var_014A", 0, gameState1.var_014A, gameState2.var_014A);
        foundDivergence |= isLoggedDivergentGameStateField("scenarioTicks2", 0, gameState1.scenarioTicks2, gameState2.scenarioTicks2);
        foundDivergence |= isLoggedDivergentGameStateField("magicNumber", 0, gameState1.magicNumber, gameState2.magicNumber);
        foundDivergence |= isLoggedDivergentGameStateField("numMapAnimations", 0, gameState1.numMapAnimations, gameState2.numMapAnimations);
        foundDivergence |= isLoggedDivergence("tileUpdateStartLocation", gameState1.tileUpdateStartLocation, gameState2.tileUpdateStartLocation, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioSignals", gameState1.scenarioSignals, gameState2.scenarioSignals, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioBridges", gameState1.scenarioBridges, gameState2.scenarioBridges, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioTrainStations", gameState1.scenarioTrainStations, gameState2.scenarioTrainStations, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioTrackMods", gameState1.scenarioTrackMods, gameState2.scenarioTrackMods, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("var_17A", gameState1.var_17A, gameState2.var_17A, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioRoadStations", gameState1.scenarioRoadStations, gameState2.scenarioRoadStations, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioRoadMods", gameState1.scenarioRoadMods, gameState2.scenarioRoadMods, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("lastRailroadOption", 0, gameState1.lastRailroadOption, gameState2.lastRailroadOption);
        foundDivergence |= isLoggedDivergentGameStateField("lastRoadOption", 0, gameState1.lastRoadOption, gameState2.lastRoadOption);
        foundDivergence |= isLoggedDivergentGameStateField("lastAirport", 0, gameState1.lastAirport, gameState2.lastAirport);
        foundDivergence |= isLoggedDivergentGameStateField("lastShipPort", 0, gameState1.lastShipPort, gameState2.lastShipPort);
        foundDivergence |= isLoggedDivergentGameStateField("trafficHandedness", 0, gameState1.trafficHandedness, gameState2.trafficHandedness);
        foundDivergence |= isLoggedDivergentGameStateField("lastVehicleType", 0, gameState1.lastVehicleType, gameState2.lastVehicleType);
        foundDivergence |= isLoggedDivergentGameStateField("pickupDirection", 0, gameState1.pickupDirection, gameState2.pickupDirection);
        foundDivergence |= isLoggedDivergentGameStateField("lastTreeOption", 0, gameState1.lastTreeOption, gameState2.lastTreeOption);
        foundDivergence |= isLoggedDivergentGameStateField("seaLevel", 0, gameState1.seaLevel, gameState2.seaLevel);
        foundDivergence |= isLoggedDivergentGameStateField("currentSnowLine", 0, gameState1.currentSnowLine, gameState2.currentSnowLine);
        foundDivergence |= isLoggedDivergentGameStateField("currentSeason", 0, gameState1.currentSeason, gameState2.currentSeason);
        foundDivergence |= isLoggedDivergentGameStateField("lastLandOption", 0, gameState1.lastLandOption, gameState2.lastLandOption);
        foundDivergence |= isLoggedDivergentGameStateField("maxCompetingCompanies", 0, gameState1.maxCompetingCompanies, gameState2.maxCompetingCompanies);
        foundDivergence |= isLoggedDivergentGameStateField("orderTableLength", 0, gameState1.orderTableLength, gameState2.orderTableLength);
        foundDivergence |= isLoggedDivergentGameStateField("roadObjectIdIsTram", 0, gameState1.roadObjectIdIsTram, gameState2.roadObjectIdIsTram);
        foundDivergence |= isLoggedDivergentGameStateField("roadObjectIdIsFlag7", 0, gameState1.roadObjectIdIsFlag7, gameState2.roadObjectIdIsFlag7);
        foundDivergence |= isLoggedDivergentGameStateField("var_1AC", 0, gameState1.var_1AC, gameState2.var_1AC);
        foundDivergence |= isLoggedDivergentGameStateField("lastTrackTypeOption", 0, gameState1.lastTrackTypeOption, gameState2.lastTrackTypeOption);
        foundDivergence |= isLoggedDivergentGameStateField("loanInterestRate", 0, gameState1.loanInterestRate, gameState2.loanInterestRate);
        foundDivergence |= isLoggedDivergentGameStateField("lastIndustryOption", 0, gameState1.lastIndustryOption, gameState2.lastIndustryOption);
        foundDivergence |= isLoggedDivergentGameStateField("lastBuildingOption", 0, gameState1.lastBuildingOption, gameState2.lastBuildingOption);
        foundDivergence |= isLoggedDivergentGameStateField("lastMiscBuildingOption", 0, gameState1.lastMiscBuildingOption, gameState2.lastMiscBuildingOption);
        foundDivergence |= isLoggedDivergentGameStateField("lastWallOption", 0, gameState1.lastWallOption, gameState2.lastWallOption);
        foundDivergence |= isLoggedDivergentGameStateField("produceAICompanyTimeout", 0, gameState1.produceAICompanyTimeout, gameState2.produceAICompanyTimeout);
        foundDivergence |= isLoggedDivergence("tickStartPrngState", gameState1.tickStartPrngState, gameState2.tickStartPrngState, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioFileName", gameState1.scenarioFileName, gameState2.scenarioFileName, 256, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioName", gameState1.scenarioName, gameState2.scenarioName, 64, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioDetails", gameState1.scenarioDetails, gameState2.scenarioDetails, 256, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("competitorStartDelay", 0, gameState1.competitorStartDelay, gameState2.competitorStartDelay);
        foundDivergence |= isLoggedDivergentGameStateField("preferredAIIntelligence", 0, gameState1.preferredAIIntelligence, gameState2.preferredAIIntelligence);
        foundDivergence |= isLoggedDivergentGameStateField("preferredAIAggressiveness", 0, gameState1.preferredAIAggressiveness, gameState2.preferredAIAggressiveness);
        foundDivergence |= isLoggedDivergentGameStateField("preferredAICompetitiveness", 0, gameState1.preferredAICompetitiveness, gameState2.preferredAICompetitiveness);
        foundDivergence |= isLoggedDivergentGameStateField("startingLoanSize", 0, gameState1.startingLoanSize, gameState2.startingLoanSize);
        foundDivergence |= isLoggedDivergentGameStateField("maxLoanSize", 0, gameState1.maxLoanSize, gameState2.maxLoanSize);
        foundDivergence |= isLoggedDivergentGameStateField("var_404", 0, gameState1.var_404, gameState2.var_404);
        foundDivergence |= isLoggedDivergentGameStateField("var_408", 0, gameState1.var_408, gameState2.var_408);
        foundDivergence |= isLoggedDivergentGameStateField("var_40C", 0, gameState1.var_40C, gameState2.var_40C);
        foundDivergence |= isLoggedDivergentGameStateField("var_410", 0, gameState1.var_410, gameState2.var_410);
        foundDivergence |= isLoggedDivergentGameStateField("lastBuildVehiclesOption", 0, gameState1.lastBuildVehiclesOption, gameState2.lastBuildVehiclesOption);
        foundDivergence |= isLoggedDivergentGameStateField("numberOfIndustries", 0, gameState1.numberOfIndustries, gameState2.numberOfIndustries);
        foundDivergence |= isLoggedDivergentGameStateField("vehiclePreviewRotationFrame", 0, gameState1.vehiclePreviewRotationFrame, gameState2.vehiclePreviewRotationFrame);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveType", 0, gameState1.objectiveType, gameState2.objectiveType);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveFlags", 0, gameState1.objectiveFlags, gameState2.objectiveFlags);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveCompanyValue", 0, gameState1.objectiveCompanyValue, gameState2.objectiveCompanyValue);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveMonthlyVehicleProfit", 0, gameState1.objectiveMonthlyVehicleProfit, gameState2.objectiveMonthlyVehicleProfit);
        foundDivergence |= isLoggedDivergentGameStateField("objectivePerformanceIndex", 0, gameState1.objectivePerformanceIndex, gameState2.objectivePerformanceIndex);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveDeliveredCargoType", 0, gameState1.objectiveDeliveredCargoType, gameState2.objectiveDeliveredCargoType);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveDeliveredCargoAmount", 0, gameState1.objectiveDeliveredCargoAmount, gameState2.objectiveDeliveredCargoAmount);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveTimeLimitUntilYear", 0, gameState1.objectiveTimeLimitUntilYear, gameState2.objectiveTimeLimitUntilYear);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveMonthsInChallenge", 0, gameState1.objectiveMonthsInChallenge, gameState2.objectiveMonthsInChallenge);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveCompletedChallengeInMonths", 0, gameState1.objectiveCompletedChallengeInMonths, gameState2.objectiveCompletedChallengeInMonths);
        foundDivergence |= isLoggedDivergentGameStateField("industryFlags", 0, gameState1.industryFlags, gameState2.industryFlags);
        foundDivergence |= isLoggedDivergentGameStateField("forbiddenVehiclesPlayers", 0, gameState1.forbiddenVehiclesPlayers, gameState2.forbiddenVehiclesPlayers);
        foundDivergence |= isLoggedDivergentGameStateField("forbiddenVehiclesCompetitors", 0, gameState1.forbiddenVehiclesCompetitors, gameState2.forbiddenVehiclesCompetitors);
        foundDivergence |= isLoggedDivergentGameStateFlag("fixFlags", gameState1.fixFlags, gameState2.fixFlags);
        foundDivergence |= isLoggedDivergence("recordSpeed", gameState1.recordSpeed, gameState2.recordSpeed, 3, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("recordCompany", gameState1.recordCompany, gameState2.recordCompany, 4, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("recordDate", gameState1.recordDate, gameState2.recordDate, 3, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("var_44C", 0, gameState1.var_44C, gameState2.var_44C);
        foundDivergence |= isLoggedDivergentGameStateField("var_450", 0, gameState1.var_450, gameState2.var_450);
        foundDivergence |= isLoggedDivergentGameStateField("var_454", 0, gameState1.var_454, gameState2.var_454);
        foundDivergence |= isLoggedDivergentGameStateField("var_458", 0, gameState1.var_458, gameState2.var_458);
        foundDivergence |= isLoggedDivergentGameStateField("var_460", 0, gameState1.var_460, gameState2.var_460);
        foundDivergence |= isLoggedDivergentGameStateField("var_464", 0, gameState1.var_464, gameState2.var_464);
        foundDivergence |= isLoggedDivergentGameStateField("var_468", 0, gameState1.var_468, gameState2.var_468);
        foundDivergence |= isLoggedDivergentGameStateField("lastMapWindowFlags", 0, gameState1.lastMapWindowFlags, gameState2.lastMapWindowFlags);
        foundDivergence |= isLoggedDivergence("lastMapWindowSize", gameState1.lastMapWindowSize, gameState2.lastMapWindowSize, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("lastMapWindowVar88A", 0, gameState1.lastMapWindowVar88A, gameState2.lastMapWindowVar88A);
        foundDivergence |= isLoggedDivergentGameStateField("lastMapWindowVar88C", 0, gameState1.lastMapWindowVar88C, gameState2.lastMapWindowVar88C);
        foundDivergence |= isLoggedDivergentGameStateField("var_478", 0, gameState1.var_478, gameState2.var_478);
        foundDivergence |= isLoggedDivergence("pad_047C", gameState1.pad_047C, gameState2.pad_047C, 0x13B6 - 0x47C, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("numMessages", 0, gameState1.numMessages, gameState2.numMessages);
        foundDivergence |= isLoggedDivergentGameStateField("activeMessageIndex", 0, gameState1.activeMessageIndex, gameState2.activeMessageIndex);
        foundDivergence |= isLoggedDivergence("messages", gameState1.messages, gameState2.messages, Limits::kMaxMessages, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("pad_B886", gameState1.pad_B886, gameState2.pad_B886, 0xB94C - 0xB886, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("var_B94C", 0, gameState1.var_B94C, gameState2.var_B94C);
        foundDivergence |= isLoggedDivergence("pad_B94D", gameState1.pad_B94D, gameState2.pad_B94D, 0xB950 - 0xB94D, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("var_B950", 0, gameState1.var_B950, gameState2.var_B950);
        foundDivergence |= isLoggedDivergentGameStateField("pad_B951", 0, gameState1.pad_B951, gameState2.pad_B951);
        foundDivergence |= isLoggedDivergentGameStateField("var_B952", 0, gameState1.var_B952, gameState2.var_B952);
        foundDivergence |= isLoggedDivergentGameStateField("pad_B953", 0, gameState1.pad_B953, gameState2.pad_B953);
        foundDivergence |= isLoggedDivergentGameStateField("var_B954", 0, gameState1.var_B954, gameState2.var_B954);
        foundDivergence |= isLoggedDivergentGameStateField("pad_B955", 0, gameState1.pad_B955, gameState2.pad_B955);
        foundDivergence |= isLoggedDivergentGameStateField("var_B956", 0, gameState1.var_B956, gameState2.var_B956);
        foundDivergence |= isLoggedDivergence("pad_B957", gameState1.pad_B957, gameState2.pad_B957, 0xB968 - 0xB957, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("currentRainLevel", 0, gameState1.currentRainLevel, gameState2.currentRainLevel);
        foundDivergence |= isLoggedDivergence("pad_B969", gameState1.pad_B969, gameState2.pad_B969, 0xB96C - 0xB969, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("companies", gameState1.companies, gameState2.companies, S5::Limits::kMaxCompanies, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("towns", gameState1.towns, gameState2.towns, S5::Limits::kMaxTowns, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("industries", gameState1.industries, gameState2.industries, S5::Limits::kMaxIndustries, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("stations", gameState1.stations, gameState2.stations, S5::Limits::kMaxStations, displayAllDivergences);
        foundDivergence |= logDivergentEntity(gameState1.entities, gameState2.entities, S5::Limits::kMaxEntities, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("animations", gameState1.animations, gameState2.animations, S5::Limits::kMaxAnimations, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("waves", gameState1.waves, gameState2.waves, S5::Limits::kMaxWaves, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("userStrings ", gameState1.userStrings, gameState2.userStrings, S5::Limits::kMaxUserStrings, displayAllDivergences);
        foundDivergence |= isLoggedDivergenceRoutings(gameState1, gameState2, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("orders", gameState1.orders, gameState2.orders, S5::Limits::kMaxOrders, displayAllDivergences);

        return not foundDivergence;
    }

    bool isLoggedDivergenceRoutings(OpenLoco::S5::GameState& gameState1, OpenLoco::S5::GameState& gameState2, bool displayAllDivergences)
    {
        long divergentBytesTotal = 0;
        for (unsigned int route = 0; route < S5::Limits::kMaxVehicles; route++)
        {
            for (unsigned int routePerVehicle = 0; routePerVehicle < S5::Limits::kMaxRoutingsPerVehicle; routePerVehicle++)
            {
                if (!bitWiseEqual(gameState1.routings[route][routePerVehicle], gameState2.routings[route][routePerVehicle]))
                {
                    if (divergentBytesTotal == 0)
                    {
                        Logging::info("DIVERGENCE");
                        Logging::info("TYPE: routings [{}][{}]", route, routePerVehicle);
                    }
                    divergentBytesTotal += sizeof(gameState1.routings[route][routePerVehicle]);
                }

                if (displayAllDivergences || divergentBytesTotal == 0)
                {
                    isLoggedDivergentGameStateFieldNoHeader(
                        route * Limits::kMaxOrdersPerVehicle + routePerVehicle,
                        gameState1.routings[route][routePerVehicle],
                        gameState2.routings[route][routePerVehicle]);
                }
            }
        }
        if (!displayAllDivergences && divergentBytesTotal > 0)
        {
            Logging::info(" {} other diverging bytes omitted", divergentBytesTotal);
        }
        return divergentBytesTotal > 0;
    }

    bool compareElements(const std::vector<S5::TileElement>& tileElements1, const std::vector<S5::TileElement>& tileElements2, bool displayAllDivergences)
    {
        long divergentBytesTotal = 0;
        std::vector<S5::TileElement> smaller = tileElements1;
        std::vector<S5::TileElement> larger = tileElements2;
        if (tileElements1.size() > tileElements2.size())
        {
            smaller = tileElements2;
            larger = tileElements1;
        }

        if (tileElements1.size() != tileElements2.size())
        {
            Logging::info("The TileElements sizes are different. Will compare up to the smallest TileElements size.");
            Logging::info("Size of TileElements1 = {}", tileElements1.size());
            Logging::info("Size of TileElements2 = {}", tileElements2.size());
        }
        int elementCount = 0;
        for (auto iterator1 = smaller.begin(), iterator2 = larger.begin(); iterator1 != smaller.end();
             ++iterator1, ++iterator2)
        {
            if (iterator2 != larger.end())
            {
                S5::TileElement tile1 = tileElements1.at(elementCount);
                S5::TileElement tile2 = tileElements2.at(elementCount);

                if (!bitWiseEqual(tile1, tile2))
                {
                    if (divergentBytesTotal == 0)
                    {
                        Logging::info("DIVERGENCE");
                        Logging::info("TILE ELEMENT[{}]", elementCount);
                    }
                    divergentBytesTotal = bitWiseLogDivergence("Elements[" + std::to_string(elementCount) + "]", tile1, tile2, displayAllDivergences, divergentBytesTotal);
                }
                elementCount++;
            }
        }
        if (!displayAllDivergences && divergentBytesTotal > 0)
        {
            Logging::info(" {} other diverging bytes omitted", divergentBytesTotal);
        }
        return not(divergentBytesTotal > 0);
    }

    bool compareGameStates(const fs::path& path)
    {
        char* gameStateChar = (char*)(&getGameState());
        S5::GameState* currentS5GameState = reinterpret_cast<S5::GameState*>(gameStateChar);
        Logging::info("Comparing reference file {} to current GameState frame", path);
        return compareGameStates(*currentS5GameState, S5::importSave(path).get()->gameState, false);
    }

    bool compareGameStates(const fs::path& path1, const fs::path& path2, bool displayAllDivergences)
    {
        Logging::info("Comparing game state files:");
        Logging::info("   file1: {}", path1);
        Logging::info("   file2: {}", path2);

        auto state1 = S5::importSave(path1);
        auto state2 = S5::importSave(path2);
        auto match = compareGameStates(state1->gameState, state2->gameState, displayAllDivergences);
        match &= compareElements(state1->tileElements, state2->tileElements, displayAllDivergences);
        return match;
    }
}
