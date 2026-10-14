#pragma once
#include "Vehicle.h"

namespace OpenLoco::Vehicles
{
    enum class MotorState : uint8_t
    {
        stopped = 0,
        accelerating = 1,
        coasting = 2,
        braking = 3,
        stoppedOnIncline = 4,
        airplaneAtTaxiSpeed = 5,
    };

    enum class Flags73 : uint8_t // veh2 Train breakdown flags
    {
        none = 0U,
        isBrokenDown = 1U << 0,
        isStillPowered = 1U << 1
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(Flags73);

    struct Vehicle2 : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::vehicle_2;
        VehicleSound sound;
        int8_t var_4F;
        uint16_t totalPower; // maybe not used by aircraft and ship
        uint16_t totalWeight;
        Speed16 maxSpeed;
        Speed32 currentSpeed;
        MotorState motorState;
        uint8_t brakeLightTimeout;
        Speed16 rackRailMaxSpeed;
        currency32_t curMonthRevenue; // monthly revenue
        currency32_t profit[4];       // last 4 months net profit
        uint8_t reliability;
        Flags73 var_73; // (bit 0 = broken down, bit 1 = still powered)

        bool has73Flags(Flags73 flagsToTest) const;

        bool update();
        bool sub_4A9F20();
        currency32_t totalRecentProfit() const
        {
            return profit[0] + profit[1] + profit[2] + profit[3];
        }
    };
    static_assert(sizeof(Vehicle2) <= sizeof(Entity));

    void railProduceCrossingWhistle(const Vehicle2& veh2);
}
