#include "ObjectUtils.h"
#include "AirportObject.h"
#include "BridgeObject.h"
#include "Date.h"
#include "DockObject.h"
#include "ObjectManager.h"
#include "RoadExtraObject.h"
#include "RoadObject.h"
#include "RoadStationObject.h"
#include "TrackExtraObject.h"
#include "TrackObject.h"
#include "TrainSignalObject.h"
#include "TrainStationObject.h"
#include "VehicleObject.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco
{
    // 0x0048D70C
    sfl::static_vector<uint8_t, Limits::kMaxAirportObjects> getAvailableAirports()
    {
        sfl::static_vector<uint8_t, Limits::kMaxAirportObjects> stationList;
        auto currentYear = getCurrentYear();
        for (uint8_t i = 0; i < Limits::kMaxAirportObjects; i++)
        {
            auto airportObj = ObjectManager::get<AirportObject>(i);
            if (airportObj == nullptr)
            {
                continue;
            }
            if (currentYear < airportObj->designedYear)
            {
                continue;
            }
            if (currentYear > airportObj->obsoleteYear)
            {
                continue;
            }
            stationList.push_back(i);
        }

        std::ranges::sort(stationList);
        return stationList;
    }

    // 0x0048D753
    sfl::static_vector<uint8_t, Limits::kMaxDockObjects> getAvailableDocks()
    {
        sfl::static_vector<uint8_t, Limits::kMaxDockObjects> stationList;
        auto currentYear = getCurrentYear();
        for (uint8_t i = 0; i < Limits::kMaxDockObjects; i++)
        {
            auto dockObj = ObjectManager::get<DockObject>(i);
            if (dockObj == nullptr)
            {
                continue;
            }
            if (currentYear < dockObj->designedYear)
            {
                continue;
            }
            if (currentYear > dockObj->obsoleteYear)
            {
                continue;
            }
            stationList.push_back(i);
        }
        std::ranges::sort(stationList);
        return stationList;
    }

    // 0x0048D678, 0x0048D5E4
    sfl::static_vector<uint8_t, Limits::kMaxTrainStationObjects> getAvailableCompatibleStations(uint8_t trackType, TransportMode transportMode)
    {
        auto currentYear = getCurrentYear();
        sfl::static_vector<uint8_t, Limits::kMaxTrainStationObjects> stationList;

        if (transportMode == TransportMode::road)
        {
            trackType &= ~(1 << 7);
            auto roadObj = ObjectManager::get<RoadObject>(trackType);

            for (auto i = 0; i < roadObj->numStations; i++)
            {
                auto station = roadObj->stations[i];
                if (station == 0xFF)
                {
                    continue;
                }
                auto roadStationObj = ObjectManager::get<RoadStationObject>(station);
                if (currentYear < roadStationObj->designedYear)
                {
                    continue;
                }
                if (currentYear > roadStationObj->obsoleteYear)
                {
                    continue;
                }
                stationList.push_back(station);
            }
        }

        if (transportMode == TransportMode::rail)
        {
            auto trackObj = ObjectManager::get<TrackObject>(trackType);

            for (auto i = 0; i < trackObj->numStations; i++)
            {
                auto station = trackObj->stations[i];
                if (station == 0xFF)
                {
                    continue;
                }
                auto trainStationObj = ObjectManager::get<TrainStationObject>(station);
                if (currentYear < trainStationObj->designedYear)
                {
                    continue;
                }
                if (currentYear > trainStationObj->obsoleteYear)
                {
                    continue;
                }
                stationList.push_back(station);
            }
        }

        for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::roadStation); i++)
        {
            uint8_t numCompatible;
            const uint8_t* mods;
            uint16_t designedYear;
            uint16_t obsoleteYear;

            if (transportMode == TransportMode::road)
            {
                const auto* roadStationObj = ObjectManager::get<RoadStationObject>(i);

                if (roadStationObj == nullptr)
                {
                    continue;
                }

                numCompatible = roadStationObj->numCompatible;
                mods = roadStationObj->mods;
                designedYear = roadStationObj->designedYear;
                obsoleteYear = roadStationObj->obsoleteYear;
            }
            else // transportMode == TransportMode::rail
            {
                auto trainStationObj = ObjectManager::get<TrainStationObject>(i);

                if (trainStationObj == nullptr)
                {
                    continue;
                }

                numCompatible = trainStationObj->numCompatible;
                mods = trainStationObj->mods;
                designedYear = trainStationObj->designedYear;
                obsoleteYear = trainStationObj->obsoleteYear;
            }

            for (auto modCount = 0; modCount < numCompatible; modCount++)
            {
                if (trackType != mods[modCount])
                {
                    continue;
                }
                if (currentYear < designedYear)
                {
                    continue;
                }
                if (currentYear > obsoleteYear)
                {
                    continue;
                }
                if (std::ranges::find(stationList, i) == std::end(stationList))
                {
                    stationList.push_back(i);
                }
            }
        }
        std::ranges::sort(stationList);
        return stationList;
    }

    // 0x0042C518, 0x0042C490
    sfl::static_vector<uint8_t, Limits::kMaxBridgeObjects> getAvailableCompatibleBridges(uint8_t trackType, TransportMode transportMode)
    {
        auto currentYear = getCurrentYear();
        sfl::static_vector<uint8_t, Limits::kMaxBridgeObjects> bridgeList;

        if (transportMode == TransportMode::road)
        {
            trackType &= ~(1 << 7);
            auto roadObj = ObjectManager::get<RoadObject>(trackType);
            for (auto i = 0; i < roadObj->numBridges; i++)
            {
                auto bridge = roadObj->bridges[i];
                if (bridge == 0xFF)
                {
                    continue;
                }
                auto bridgeObj = ObjectManager::get<BridgeObject>(bridge);
                if (currentYear < bridgeObj->designedYear)
                {
                    continue;
                }
                bridgeList.push_back(bridge);
            }
        }

        if (transportMode == TransportMode::rail)
        {
            auto trackObj = ObjectManager::get<TrackObject>(trackType);
            for (auto i = 0; i < trackObj->numBridges; i++)
            {
                auto bridge = trackObj->bridges[i];
                if (bridge == 0xFF)
                {
                    continue;
                }
                auto bridgeObj = ObjectManager::get<BridgeObject>(bridge);
                if (currentYear < bridgeObj->designedYear)
                {
                    continue;
                }
                bridgeList.push_back(bridge);
            }
        }

        for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::bridge); i++)
        {
            auto bridgeObj = ObjectManager::get<BridgeObject>(i);
            if (bridgeObj == nullptr)
            {
                continue;
            }

            uint8_t numCompatible;
            const uint8_t* mods;

            if (transportMode == TransportMode::road)
            {
                numCompatible = bridgeObj->roadNumCompatible;
                mods = bridgeObj->roadMods;
            }
            else // transportMode == TransportMode::rail
            {
                numCompatible = bridgeObj->trackNumCompatible;
                mods = bridgeObj->trackMods;
            }

            for (auto modCount = 0; modCount < numCompatible; modCount++)
            {
                if (trackType != mods[modCount])
                {
                    continue;
                }
                if (currentYear < bridgeObj->designedYear)
                {
                    continue;
                }
                if (std::ranges::find(bridgeList, i) == bridgeList.end())
                {
                    bridgeList.push_back(i);
                }
            }
        }
        std::ranges::sort(bridgeList);
        return bridgeList;
    }

    // 0x004781C5, 0x004A693D
    std::array<uint8_t, kMaxMods> getAvailableCompatibleMods(uint8_t trackType, TransportMode transportMode, CompanyId companyId)
    {
        if (transportMode == TransportMode::road)
        {
            trackType &= ~(1 << 7);
        }

        std::array<uint8_t, 4> modList = { 0xFF, 0xFF, 0xFF, 0xFF };
        auto flags = 0;

        for (uint8_t vehicle = 0; vehicle < ObjectManager::getMaxObjects(ObjectType::vehicle); vehicle++)
        {
            auto vehicleObj = ObjectManager::get<VehicleObject>(vehicle);

            if (vehicleObj == nullptr)
            {
                continue;
            }

            if (vehicleObj->mode != transportMode)
            {
                continue;
            }

            if (trackType != vehicleObj->trackType)
            {
                continue;
            }

            auto company = CompanyManager::get(companyId);

            if (!company->isVehicleIndexUnlocked(vehicle))
            {
                continue;
            }

            for (auto i = 0; i < vehicleObj->numTrackExtras; i++)
            {
                flags |= 1ULL << vehicleObj->requiredTrackExtras[i];
            }

            if (!vehicleObj->hasFlags(VehicleObjectFlags::rackRail))
            {
                continue;
            }

            flags |= 1ULL << vehicleObj->rackRailType;
        }

        if (transportMode == TransportMode::road)
        {
            auto roadObj = ObjectManager::get<RoadObject>(trackType);

            for (auto i = 0; i < roadObj->numMods; i++)
            {
                if (flags & (1 << roadObj->mods[i]))
                {
                    modList[i] = roadObj->mods[i];
                }
            }
        }

        if (transportMode == TransportMode::rail)
        {
            auto trackObj = ObjectManager::get<TrackObject>(trackType);

            for (auto i = 0; i < trackObj->numMods; i++)
            {
                if (flags & (1 << trackObj->mods[i]))
                {
                    modList[i] = trackObj->mods[i];
                }
            }
        }
        return modList;
    }

    // 0x00488B4D
    sfl::static_vector<uint8_t, Limits::kMaxTrainSignalObjects> getAvailableCompatibleSignals(uint8_t trackType)
    {
        sfl::static_vector<uint8_t, Limits::kMaxTrainSignalObjects> signalList;
        auto currentYear = getCurrentYear();
        auto trackObj = ObjectManager::get<TrackObject>(trackType);
        auto signals = trackObj->signals;
        while (signals > 0)
        {
            const auto signalId = Numerics::bitScanForward(signals);
            if (signalId == -1)
            {
                break;
            }
            signals &= ~(1 << signalId);
            auto signalObj = ObjectManager::get<TrainSignalObject>(signalId);

            if (currentYear > signalObj->obsoleteYear)
            {
                continue;
            }
            if (currentYear < signalObj->designedYear)
            {
                continue;
            }
            signalList.push_back(signalId);
        }

        for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::trackSignal); i++)
        {
            auto signalObj = ObjectManager::get<TrainSignalObject>(i);
            if (signalObj == nullptr)
            {
                continue;
            }
            for (auto modCount = 0; modCount < signalObj->numCompatible; modCount++)
            {
                if (trackType != ObjectManager::get<TrainSignalObject>(i)->mods[modCount])
                {
                    continue;
                }
                if (currentYear < signalObj->designedYear)
                {
                    continue;
                }
                if (currentYear > signalObj->obsoleteYear)
                {
                    continue;
                }
                if (std::ranges::find(signalList, i) == signalList.end())
                {
                    signalList.push_back(i);
                }
            }
        }
        std::ranges::sort(signalList);
        return signalList;
    }
}
