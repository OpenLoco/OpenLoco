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
    namespace unsafe
    {
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
            return std::equal(begin(lhs), end(lhs), // will become constexpr with C++20
                              begin(rhs),
                              end(rhs));
        }
        template<typename T1, typename T2>
        auto bitWiseEqual(const T1& lhs, const T2& rhs)
        {
            return std::equal(begin(lhs), end(lhs), // will become constexpr with C++20
                              begin(rhs),
                              end(rhs));
        }

        template<typename T1, typename T2>
        void bitWiseLogDivergence(const std::string type, const T1& lhs, const T2& rhs, bool printAHeader)
        {
            size_t size = sizeof(T1) / sizeof(char);
            char* array_lhs = (char*)(&lhs);
            char* array_rhs = (char*)(&rhs);

            bool printHeader = true;

            if (!printAHeader)
                printHeader = false;

            for (size_t offset = 0; offset < size; offset++)
            {
                if (array_lhs[offset] != array_rhs[offset])
                {
                    if (printHeader)
                    {
                        Logging::info("DIVERGENCE");
                        Logging::info("TYPE: {}", type);
                        printHeader = false;
                    }

                    Logging::info("    OFFSET: {}", offset);
                    Logging::info("    LHS: {:#x}", array_lhs[offset]);
                    Logging::info("    RHS: {:#x}", array_rhs[offset]);
                }
            }
        }

        template<typename T1, typename T2>
        bool bitWiseCompare(const T1& lhs, const T2& rhs)
        {
            int size = sizeof(T1) / sizeof(char);
            char* array_lhs = (char*)(&lhs);
            char* array_rhs = (char*)(&rhs);

            for (int offset = 0; offset < size; offset++)
            {
                if (array_lhs[offset] != array_rhs[offset])
                    return false;
            }
            return true;
        }
    }

    template<typename T1, typename T2>
    void logDivergence(const std::string type, const T1& lhs, const T2& rhs)
    {
        for (unsigned int offset = 0; offset < sizeof(lhs); offset++)
            unsafe::bitWiseLogDivergence(type + " [" + std::to_string(offset) + "]", lhs[offset], rhs[offset], true);
    }

    template<typename T1, typename T2>
    void logDivergence(const std::string type, const T1& lhs, const T2& rhs, int arraySize)
    {
        for (int offset = 0; offset < arraySize; offset++)
            unsafe::bitWiseLogDivergence(type + " [" + std::to_string(offset) + "]", lhs[offset], rhs[offset], true);
    }

    template<typename T>
    void logDivergentGameStateField(const std::string type, int offset, const T& lhs, const T& rhs)
    {
        if (!unsafe::bitWiseEqual(lhs, rhs))
        {
            Logging::info("DIVERGENCE");
            Logging::info("TYPE: {}", type);
            Logging::info("    OFFSET: {}", offset);
            Logging::info("    LHS: {:#x}", lhs);
            Logging::info("    RHS: {:#x}", rhs);
        }
    }

    template<typename T>
    void logDivergentGameStateFieldNoHeader(int offset, const T& lhs, const T& rhs)
    {
        if (!unsafe::bitWiseEqual(lhs, rhs))
        {
            Logging::info("    OFFSET: {}", offset);
            Logging::info("    LHS: {:#x}", lhs);
            Logging::info("    RHS: {:#x}", rhs);
        }
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

    void logVehicleTypeAndSubTYpe(int offset, const OpenLoco::Entity& entity)
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

    void logDivergentEntityOffset(const OpenLoco::Entity& lhs, const S5::Entity& rhs, int offset)
    {
        if (!unsafe::bitWiseEqual(lhs, rhs))
        {
            char* entity = (char*)(&rhs);
            OpenLoco::Entity* rhsEntity = reinterpret_cast<OpenLoco::Entity*>(entity);

            // Logging::info("DIVERGENCE");
            if (lhs.baseType == EntityBaseType::vehicle)
            {
                logVehicleTypeAndSubTYpe(offset, lhs);
            }
            if (lhs.baseType != rhsEntity->baseType)
            {
                if (rhsEntity->baseType == EntityBaseType::vehicle)
                {
                    logVehicleTypeAndSubTYpe(offset, *rhsEntity);
                }
            }
            if (lhs.baseType == EntityBaseType::effect)
            {
                logEffectType(offset, lhs);
            }
            if (lhs.baseType != rhsEntity->baseType)
            {
                if (rhsEntity->baseType == EntityBaseType::effect)
                {
                    logEffectType(offset, *rhsEntity);
                }
            }
            if (lhs.baseType == EntityBaseType::null)
                Logging::info("TYPE: ENTITY [{}] NULL", offset);
            if (lhs.baseType != rhsEntity->baseType)
            {
                if (rhsEntity->baseType == EntityBaseType::null)
                    Logging::info("TYPE: ENTITY [{}] NULL", offset);
            }
            unsafe::bitWiseLogDivergence("", lhs, rhs, false);
        }
    }

    template<typename T1, typename T2>
    void logDivergentEntity(const T1& lhs, const T2& rhs, int arraySize)
    {
        for (int offset = 0; offset < arraySize; offset++)
            logDivergentEntityOffset(lhs[offset], rhs[offset], offset);
    }

    void compareGameStates(GameState& gameState1, S5::GameState& gameState2)
    {
        logDivergentGameStateField("rng_0", 0, gameState1.rng.srand_0(), gameState2.rng[0]);
        logDivergentGameStateField("rng_1", 0, gameState1.rng.srand_1(), gameState2.rng[1]);

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

        logDivergentGameStateField("currentDay", 0, gameState1.currentDay, gameState2.currentDay);
        logDivergentGameStateField("dayCounter", 0, gameState1.dayCounter, gameState2.dayCounter);
        logDivergentGameStateField("currentYear", 0, gameState1.currentYear, gameState2.currentYear);
        logDivergentGameStateField("currentMonth", 0, gameState1.currentMonth, gameState2.currentMonth);
        logDivergentGameStateField("currentMonthOfMonth", 0, gameState1.currentDayOfMonth, gameState2.currentDayOfMonth);

        logDivergence("companies", gameState1.companies, gameState2.companies, Limits::kMaxCompanies);
        logDivergence("towns", gameState1.towns, gameState2.towns, Limits::kMaxTowns);
        logDivergence("industries", gameState1.industries, gameState2.industries, Limits::kMaxIndustries);
        logDivergence("stations", gameState1.stations, gameState2.stations, Limits::kMaxStations);
        logDivergentEntity(gameState1.entities, gameState2.entities, Limits::kMaxEntities);

        logDivergence("animations", gameState1.animations, gameState2.animations, Limits::kMaxAnimations);
        logDivergence("waves", gameState1.waves, gameState2.waves, Limits::kMaxWaves);
        logDivergence("userStrings ", gameState1.userStrings, gameState2.userStrings, Limits::kMaxUserStrings);

        for (unsigned int route = 0; route < Limits::kMaxVehicles; route++)
        {
            bool printHeader = true;

            for (unsigned int routePerVehicle = 0; routePerVehicle < Limits::kMaxRoutingsPerVehicle; routePerVehicle++)
            {
                if (!unsafe::bitWiseEqual(gameState1.routings[route][routePerVehicle], gameState2.routings[route][routePerVehicle]))
                {
                    if (printHeader)
                    {
                        Logging::info("DIVERGENCE");
                        Logging::info("TYPE: routings [{}][{}]", route, routePerVehicle);
                        printHeader = false;
                    }
                }

                logDivergentGameStateFieldNoHeader(
                    route * Limits::kMaxOrdersPerVehicle + routePerVehicle,
                    gameState1.routings[route][routePerVehicle],
                    gameState2.routings[route][routePerVehicle]);
            }
        }

        logDivergence("orders", gameState1.orders, gameState2.orders);
    }

    void compareElements(const std::vector<S5::TileElement>& tileElements1, const std::vector<S5::TileElement>& tileElements2)
    {
        Logging::info("Size of TileElements1={}", tileElements1.size());
        Logging::info("Size of TileElements1={}", tileElements2.size());
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
        }
        int elementCount = 0;
        for (auto iterator1 = smaller.begin(), iterator2 = larger.begin(); iterator1 != smaller.end();
             ++iterator1, ++iterator2)
        {
            if (iterator2 != larger.end())
            {
                auto tile1 = tileElements1.at(elementCount);
                auto tile2 = tileElements2.at(elementCount);

                if (!unsafe::bitWiseEqual(tile1, tile2))
                {
                    Logging::info("DIVERGENCE");
                    Logging::info("TILE ELEMENT[{}]", elementCount);
                    unsafe::bitWiseLogDivergence("Elements[{}]" + std::to_string(elementCount), tile1, tile2, false);
                }
                elementCount++;
            }
        }
    }

    void compareGameStates(const fs::path& path)
    {
        Logging::info("Comparing reference file {} to current GameState frame", path);
        compareGameStates(getGameState(), S5::importSave(path).get()->gameState);
    }

    void compareGameStates(const fs::path& path1, const fs::path& path2)
    {
        Logging::info("Comparing game state files:");
        Logging::info("   file1: {}", path1);
        Logging::info("   file2: {}", path2);

        S5::setGameState(S5::importSave(path1));
        compareGameStates(getGameState(), S5::importSave(path2).get()->gameState);
        compareElements(S5::importSave(path1).get()->tileElements, S5::importSave(path2).get()->tileElements);
    }
}
