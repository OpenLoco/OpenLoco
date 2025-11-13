#include "GameSaveCompare.h"

#include <string>

#include "Effects/Effect.h"
#include "GameState.h"
#include "Logging.h"
#include "OpenLoco.h"
#include "S5/Limits.h"
#include "S5/S5.h"
#include "S5/S5File.h"
#include "S5/S5Options.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Core/FileStream.h>

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

    static bool isLoggedDivergenceUserString(const std::string type, const std::span<char[32]> lhsArr, const std::span<char[32]> rhsArr, bool displayAllDivergences)
    {
        long divergentBytesTotal = 0;
        for (auto offset = 0U; offset < lhsArr.size(); offset++)
        {
            bool isSame = strcmp(lhsArr[offset], rhsArr[offset]) == 0;
            if (!isSame)
            {
                divergentBytesTotal++;
                if (displayAllDivergences || divergentBytesTotal == 1)
                {
                    Logging::info("DIVERGENCE");
                    Logging::info("TYPE: {}", type);
                    Logging::info("    OFFSET: {}", offset);
                    Logging::info("    LHS: {}", lhsArr[offset]);
                    Logging::info("    RHS: {}", rhsArr[offset]);
                }
            }
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
        {
            Logging::info("display all divergences!");
        }

        bool foundDivergence = false;

        foundDivergence |= isLoggedDivergence("rng", gameState1.general.rng, gameState2.general.rng, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("unkRng", gameState1.general.unkRng, gameState2.general.unkRng, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateFlag("flags", gameState1.general.flags, gameState2.general.flags);
        foundDivergence |= isLoggedDivergentGameStateField("currentDay", 0, gameState1.general.currentDay, gameState2.general.currentDay);
        foundDivergence |= isLoggedDivergentGameStateField("dayCounter", 0, gameState1.general.dayCounter, gameState2.general.dayCounter);
        foundDivergence |= isLoggedDivergentGameStateField("currentYear", 0, gameState1.general.currentYear, gameState2.general.currentYear);
        foundDivergence |= isLoggedDivergentGameStateField("currentMonth", 0, gameState1.general.currentMonth, gameState2.general.currentMonth);
        foundDivergence |= isLoggedDivergentGameStateField("currentDayOfMonth", 0, gameState1.general.currentDayOfMonth, gameState2.general.currentDayOfMonth);
        foundDivergence |= isLoggedDivergentGameStateField("savedViewX", 0, gameState1.general.savedViewX, gameState2.general.savedViewX);
        foundDivergence |= isLoggedDivergentGameStateField("savedViewY", 0, gameState1.general.savedViewY, gameState2.general.savedViewY);
        foundDivergence |= isLoggedDivergentGameStateField("savedViewZoom", 0, gameState1.general.savedViewZoom, gameState2.general.savedViewZoom);
        foundDivergence |= isLoggedDivergentGameStateField("savedViewRotation", 0, gameState1.general.savedViewRotation, gameState2.general.savedViewRotation);
        foundDivergence |= isLoggedDivergence("playerCompanies", gameState1.general.playerCompanies, gameState2.general.playerCompanies, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("entityListHeads", gameState1.general.entityListHeads, gameState2.general.entityListHeads, Limits::kNumEntityLists, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("entityListCounts", gameState1.general.entityListCounts, gameState2.general.entityListCounts, Limits::kNumEntityLists, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("pad_0042", gameState1.general.pad_0042, gameState2.general.pad_0042, 0x046 - 0x042, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("currencyMultiplicationFactor", gameState1.general.currencyMultiplicationFactor, gameState2.general.currencyMultiplicationFactor, 32, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("unusedCurrencyMultiplicationFactor[", gameState1.general.unusedCurrencyMultiplicationFactor, gameState2.general.unusedCurrencyMultiplicationFactor, 32, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("scenarioTicks", 0, gameState1.general.scenarioTicks, gameState2.general.scenarioTicks);
        foundDivergence |= isLoggedDivergentGameStateField("var_014A", 0, gameState1.general.var_014A, gameState2.general.var_014A);
        foundDivergence |= isLoggedDivergentGameStateField("scenarioTicks2", 0, gameState1.general.scenarioTicks2, gameState2.general.scenarioTicks2);
        foundDivergence |= isLoggedDivergentGameStateField("magicNumber", 0, gameState1.general.magicNumber, gameState2.general.magicNumber);
        foundDivergence |= isLoggedDivergentGameStateField("numMapAnimations", 0, gameState1.general.numMapAnimations, gameState2.general.numMapAnimations);
        foundDivergence |= isLoggedDivergence("tileUpdateStartLocation", reinterpret_cast<uint16_t(&)[2]>(gameState1.general.tileUpdateStartLocation), reinterpret_cast<uint16_t(&)[2]>(gameState2.general.tileUpdateStartLocation), 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioConstruction.signals", gameState1.general.scenarioConstruction.signals, gameState2.general.scenarioConstruction.signals, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioConstruction.bridges", gameState1.general.scenarioConstruction.bridges, gameState2.general.scenarioConstruction.bridges, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioConstruction.trainStations", gameState1.general.scenarioConstruction.trainStations, gameState2.general.scenarioConstruction.trainStations, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioConstruction.trackMods", gameState1.general.scenarioConstruction.trackMods, gameState2.general.scenarioConstruction.trackMods, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioConstruction.var_17A", gameState1.general.scenarioConstruction.var_17A, gameState2.general.scenarioConstruction.var_17A, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioConstruction.roadStations", gameState1.general.scenarioConstruction.roadStations, gameState2.general.scenarioConstruction.roadStations, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioConstruction.roadMods", gameState1.general.scenarioConstruction.roadMods, gameState2.general.scenarioConstruction.roadMods, 8, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("lastRailroadOption", 0, gameState1.general.lastRailroadOption, gameState2.general.lastRailroadOption);
        foundDivergence |= isLoggedDivergentGameStateField("lastRoadOption", 0, gameState1.general.lastRoadOption, gameState2.general.lastRoadOption);
        foundDivergence |= isLoggedDivergentGameStateField("lastAirport", 0, gameState1.general.lastAirport, gameState2.general.lastAirport);
        foundDivergence |= isLoggedDivergentGameStateField("lastShipPort", 0, gameState1.general.lastShipPort, gameState2.general.lastShipPort);
        foundDivergence |= isLoggedDivergentGameStateField("trafficHandedness", 0, gameState1.general.trafficHandedness, gameState2.general.trafficHandedness);
        foundDivergence |= isLoggedDivergentGameStateField("lastVehicleType", 0, gameState1.general.lastVehicleType, gameState2.general.lastVehicleType);
        foundDivergence |= isLoggedDivergentGameStateField("pickupDirection", 0, gameState1.general.pickupDirection, gameState2.general.pickupDirection);
        foundDivergence |= isLoggedDivergentGameStateField("lastTreeOption", 0, gameState1.general.lastTreeOption, gameState2.general.lastTreeOption);
        foundDivergence |= isLoggedDivergentGameStateField("seaLevel", 0, gameState1.general.seaLevel, gameState2.general.seaLevel);
        foundDivergence |= isLoggedDivergentGameStateField("currentSnowLine", 0, gameState1.general.currentSnowLine, gameState2.general.currentSnowLine);
        foundDivergence |= isLoggedDivergentGameStateField("currentSeason", 0, gameState1.general.currentSeason, gameState2.general.currentSeason);
        foundDivergence |= isLoggedDivergentGameStateField("lastLandOption", 0, gameState1.general.lastLandOption, gameState2.general.lastLandOption);
        foundDivergence |= isLoggedDivergentGameStateField("maxCompetingCompanies", 0, gameState1.general.maxCompetingCompanies, gameState2.general.maxCompetingCompanies);
        foundDivergence |= isLoggedDivergentGameStateField("orderTableLength", 0, gameState1.general.orderTableLength, gameState2.general.orderTableLength);
        foundDivergence |= isLoggedDivergentGameStateField("roadObjectIdIsNotTram", 0, gameState1.general.roadObjectIdIsNotTram, gameState2.general.roadObjectIdIsNotTram);
        foundDivergence |= isLoggedDivergentGameStateField("roadObjectIdIsFlag7", 0, gameState1.general.roadObjectIdIsFlag7, gameState2.general.roadObjectIdIsFlag7);
        foundDivergence |= isLoggedDivergentGameStateField("currentDefaultLevelCrossingType", 0, gameState1.general.currentDefaultLevelCrossingType, gameState2.general.currentDefaultLevelCrossingType);
        foundDivergence |= isLoggedDivergentGameStateField("lastTrackTypeOption", 0, gameState1.general.lastTrackTypeOption, gameState2.general.lastTrackTypeOption);
        foundDivergence |= isLoggedDivergentGameStateField("loanInterestRate", 0, gameState1.general.loanInterestRate, gameState2.general.loanInterestRate);
        foundDivergence |= isLoggedDivergentGameStateField("lastIndustryOption", 0, gameState1.general.lastIndustryOption, gameState2.general.lastIndustryOption);
        foundDivergence |= isLoggedDivergentGameStateField("lastBuildingOption", 0, gameState1.general.lastBuildingOption, gameState2.general.lastBuildingOption);
        foundDivergence |= isLoggedDivergentGameStateField("lastMiscBuildingOption", 0, gameState1.general.lastMiscBuildingOption, gameState2.general.lastMiscBuildingOption);
        foundDivergence |= isLoggedDivergentGameStateField("lastWallOption", 0, gameState1.general.lastWallOption, gameState2.general.lastWallOption);
        foundDivergence |= isLoggedDivergentGameStateField("produceAICompanyTimeout", 0, gameState1.general.produceAICompanyTimeout, gameState2.general.produceAICompanyTimeout);
        foundDivergence |= isLoggedDivergence("tickStartPrngState", gameState1.general.tickStartPrngState, gameState2.general.tickStartPrngState, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioFileName", gameState1.general.scenarioFileName, gameState2.general.scenarioFileName, 256, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioName", gameState1.general.scenarioName, gameState2.general.scenarioName, 64, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("scenarioDetails", gameState1.general.scenarioDetails, gameState2.general.scenarioDetails, 256, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("competitorStartDelay", 0, gameState1.general.competitorStartDelay, gameState2.general.competitorStartDelay);
        foundDivergence |= isLoggedDivergentGameStateField("preferredAIIntelligence", 0, gameState1.general.preferredAIIntelligence, gameState2.general.preferredAIIntelligence);
        foundDivergence |= isLoggedDivergentGameStateField("preferredAIAggressiveness", 0, gameState1.general.preferredAIAggressiveness, gameState2.general.preferredAIAggressiveness);
        foundDivergence |= isLoggedDivergentGameStateField("preferredAICompetitiveness", 0, gameState1.general.preferredAICompetitiveness, gameState2.general.preferredAICompetitiveness);
        foundDivergence |= isLoggedDivergentGameStateField("startingLoanSize", 0, gameState1.general.startingLoanSize, gameState2.general.startingLoanSize);
        foundDivergence |= isLoggedDivergentGameStateField("maxLoanSize", 0, gameState1.general.maxLoanSize, gameState2.general.maxLoanSize);
        foundDivergence |= isLoggedDivergence("multiplayerPrng", gameState1.general.multiplayerPrng, gameState2.general.multiplayerPrng, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("var_40C", 0, gameState1.general.multiplayerChecksumA, gameState2.general.multiplayerChecksumA);
        foundDivergence |= isLoggedDivergentGameStateField("var_410", 0, gameState1.general.multiplayerChecksumB, gameState2.general.multiplayerChecksumB);
        foundDivergence |= isLoggedDivergentGameStateField("lastBuildVehiclesOption", 0, gameState1.general.lastBuildVehiclesOption, gameState2.general.lastBuildVehiclesOption);
        foundDivergence |= isLoggedDivergentGameStateField("numberOfIndustries", 0, gameState1.general.numberOfIndustries, gameState2.general.numberOfIndustries);
        foundDivergence |= isLoggedDivergentGameStateField("vehiclePreviewRotationFrame", 0, gameState1.general.vehiclePreviewRotationFrame, gameState2.general.vehiclePreviewRotationFrame);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveType", 0, gameState1.general.objectiveType, gameState2.general.objectiveType);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveFlags", 0, gameState1.general.objectiveFlags, gameState2.general.objectiveFlags);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveCompanyValue", 0, gameState1.general.objectiveCompanyValue, gameState2.general.objectiveCompanyValue);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveMonthlyVehicleProfit", 0, gameState1.general.objectiveMonthlyVehicleProfit, gameState2.general.objectiveMonthlyVehicleProfit);
        foundDivergence |= isLoggedDivergentGameStateField("objectivePerformanceIndex", 0, gameState1.general.objectivePerformanceIndex, gameState2.general.objectivePerformanceIndex);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveDeliveredCargoType", 0, gameState1.general.objectiveDeliveredCargoType, gameState2.general.objectiveDeliveredCargoType);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveDeliveredCargoAmount", 0, gameState1.general.objectiveDeliveredCargoAmount, gameState2.general.objectiveDeliveredCargoAmount);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveTimeLimitUntilYear", 0, gameState1.general.objectiveTimeLimitUntilYear, gameState2.general.objectiveTimeLimitUntilYear);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveMonthsInChallenge", 0, gameState1.general.objectiveMonthsInChallenge, gameState2.general.objectiveMonthsInChallenge);
        foundDivergence |= isLoggedDivergentGameStateField("objectiveCompletedChallengeInMonths", 0, gameState1.general.objectiveCompletedChallengeInMonths, gameState2.general.objectiveCompletedChallengeInMonths);
        foundDivergence |= isLoggedDivergentGameStateField("industryFlags", 0, gameState1.general.industryFlags, gameState2.general.industryFlags);
        foundDivergence |= isLoggedDivergentGameStateField("forbiddenVehiclesPlayers", 0, gameState1.general.forbiddenVehiclesPlayers, gameState2.general.forbiddenVehiclesPlayers);
        foundDivergence |= isLoggedDivergentGameStateField("forbiddenVehiclesCompetitors", 0, gameState1.general.forbiddenVehiclesCompetitors, gameState2.general.forbiddenVehiclesCompetitors);
        foundDivergence |= isLoggedDivergentGameStateFlag("fixFlags", gameState1.general.fixFlags, gameState2.general.fixFlags);
        foundDivergence |= isLoggedDivergence("recordSpeed", gameState1.general.companyRecords.speed, gameState2.general.companyRecords.speed, 3, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("recordCompany", gameState1.general.companyRecords.company, gameState2.general.companyRecords.company, 4, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("recordDate", gameState1.general.companyRecords.date, gameState2.general.companyRecords.date, 3, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("var_44C", 0, gameState1.general.var_44C, gameState2.general.var_44C);
        foundDivergence |= isLoggedDivergentGameStateField("var_450", 0, gameState1.general.var_450, gameState2.general.var_450);
        foundDivergence |= isLoggedDivergentGameStateField("var_454", 0, gameState1.general.var_454, gameState2.general.var_454);
        foundDivergence |= isLoggedDivergentGameStateField("var_458", 0, gameState1.general.var_458, gameState2.general.var_458);
        foundDivergence |= isLoggedDivergentGameStateField("var_460", 0, gameState1.general.var_460, gameState2.general.var_460);
        foundDivergence |= isLoggedDivergentGameStateField("var_464", 0, gameState1.general.var_464, gameState2.general.var_464);
        foundDivergence |= isLoggedDivergentGameStateField("var_468", 0, gameState1.general.var_468, gameState2.general.var_468);
        foundDivergence |= isLoggedDivergentGameStateField("lastMapWindowFlags", 0, gameState1.general.lastMapWindowFlags, gameState2.general.lastMapWindowFlags);
        foundDivergence |= isLoggedDivergence("lastMapWindowSize", gameState1.general.lastMapWindowSize, gameState2.general.lastMapWindowSize, 2, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("lastMapWindowVar88A", 0, gameState1.general.lastMapWindowVar88A, gameState2.general.lastMapWindowVar88A);
        foundDivergence |= isLoggedDivergentGameStateField("lastMapWindowVar88C", 0, gameState1.general.lastMapWindowVar88C, gameState2.general.lastMapWindowVar88C);
        foundDivergence |= isLoggedDivergentGameStateField("var_478", 0, gameState1.general.var_478, gameState2.general.var_478);
        foundDivergence |= isLoggedDivergence("pad_047C", gameState1.general.pad_047C, gameState2.general.pad_047C, 0x13B6 - 0x47C, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("numMessages", 0, gameState1.general.numMessages, gameState2.general.numMessages);
        foundDivergence |= isLoggedDivergentGameStateField("activeMessageIndex", 0, gameState1.general.activeMessageIndex, gameState2.general.activeMessageIndex);
        foundDivergence |= isLoggedDivergence("messages", gameState1.general.messages, gameState2.general.messages, Limits::kMaxMessages, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("pad_B886", gameState1.general.pad_B95A, gameState2.general.pad_B95A, 0xB95C - 0xB95A, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("var_B95C", 0, gameState1.general.var_B95C, gameState2.general.var_B95C);
        foundDivergence |= isLoggedDivergence("pad_B94D", gameState1.general.pad_B95D, gameState2.general.pad_B95D, 0xB960 - 0xB95D, displayAllDivergences);
        foundDivergence |= isLoggedDivergentGameStateField("var_B960", 0, gameState1.general.var_B960, gameState2.general.var_B960);
        foundDivergence |= isLoggedDivergentGameStateField("pad_B961", 0, gameState1.general.pad_B961, gameState2.general.pad_B961);
        foundDivergence |= isLoggedDivergentGameStateField("var_B962", 0, gameState1.general.var_B962, gameState2.general.var_B962);
        foundDivergence |= isLoggedDivergentGameStateField("pad_B963", 0, gameState1.general.pad_B963, gameState2.general.pad_B963);
        foundDivergence |= isLoggedDivergentGameStateField("var_B964", 0, gameState1.general.var_B964, gameState2.general.var_B964);
        foundDivergence |= isLoggedDivergentGameStateField("pad_B965", 0, gameState1.general.pad_B965, gameState2.general.pad_B965);
        foundDivergence |= isLoggedDivergentGameStateField("var_B966", 0, gameState1.general.var_B966, gameState2.general.var_B966);
        foundDivergence |= isLoggedDivergentGameStateField("pad_B967", 0, gameState1.general.pad_B967, gameState2.general.pad_B967);
        foundDivergence |= isLoggedDivergentGameStateField("currentRainLevel", 0, gameState1.general.currentRainLevel, gameState2.general.currentRainLevel);
        foundDivergence |= isLoggedDivergence("pad_B969", gameState1.general.pad_B969, gameState2.general.pad_B969, 0xB96C - 0xB969, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("companies", gameState1.companies, gameState2.companies, S5::Limits::kMaxCompanies, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("towns", gameState1.towns, gameState2.towns, S5::Limits::kMaxTowns, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("industries", gameState1.industries, gameState2.industries, S5::Limits::kMaxIndustries, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("stations", gameState1.stations, gameState2.stations, S5::Limits::kMaxStations, displayAllDivergences);
        foundDivergence |= logDivergentEntity(gameState1.entities, gameState2.entities, S5::Limits::kMaxEntities, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("animations", gameState1.animations, gameState2.animations, S5::Limits::kMaxAnimations, displayAllDivergences);
        foundDivergence |= isLoggedDivergence("waves", gameState1.waves, gameState2.waves, S5::Limits::kMaxWaves, displayAllDivergences);
        foundDivergence |= isLoggedDivergenceUserString("userStrings ", gameState1.userStrings, gameState2.userStrings, displayAllDivergences);
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
            Logging::info("The TileElements sizes are different.");
            Logging::info("Size of TileElements1 = {}", tileElements1.size());
            Logging::info("Size of TileElements2 = {}", tileElements2.size());
        }

        int elementCount = 0;
        auto iterator1 = tileElements1.begin();
        auto iterator2 = tileElements2.begin();
        for (auto y = 0; y < 384; ++y)
        {
            for (auto x = 0; x < 384; ++x)
            {
                auto allElementsOnTile = [](auto& iter) {
                    std::vector<S5::TileElement> ts;
                    do
                    {
                        ts.push_back(*iter);
                    } while (!iter++->isLast());
                    return ts;
                };
                const auto t1s = allElementsOnTile(iterator1);
                const auto t2s = allElementsOnTile(iterator2);
                auto i = 0U;
                auto limit = std::min(t1s.size(), t2s.size());
                for (; i < limit; ++i)
                {
                    if (!bitWiseEqual(t1s[i], t2s[i]))
                    {
                        if (divergentBytesTotal == 0)
                        {
                            Logging::info("DIVERGENCE");
                        }
                        if (divergentBytesTotal == 0 || displayAllDivergences)
                        {
                            Logging::info("TILE ELEMENT[{}] TYPE[{}] x:{}, y:{}", elementCount, (t1s[i].type & 0x3C) >> 2, x, y);
                        }
                        divergentBytesTotal = bitWiseLogDivergence("Elements[" + std::to_string(elementCount) + "] x:" + std::to_string(x) + ", y:" + std::to_string(y), t1s[i], t2s[i], displayAllDivergences, divergentBytesTotal);
                    }
                    elementCount++;
                }

                for (; i < t1s.size(); ++i)
                {
                    if (divergentBytesTotal == 0)
                    {
                        Logging::info("DIVERGENCE");
                    }
                    if (divergentBytesTotal == 0 || displayAllDivergences)
                    {
                        Logging::info("Extra TILE ELEMENT [{}] x:{}, y:{}", elementCount, x, y);
                    }
                    if (displayAllDivergences)
                    {
                        std::span<const std::byte> bytesSpanLhs = getBytesSpan(t1s[i]);
                        for (size_t offset = 0; offset < sizeof(S5::TileElement); offset++)
                        {
                            Logging::info("    OFFSET: {}", offset);
                            Logging::info("    LHS: {:#x}", bytesSpanLhs[offset]);
                        }
                    }
                    divergentBytesTotal += sizeof(S5::TileElement);
                    elementCount++;
                }
                for (; i < t2s.size(); ++i)
                {
                    if (divergentBytesTotal == 0)
                    {
                        Logging::info("DIVERGENCE");
                    }
                    if (divergentBytesTotal == 0 || displayAllDivergences)
                    {
                        Logging::info("Removed TILE ELEMENT [{}] x:{}, y:{}", elementCount, x, y);
                    }

                    if (displayAllDivergences)
                    {
                        std::span<const std::byte> bytesSpanRhs = getBytesSpan(t2s[i]);
                        for (size_t offset = 0; offset < sizeof(S5::TileElement); offset++)
                        {
                            Logging::info("    OFFSET: {}", offset);
                            Logging::info("    RHS: {:#x}", bytesSpanRhs[offset]);
                        }
                    }
                    divergentBytesTotal += sizeof(S5::TileElement);
                }
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
        FileStream referenceFile(path, StreamMode::read);
        auto referenceGameState = S5::importSave(referenceFile);
        return compareGameStates(*currentS5GameState, referenceGameState->gameState, false);
    }

    bool compareGameStates(const fs::path& path1, const fs::path& path2, bool displayAllDivergences)
    {
        Logging::info("Comparing game state files:");
        Logging::info("   file1: {}", path1);
        Logging::info("   file2: {}", path2);

        FileStream file1(path1, StreamMode::read);
        auto state1 = S5::importSave(file1);
        FileStream file2(path2, StreamMode::read);
        auto state2 = S5::importSave(file2);
        auto match = compareGameStates(state1->gameState, state2->gameState, displayAllDivergences);
        match &= compareElements(state1->tileElements, state2->tileElements, displayAllDivergences);
        return match;
    }
}
