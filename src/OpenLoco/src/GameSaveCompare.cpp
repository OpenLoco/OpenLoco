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
    long logDivergentEntityOffset(const OpenLoco::Entity& lhs, const S5::Entity& rhs, int offset, bool displayAllDivergences, long divergentBytesTotal);
    bool compareGameStates(GameState& gameState1, S5::GameState& gameState2, bool displayAllDivergences);
    bool logDivergenceRoutings(OpenLoco::GameState& gameState1, OpenLoco::S5::GameState& gameState2, bool displayAllDivergences);
    bool compareElements(const std::vector<S5::TileElement>& tileElements1, const std::vector<S5::TileElement>& tileElements2, bool displayAllDivergences);

    template<typename T>
    std::span<const std::byte> getBytesSpan(const T& item)
    {
        return std::span<const std::byte, sizeof(T)>{ reinterpret_cast<const std::byte*>(std::addressof(item)), sizeof(T) };
    }

    void bitWiseLogOffsetDivergence(char* array_lhs, int offset, char* array_rhs, bool& printHeader, const std::string& type);
    template<typename T>
    constexpr auto begin(const T& item)
    {
        return reinterpret_cast<const char*>(&item);
    }

    template<typename T>
    constexpr auto end(const T& item)
    {
        return reinterpret_cast<const char*>(&item) + sizeof(T);
    }

    template<typename T>
    auto bitWiseEqual(const T& lhs, const T& rhs)
    {
        return std::equal(begin(lhs), end(lhs), begin(rhs), end(rhs));
    }

    template<typename T1, typename T2>
    auto bitWiseEqual(const T1& lhs, const T2& rhs)
    {
        return std::equal(begin(lhs), end(lhs), begin(rhs), end(rhs));
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
                    Logging::info("    LHS: {:#x}", bytesSpanRhs[offset]);
                    Logging::info("    RHS: {:#x}", bytesSpanRhs[offset]);
                }
                divergentBytesTotal++;
            }
        }
        return divergentBytesTotal;
    }

    template<typename T1, typename T2>
    bool logDivergence(const std::string type, const T1& lhs, const T2& rhs, bool displayAllDivergences)
    {
        long divergentBytesTotal = 0;
        for (unsigned int offset = 0; offset < sizeof(lhs); offset++)
        {
            divergentBytesTotal = bitWiseLogDivergence(type + " [" + std::to_string(offset) + "]", lhs[offset], rhs[offset], displayAllDivergences, divergentBytesTotal);
        }
        if (!displayAllDivergences && divergentBytesTotal > 0)
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

    template<typename T>
    bool logDivergentGameStateField(const std::string type, int offset, const T& lhs, const T& rhs)
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
    bool logDivergentGameStateFieldNoHeader(int offset, const T& lhs, const T& rhs)
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

    long logDivergentEntityOffset(const OpenLoco::Entity& lhs, const S5::Entity& rhs, int offset, bool displayAllDivergences, long divergentBytesTotal)
    {
        if (!bitWiseEqual(lhs, rhs))
        {
            if (divergentBytesTotal == 0)
            {
                Logging::info("DIVERGENCE");
            }
            char* entity = (char*)(&rhs);
            OpenLoco::Entity* rhsEntity = reinterpret_cast<OpenLoco::Entity*>(entity);

            if (lhs.baseType == EntityBaseType::vehicle)
            {
                if (displayAllDivergences || divergentBytesTotal == 0)
                {
                    logVehicleTypeAndSubType(offset, lhs);
                }
                divergentBytesTotal += sizeof(lhs.baseType);
            }
            if (lhs.baseType != rhsEntity->baseType)
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
            if (lhs.baseType == EntityBaseType::effect)
            {
                if (displayAllDivergences || divergentBytesTotal == 0)
                {
                    logEffectType(offset, lhs);
                }
                else
                {
                    divergentBytesTotal += sizeof(lhs.baseType);
                }
            }
            if (lhs.baseType != rhsEntity->baseType)
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
            if (lhs.baseType == EntityBaseType::null)
            {
                if (displayAllDivergences || divergentBytesTotal == 0)
                {
                    Logging::info("TYPE: ENTITY [{}] NULL", offset);
                }
                else
                {
                    divergentBytesTotal += sizeof(lhs.baseType);
                }
            }
            else
            {
                divergentBytesTotal += sizeof(lhs.baseType);
            }
            if (lhs.baseType != rhsEntity->baseType)
            {
                if (rhsEntity->baseType == EntityBaseType::null)
                {

                    if (displayAllDivergences || divergentBytesTotal == 0)
                    {
                        Logging::info("TYPE: ENTITY [{}] NULL", offset);
                    }
                    else
                    {
                        divergentBytesTotal += sizeof(lhs.baseType);
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
    long logDivergentEntity(const T1& lhs, const T2& rhs, int arraySize, bool displayAllDivergences)
    {
        long divergentBytesTotal = 0;
        for (int offset = 0; offset < arraySize; offset++)
        {
            divergentBytesTotal = logDivergentEntityOffset(lhs[offset], rhs[offset], offset, displayAllDivergences, divergentBytesTotal);
        }
        if (!displayAllDivergences && divergentBytesTotal > 0)
        {
            Logging::info(" {} other diverging bytes omitted", divergentBytesTotal);
        }
        return divergentBytesTotal;
    }

    bool compareGameStates(GameState& gameState1, S5::GameState& gameState2, bool displayAllDivergences)
    {
        if (displayAllDivergences)
            Logging::info("display all divergences!");

        bool foundDivergence = false;

        foundDivergence = logDivergentGameStateField("rng_0", 0, gameState1.rng.srand_0(), gameState2.rng[0])
            || foundDivergence;
        foundDivergence = logDivergentGameStateField("rng_1", 0, gameState1.rng.srand_1(), gameState2.rng[1])
            || foundDivergence;

        if (gameState1.flags != gameState2.flags)
        {
            auto flags1 = static_cast<uint32_t>(gameState1.flags);
            auto flags2 = static_cast<uint32_t>(gameState2.flags);
            Logging::info("DIVERGENCE");
            Logging::info("TYPE: {}", "flags");
            Logging::info("    OFFSET: {}", 0);
            Logging::info("    LHS: {:#x}", flags1);
            Logging::info("    LHS: {:#x}", flags2);
        }

        foundDivergence = logDivergentGameStateField("currentDay", 0, gameState1.currentDay, gameState2.currentDay)
            || foundDivergence;
        foundDivergence = logDivergentGameStateField("dayCounter", 0, gameState1.dayCounter, gameState2.dayCounter)
            || foundDivergence;
        foundDivergence = logDivergentGameStateField("currentYear", 0, gameState1.currentYear, gameState2.currentYear)
            || foundDivergence;
        foundDivergence = logDivergentGameStateField("currentMonth", 0, gameState1.currentMonth, gameState2.currentMonth)
            || foundDivergence;
        foundDivergence = logDivergentGameStateField("currentMonthOfMonth", 0, gameState1.currentDayOfMonth, gameState2.currentDayOfMonth)
            || foundDivergence;

        foundDivergence = logDivergence("companies", gameState1.companies, gameState2.companies, Limits::kMaxCompanies, displayAllDivergences)
            || foundDivergence;
        foundDivergence = logDivergence("towns", gameState1.towns, gameState2.towns, Limits::kMaxTowns, displayAllDivergences)
            || foundDivergence;
        foundDivergence = logDivergence("industries", gameState1.industries, gameState2.industries, Limits::kMaxIndustries, displayAllDivergences)
            || foundDivergence;
        foundDivergence = logDivergence("stations", gameState1.stations, gameState2.stations, Limits::kMaxStations, displayAllDivergences)
            || foundDivergence;
        foundDivergence = logDivergentEntity(gameState1.entities, gameState2.entities, Limits::kMaxEntities, displayAllDivergences)
            || foundDivergence;

        foundDivergence = logDivergence("animations", gameState1.animations, gameState2.animations, Limits::kMaxAnimations, displayAllDivergences)
            || foundDivergence;
        foundDivergence = logDivergence("waves", gameState1.waves, gameState2.waves, Limits::kMaxWaves, displayAllDivergences)
            || foundDivergence;
        foundDivergence = logDivergence("userStrings ", gameState1.userStrings, gameState2.userStrings, Limits::kMaxUserStrings, displayAllDivergences)
            || foundDivergence;

        foundDivergence = logDivergenceRoutings(gameState1, gameState2, displayAllDivergences)
            || foundDivergence;

        foundDivergence = logDivergence("orders", gameState1.orders, gameState2.orders, displayAllDivergences)
            || foundDivergence;

        return not foundDivergence;
    }

    bool logDivergenceRoutings(OpenLoco::GameState& gameState1, OpenLoco::S5::GameState& gameState2, bool displayAllDivergences)
    {
        long divergentBytesTotal = 0;
        for (unsigned int route = 0; route < Limits::kMaxVehicles; route++)
        {
            for (unsigned int routePerVehicle = 0; routePerVehicle < Limits::kMaxRoutingsPerVehicle; routePerVehicle++)
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
                    logDivergentGameStateFieldNoHeader(
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
                auto tile1 = tileElements1.at(elementCount);
                auto tile2 = tileElements2.at(elementCount);

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
        Logging::info("Comparing reference file {} to current GameState frame", path);
        return compareGameStates(getGameState(), S5::importSave(path).get()->gameState, false);
    }

    bool compareGameStates(const fs::path& path1, const fs::path& path2, bool displayAllDivergences)
    {
        Logging::info("Comparing game state files:");
        Logging::info("   file1: {}", path1);
        Logging::info("   file2: {}", path2);

        S5::setGameState(S5::importSave(path1));

        bool match = false;
        match = compareGameStates(getGameState(), S5::importSave(path2).get()->gameState, displayAllDivergences)
            || match;
        match = compareElements(S5::importSave(path1).get()->tileElements, S5::importSave(path2).get()->tileElements, displayAllDivergences)
            || match;
        return match;
    }
}
