#include "ScenarioConstruction.h"
#include "GameState.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Objects/VehicleObject.h"
#include "World/CompanyManager.h"

namespace OpenLoco::Scenario
{
    Construction& getConstruction()
    {
        return getGameState().scenarioConstruction;
    }

    // 0x00475988
    void resetRoadObjects()
    {
        auto& construction = getGameState().scenarioConstruction;
        for (auto i = 0U; i < 8; i++)
        {
            construction.var_17A[i] = 0xFF;
            construction.roadStations[i] = 0xFF;
            construction.roadMods[i] = 0xFF;
        }
    }

    void resetTrackObjects()
    {
        auto& construction = getConstruction();
        std::fill(std::begin(construction.signals), std::end(construction.signals), 0xFF);
        std::fill(std::begin(construction.bridges), std::end(construction.bridges), 0xFF);
        std::fill(std::begin(construction.trainStations), std::end(construction.trainStations), 0xFF);
        std::fill(std::begin(construction.trackMods), std::end(construction.trackMods), 0xFF);
    }

    // 0x0049B546
    void initialiseDefaultTrackRoadMods()
    {
        auto& construction = getConstruction();

        for (uint8_t trackObjId = 0; trackObjId < ObjectManager::getMaxObjects(ObjectType::track); ++trackObjId)
        {
            auto* trackObj = ObjectManager::get<TrackObject>(trackObjId);
            if (trackObj == nullptr)
            {
                continue;
            }
            uint32_t requiredMods = 0xFFFFFFFFU;
            for (uint8_t vehicleObjId = 0; vehicleObjId < ObjectManager::getMaxObjects(ObjectType::vehicle); ++vehicleObjId)
            {
                auto* vehObj = ObjectManager::get<VehicleObject>(vehicleObjId);
                if (vehObj == nullptr)
                {
                    continue;
                }

                if (vehObj->mode != TransportMode::rail)
                {
                    continue;
                }

                if (vehObj->trackType != trackObjId)
                {
                    continue;
                }

                if (vehObj->power == 0)
                {
                    continue;
                }

                auto* playerCompany = CompanyManager::getPlayerCompany();
                if (!playerCompany->isVehicleIndexUnlocked(vehicleObjId))
                {
                    continue;
                }

                uint32_t mods = 0U;
                for (auto i = 0; i < vehObj->numTrackExtras; ++i)
                {
                    mods |= 1U << vehObj->requiredTrackExtras[i];
                }
                requiredMods &= mods;
            }
            construction.trackMods[trackObjId] = 0xFFU;

            if (requiredMods != 0xFFFFFFFFU && requiredMods != 0)
            {
                uint8_t presetMods = 0U;
                for (auto i = 0; i < trackObj->numMods; ++i)
                {
                    if (requiredMods & (1U << trackObj->mods[i]))
                    {
                        presetMods |= 1U << i;
                    }
                }
                construction.trackMods[trackObjId] = presetMods;
            }
        }

        for (uint8_t roadObjId = 0; roadObjId < ObjectManager::getMaxObjects(ObjectType::road); ++roadObjId)
        {
            auto* roadObj = ObjectManager::get<RoadObject>(roadObjId);
            if (roadObj == nullptr)
            {
                continue;
            }
            uint32_t requiredMods = 0xFFFFFFFFU;
            for (uint8_t vehicleObjId = 0; vehicleObjId < ObjectManager::getMaxObjects(ObjectType::vehicle); ++vehicleObjId)
            {
                auto* vehObj = ObjectManager::get<VehicleObject>(vehicleObjId);
                if (vehObj == nullptr)
                {
                    continue;
                }

                if (vehObj->mode != TransportMode::road)
                {
                    continue;
                }

                if (vehObj->trackType != roadObjId)
                {
                    continue;
                }

                if (vehObj->power == 0)
                {
                    continue;
                }

                auto* playerCompany = CompanyManager::getPlayerCompany();
                if (!playerCompany->isVehicleIndexUnlocked(vehicleObjId))
                {
                    continue;
                }

                uint32_t mods = 0U;
                for (auto i = 0; i < vehObj->numTrackExtras; ++i)
                {
                    mods |= 1U << vehObj->requiredTrackExtras[i];
                }
                requiredMods &= mods;
            }
            construction.roadMods[roadObjId] = 0xFFU;

            if (requiredMods != 0xFFFFFFFFU && requiredMods != 0)
            {
                uint8_t presetMods = 0U;
                for (auto i = 0; i < roadObj->numMods; ++i)
                {
                    if (requiredMods & (1U << roadObj->mods[i]))
                    {
                        presetMods |= 1U << i;
                    }
                }
                construction.trackMods[roadObjId] = presetMods;
            }
        }
    }
}
