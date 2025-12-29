#pragma once
#include "Vehicle.h"

namespace OpenLoco::Vehicles
{
    using CargoTotalArray = std::array<uint32_t, Limits::kMaxCargoObjects>;

    enum class Status : uint8_t
    {
        unk_0 = 0, // no position (not placed)
        stopped = 1,
        travelling = 2,
        waitingAtSignal = 3,
        approaching = 4,
        unloading = 5,
        loading = 6,
        brokenDown = 7,
        crashed = 8,
        stuck = 9,
        landing = 10,
        taxiing1 = 11,
        taxiing2 = 12,
        takingOff = 13,
    };

    enum class WaterMotionFlags : uint32_t
    {
        none = 0U,
        isStopping = 1U << 0,
        isLeavingDock = 1U << 1,
        hasReachedDock = 1U << 16,
        hasReachedADestination = 1U << 17,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(WaterMotionFlags);

    enum class SignalTimeoutStatus : uint32_t
    {
        ok = 0U,
        firstTimeout = 1U,
        turnaroundAtSignalTimeout = 2U,
    };

    struct VehicleStatus
    {
        StringId status1;
        uint32_t status1Args;
        StringId status2;
        uint32_t status2Args;
    };

    struct VehicleHead : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::head;
        int32_t var_3C;
        uint32_t orderTableOffset; // offset into Order Table
        uint16_t currentOrder;     // offset, combine with orderTableOffset
        uint16_t sizeOfOrderTable; // size of Order Table
        uint32_t trainAcceptedCargoTypes;
        int16_t ordinalNumber;
        uint8_t var_52;
        uint8_t var_53; //  mods?
        StationId stationId;
        uint16_t cargoTransferTimeout;
        uint32_t var_58;
        uint8_t var_5C;
        Status status;
        VehicleType vehicleType;
        BreakdownFlags breakdownFlags;
        World::Pos2 aiPlacementPos;
        uint8_t aiThoughtId; //  0xFFU for null
        uint8_t airportMovementEdge;
        uint16_t aiPlacementTaD; //  for air/water this is just rotation
        uint8_t aiPlacementBaseZ;
        uint8_t crashedTimeout;
        Speed16 lastAverageSpeed;
        uint32_t totalRefundCost;
        World::Pos2 journeyStartPos;       //  journey start position
        uint32_t journeyStartTicks;        //  ticks since journey start
        int8_t manualPower;                //  manual power control VehicleFlags::manualControl
        uint8_t restartStoppedCarsTimeout; //  timeout before auto starting trams/buses

    public:
        bool isVehicleTypeCompatible(const uint16_t vehicleTypeId);
        void updateBreakdown();
        void updateVehicle();
        bool update();
        void updateMonthly();
        void updateDaily();
        VehicleStatus getStatus() const;
        OrderRingView getCurrentOrders() const;
        bool isPlaced() const { return tileX != -1 && !has38Flags(Flags38::isGhost); }
        bool hasAnyCargo();
        char* generateCargoTotalString(char* buffer);
        char* generateCargoCapacityString(char* buffer);
        char* cargoLUTToString(CargoTotalArray& cargoTotals, char* buffer);
        bool canBeModified() const;
        void liftUpVehicle();
        void updateTrainProperties();
        currency32_t calculateRunningCost() const;
        void sub_4AD778();
        void handlePositionUpdate();
        void resetStateOnPlacementOrReverse(bool goingForward);
        uint32_t getCarCount() const;
        void applyBreakdownToTrain();
        void landCrashedUpdate();
        void autoLayoutTrain();
        uint32_t getVehicleTotalLength() const;
        constexpr bool hasBreakdownFlags(BreakdownFlags flagsToTest) const
        {
            return (breakdownFlags & flagsToTest) != BreakdownFlags::none;
        }
        void movePlaneTo(const World::Pos3& newLoc, const uint8_t newYaw, const Pitch newPitch);
        void moveBoatTo(const World::Pos3& loc, const uint8_t yaw, const Pitch pitch);
        Sub4ACEE7Result tryPositionVehicle(uint32_t unk1, uint32_t var_113612C, bool isPlaceDown);

    private:
        void updateDrivingSounds();
        void updateDrivingSound(VehicleSound& sound, const bool isVeh2);
        void updateDrivingSoundNone(VehicleSound& sound);
        void updateDrivingSoundFriction(VehicleSound& sound, const VehicleObjectFrictionSound* snd);
        void updateSimpleMotorSound(VehicleSound& sound, const bool isVeh2, const VehicleSimpleMotorSound* snd);
        void updateGearboxMotorSound(VehicleSound& sound, const bool isVeh2, const VehicleGearboxMotorSound* snd);
        bool updateLand();
        bool sub_4A8DB7();
        bool tryReverse();
        bool stoppingUpdate();
        bool manualStoppingUpdate();
        bool landTryBeginUnloading();
        bool landLoadingUpdate();
        bool landNormalMovementUpdate();
        bool trainNormalMovementUpdate(uint8_t al, uint8_t flags, StationId nextStation);
        bool roadNormalMovementUpdate(uint8_t al, StationId nextStation);
        bool landReverseFromSignal();
        bool updateAir();
        bool airplaneLoadingUpdate();
        bool sub_4A95CB();
        bool sub_4A9348(uint8_t newMovementEdge, const AirplaneApproachTargetParams& approachParams);
        bool airplaneApproachTarget(const AirplaneApproachTargetParams& params);
        std::pair<Status, Speed16> airplaneGetNewStatus();
        uint8_t airportGetNextMovementEdge(uint8_t curEdge);
        AirplaneApproachTargetParams airplanePathfind();
        bool updateWater();
        void tryCreateInitialMovementSound(const Status initialStatus);
        void setStationVisitedTypes();
        void checkIfAtOrderStation();
        void updateLastJourneyAverageSpeed();
        void beginUnloading();
        void beginLoading();
        WaterMotionFlags updateWaterMotion(WaterMotionFlags flags);
        uint8_t getLoadingModifier(const VehicleBogie* bogie);
        bool updateUnloadCargoComponent(VehicleCargo& cargo, VehicleBogie* bogie);
        void updateUnloadCargo();
        bool updateLoadCargoComponent(VehicleCargo& cargo, VehicleBogie* bogie);
        bool updateLoadCargo();
        void beginNewJourney();
        void advanceToNextRoutableOrder();
        Status approachingIfStationElseTraveling();
        void produceLeavingDockSound();
        void produceTouchdownAirportSound();
        SignalTimeoutStatus categoriseTimeElapsed();
        bool shouldPassSignal();
        bool opposingTrainAtSignal();
        bool pathingShouldReverse();
        StationId manualFindTrainStationAtLocation();
        bool isOnExpectedRoadOrTrack();
        VehicleStatus getStatusTravelling() const;
        void getSecondStatus(VehicleStatus& vehStatus) const;
        void updateLastIncomeStats(uint8_t cargoType, uint16_t cargoQty, uint16_t cargoDist, uint8_t cargoAge, currency32_t profit);
        void calculateRefundCost();
    };
    static_assert(sizeof(VehicleHead) <= sizeof(Entity));
}
