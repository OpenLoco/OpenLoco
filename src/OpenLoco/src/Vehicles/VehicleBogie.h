#pragma once
#include "Vehicle.h"

namespace OpenLoco
{
    enum class AirportObjectFlags : uint16_t;
}

namespace OpenLoco::Vehicles
{
    struct VehicleBogie : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::bogie;
        ColourScheme colourScheme;
        uint8_t objectSpriteType;
        uint16_t objectId;
        uint16_t var_44;
        uint8_t animationIndex;      // animation index
        uint8_t var_47;              // cargo sprite index (unused)
        VehicleCargo secondaryCargo; // Note back bogie cannot carry cargo always check type
        uint16_t totalCarWeight;     // only valid for first bogie of car
        uint8_t bodyIndex;
        uint32_t creationDay;
        uint32_t var_5A;
        uint8_t wheelSlipping; // timeout that counts up
        BreakdownFlags breakdownFlags;
        uint8_t var_60;
        uint8_t var_61;
        uint32_t refundCost;         // front bogies only
        uint16_t reliability;        // front bogies only
        uint16_t timeoutToBreakdown; // front bogies only (days) counts down to the next breakdown 0xFFFFU disables this
        uint8_t breakdownTimeout;    // front bogies only (days)

    public:
        AirportObjectFlags getCompatibleAirportType();
        bool update();
        void updateSegmentCrashed();
        bool isOnRackRail();
        constexpr bool hasBreakdownFlags(BreakdownFlags flagsToTest) const
        {
            return (breakdownFlags & flagsToTest) != BreakdownFlags::none;
        }

    private:
        void updateRoll(const int32_t unkDistance);
        void collision(const EntityId collideEntityId);
    };
    static_assert(sizeof(VehicleBogie) <= sizeof(Entity));
}
