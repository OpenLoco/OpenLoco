#pragma once
#include "Entities/EntityManager.h"
#include "Types.hpp"

namespace OpenLoco
{
    struct Company;
}

namespace OpenLoco::Vehicles
{
    struct VehicleHead;
    struct Car;
    struct TrackAndDirection;
}

namespace OpenLoco::VehicleManager
{
    using VehicleList = EntityManager::EntityList<EntityManager::EntityListIterator<Vehicles::VehicleHead>, EntityManager::EntityListType::vehicleHead>;

    void honkAllTrains();
    void update();
    void updateMonthly();
    void updateDaily();
    void determineAvailableVehicles(Company& company);
    void resetIfHeadingForStation(const StationId stationId);
    void deleteTrain(Vehicles::VehicleHead& head);
    void deleteCar(Vehicles::Car& car);

    enum class PlaceDownResult
    {
        Okay,
        Unk0,
        Unk1
    };

    PlaceDownResult placeDownVehicle(Vehicles::VehicleHead* const head, const coord_t x, const coord_t y, const uint8_t baseZ, const Vehicles::TrackAndDirection& unk1, const uint16_t unk2);
}
