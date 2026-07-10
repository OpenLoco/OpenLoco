#pragma once
#include "Vehicle.h"

namespace OpenLoco::Vehicles
{
    enum class Flags48 : uint8_t // veh1 Signal flags?
    {
        none = 0U,
        passSignal = 1U << 0,
        expressMode = 1U << 1,
        flag2 = 1U << 2 // cargo related?
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(Flags48);

    struct IncomeStats
    {
        int32_t day;
        uint8_t cargoTypes[4];
        uint16_t cargoQtys[4];
        uint16_t cargoDistances[4];
        uint8_t cargoAges[4];
        currency32_t cargoProfits[4];
        void beginNewIncome();
        bool addToStats(uint8_t cargoType, uint16_t cargoQty, uint16_t cargoDist, uint8_t cargoAge, currency32_t profit);
    };

    struct Vehicle1 : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::vehicle_1;
        int32_t var_3C;
        Speed16 targetSpeed;
        uint16_t timeAtSignal;
        Flags48 var_48;
        uint8_t var_49; // rackrail mod?
        uint32_t dayCreated;
        uint16_t var_4E;
        uint16_t var_50;
        uint8_t var_52;
        IncomeStats lastIncome;

        bool update();
        bool updateRoad();
        bool updateRail();
        UpdateMotionResult updateRoadMotion(int32_t distance);
    };
    static_assert(sizeof(Vehicle1) <= sizeof(Entity));
}
