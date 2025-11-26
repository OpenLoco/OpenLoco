#pragma once

#include "Audio/Audio.h"
#include "Entities/Entity.h"
#include "Map/Track/TrackModSection.h"
#include "Objects/AirportObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/VehicleObject.h"
#include "Routing.h"
#include "Speed.hpp"
#include "Types.hpp"
#include "Ui/Window.h"
#include "Ui/WindowType.h"
#include "World/Company.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Exception.hpp>

namespace OpenLoco
{
    enum class AirportObjectFlags : uint16_t;
}

namespace OpenLoco::Vehicles
{
    using CargoTotalArray = std::array<uint32_t, ObjectManager::getMaxObjects(ObjectType::cargo)>;

    constexpr auto kMaxRoadVehicleLength = 176;    // TODO: Units?
    constexpr uint8_t kWheelSlippingDuration = 64; // In ticks

    enum class MotorState : uint8_t
    {
        stopped = 0,
        accelerating = 1,
        coasting = 2,
        braking = 3,
        stoppedOnIncline = 4,
        airplaneAtTaxiSpeed = 5,
    };

    enum class Flags38 : uint8_t
    {
        none = 0U,
        unk_0 = 1U << 0,
        isReversed = 1U << 1,
        unk_2 = 1U << 2,
        jacobsBogieAvailable = 1U << 3,
        isGhost = 1U << 4,
        fasterAroundCurves = 1U << 5,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(Flags38);

    enum class Flags48 : uint8_t // veh1 Signal flags?
    {
        none = 0U,
        passSignal = 1U << 0,
        expressMode = 1U << 1,
        flag2 = 1U << 2 // cargo related?
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(Flags48);

    enum class Flags73 : uint8_t // veh2 Train breakdown flags
    {
        none = 0U,
        isBrokenDown = 1U << 0,
        isStillPowered = 1U << 1
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(Flags73);

    enum class WaterMotionFlags : uint32_t
    {
        none = 0U,
        isStopping = 1U << 0,
        isLeavingDock = 1U << 1,
        hasReachedDock = 1U << 16,
        hasReachedADestination = 1U << 17,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(WaterMotionFlags);

    enum class SoundFlags : uint16_t
    {
        none = 0U,
        flag0 = 1U << 0,
        flag1 = 1U << 1,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(SoundFlags);

    enum class UpdateVar1136114Flags : uint32_t
    {
        none = 0U,
        unk_m00 = (1U << 0),
        noRouteFound = (1U << 1),
        crashed = (1U << 2),
        unk_m03 = (1U << 3),
        approachingGradeCrossing = (1U << 4),
        unk_m15 = (1U << 15),
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(UpdateVar1136114Flags);

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

    struct OrderRingView;

    struct VehicleHead;
    struct Vehicle1;
    struct Vehicle2;
    struct VehicleBogie;
    struct VehicleBody;
    struct VehicleTail;
    struct Vehicle;
    struct VehicleSound;

    enum class BreakdownFlags : uint8_t
    {
        none = 0U,
        unk_0 = 1U << 0,
        breakdownPending = 1U << 1,
        brokenDown = 1U << 2,
        journeyStarted = 1U << 3, // The journey start meta data has been filled in
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(BreakdownFlags);

    enum class VehicleFlags : uint16_t // commands?
    {
        none = 0U,
        unk_0 = 1U << 0,
        commandStop = 1U << 1, // commanded to stop??
        unk_2 = 1U << 2,
        sorted = 1U << 3, // vehicle list
        unk_5 = 1U << 5,
        manualControl = 1U << 6,
        shuntCheat = 1U << 7,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(VehicleFlags);

    enum class VehicleEntityType : uint8_t
    {
        head = 0,
        vehicle_1,
        vehicle_2,
        bogie,
        body_start,
        body_continued,
        tail,
    };

    struct VehicleStatus
    {
        StringId status1;
        uint32_t status1Args;
        StringId status2;
        uint32_t status2Args;
    };

    constexpr uint8_t kAirportMovementNodeNull = 0xFF;
    constexpr uint8_t kAirportMovementNoValidEdge = 0xFE;

    struct TrackAndDirection
    {
        struct _TrackAndDirection
        {
            uint16_t _data;
            constexpr _TrackAndDirection(uint8_t id, uint8_t direction)
                : _data((id << 3) | direction)
            {
            }
            constexpr uint8_t id() const { return (_data >> 3) & 0x3F; }
            constexpr uint8_t cardinalDirection() const { return _data & 0x3; }
            constexpr bool isReversed() const { return _data & (1 << 2); }
            constexpr void setReversed(bool state)
            {
                _data &= ~(1 << 2);
                _data |= state ? (1 << 2) : 0;
            }
            constexpr bool operator==(const _TrackAndDirection other) const { return _data == other._data; }
        };
        struct _RoadAndDirection
        {
            uint16_t _data;
            constexpr _RoadAndDirection(uint8_t id, uint8_t direction)
                : _data((id << 3) | direction)
            {
            }
            constexpr uint8_t id() const { return (_data >> 3) & 0xF; }
            constexpr uint8_t cardinalDirection() const { return _data & 0x3; }
            // Used by road and tram vehicles to indicate side
            constexpr bool isReversed() const { return _data & (1 << 2); }
            constexpr void setReversed(bool state)
            {
                _data &= ~(1 << 2);
                _data |= state ? (1 << 2) : 0;
            }

            constexpr uint8_t basicRad() const { return _data & 0x7F; }
            // Vehicles can be in overtaking lane (trams can stay in that lane)
            constexpr bool isOvertaking() const { return _data & (1 << 7); }
            // Related to road vehicles turning around
            constexpr bool isChangingLane() const { return _data & (1 << 8); }
            constexpr bool operator==(const _RoadAndDirection other) const { return _data == other._data; }
        };

        union
        {
            _TrackAndDirection track;
            _RoadAndDirection road;
        };

        constexpr TrackAndDirection(uint8_t id, uint8_t direction)
            : track(id, direction)
        {
        }
    };
    static_assert(sizeof(TrackAndDirection) == 2);

    // TODO move to a different header
    enum class SignalStateFlags : uint8_t
    {
        none = 0U,
        occupied = 1U << 0,       // Signal occupied with a vehicle (can be one or two way)
        blockedNoRoute = 1U << 1, // There is no route through the signal at any time (e.g. one way signal and we are going the wrong way)
        occupiedOneWay = 1U << 2, // Signal occupied with a vehicle and signal is one way
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(SignalStateFlags);

    constexpr uint8_t getMovementNibble(const World::Pos3& pos1, const World::Pos3& pos2)
    {
        uint8_t nibble = 0;
        if (pos1.x != pos2.x)
        {
            nibble |= (1U << 0);
        }
        if (pos1.y != pos2.y)
        {
            nibble |= (1U << 1);
        }
        if (pos1.z != pos2.z)
        {
            nibble |= (1U << 2);
        }
        return nibble;
    }

    // 0x00500120
    constexpr std::array<uint32_t, 8> kMovementNibbleToDistance = {
        0,
        0x220C,
        0x220C,
        0x3027,
        0x199A,
        0x2A99,
        0x2A99,
        0x3689,
    };

    // 0x00500244
    constexpr std::array<World::TilePos2, 9> kMooreNeighbourhood = {
        World::TilePos2{ 0, 0 },
        World::TilePos2{ 0, 1 },
        World::TilePos2{ 1, 1 },
        World::TilePos2{ 1, 0 },
        World::TilePos2{ 1, -1 },
        World::TilePos2{ 0, -1 },
        World::TilePos2{ -1, -1 },
        World::TilePos2{ -1, 0 },
        World::TilePos2{ -1, 1 },
    };

    void setSignalState(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint8_t trackType, uint32_t flags);
    SignalStateFlags getSignalState(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint8_t trackType, uint32_t flags);
    void sub_4A2AD7(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType);
    void setReverseSignalOccupiedInBlock(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType);
    bool isBlockOccupied(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType);
    uint8_t sub_4A2A58(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType);
    uint8_t sub_4A2A77(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType);
    struct ApplyTrackModsResult
    {
        currency32_t cost;
        bool networkTooComplex;
        bool allPlacementsFailed;
    };
    ApplyTrackModsResult applyTrackModsToTrackNetwork(const World::Pos3& pos, Vehicles::TrackAndDirection::_TrackAndDirection trackAndDirection, CompanyId company, uint8_t trackType, uint8_t flags, World::Track::ModSection modSelection, uint8_t trackModObjIds);
    currency32_t removeTrackModsToTrackNetwork(const World::Pos3& pos, Vehicles::TrackAndDirection::_TrackAndDirection trackAndDirection, CompanyId company, uint8_t trackType, uint8_t flags, World::Track::ModSection modSelection, uint8_t trackModObjIds);
    ApplyTrackModsResult applyRoadModsToTrackNetwork(const World::Pos3& pos, Vehicles::TrackAndDirection::_RoadAndDirection roadAndDirection, CompanyId company, uint8_t roadType, uint8_t flags, World::Track::ModSection modSelection, uint8_t roadModObjIds);
    currency32_t removeRoadModsToTrackNetwork(const World::Pos3& pos, Vehicles::TrackAndDirection::_RoadAndDirection roadAndDirection, CompanyId company, uint8_t roadType, uint8_t flags, World::Track::ModSection modSelection, uint8_t roadModObjIds);
    void leaveLevelCrossing(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint16_t unk);

    enum class RoadOccupationFlags : uint8_t
    {
        none = 0U,
        isLaneOccupied = 1U << 0,
        isLevelCrossingClosed = 1U << 1,
        hasLevelCrossing = 1U << 2,
        hasStation = 1U << 3,
        isOneWay = 1U << 4,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(RoadOccupationFlags);
    RoadOccupationFlags getRoadOccupation(const World::Pos3 pos, const TrackAndDirection::_RoadAndDirection tad);

    EntityId checkForCollisions(VehicleBogie& bogie, World::Pos3& loc);
    void playPickupSound(Vehicles::Vehicle2* veh2);
    void playPlacedownSound(const World::Pos3 pos);

    struct UpdateMotionResult
    {
        int32_t remainingDistance;
        UpdateVar1136114Flags flags; // 0x01136114
        EntityId collidedEntityId;   // 0x0113610E

        constexpr bool hasFlags(UpdateVar1136114Flags f) const { return (flags & f) != UpdateVar1136114Flags::none; }
    };

    struct VehicleBase : EntityBase
    {
        static constexpr auto kBaseType = EntityBaseType::vehicle;
        VehicleEntityType subType;
        TransportMode mode;
        VehicleFlags vehicleFlags;
        EntityId head;
        TrackAndDirection trackAndDirection;
        uint16_t subPosition;
        int16_t tileX;
        int16_t tileY;
        World::SmallZ tileBaseZ;
        uint8_t trackType;
        int32_t remainingDistance;
        RoutingHandle routingHandle;
        EntityId nextCarId;
        Flags38 var_38;

    private:
        template<VehicleEntityType SubType>
        bool is() const
        {
            return subType == SubType;
        }

        template<typename TType, VehicleEntityType TClass>
        TType* as() const
        {
            // This can not use reinterpret_cast due to being a const member without considerable more code
            if (!is<TClass>())
            {
                throw Exception::RuntimeError("Malformed vehicle. Incorrect subType!");
            }
            return (TType*)this;
        }

        template<typename TType>
        TType* as() const
        {
            return as<TType, TType::kVehicleThingType>();
        }

    public:
        VehicleEntityType getSubType() const { return subType; }
        void setSubType(const VehicleEntityType newType) { subType = newType; }
        bool isVehicleHead() const { return is<VehicleEntityType::head>(); }
        VehicleHead* asVehicleHead() const { return as<VehicleHead>(); }
        bool isVehicle1() const { return is<VehicleEntityType::vehicle_1>(); }
        Vehicle1* asVehicle1() const { return as<Vehicle1>(); }
        bool isVehicle2() const { return is<VehicleEntityType::vehicle_2>(); }
        Vehicle2* asVehicle2() const { return as<Vehicle2>(); }
        bool isVehicleBogie() const { return is<VehicleEntityType::bogie>(); }
        VehicleBogie* asVehicleBogie() const { return as<VehicleBogie>(); }
        bool isVehicleBody() const { return is<VehicleEntityType::body_start>() || is<VehicleEntityType::body_continued>(); }
        VehicleBody* asVehicleBody() const
        {
            if (is<VehicleEntityType::body_start>())
            {
                return as<VehicleBody, VehicleEntityType::body_start>();
            }

            return as<VehicleBody, VehicleEntityType::body_continued>();
        }
        bool hasSoundPlayer() { return is<VehicleEntityType::vehicle_2>() || is<VehicleEntityType::tail>(); }
        VehicleSound* getVehicleSound();
        bool isVehicleTail() const { return is<VehicleEntityType::tail>(); }
        VehicleTail* asVehicleTail() const { return as<VehicleTail>(); }
        TransportMode getTransportMode() const;
        Flags38 getFlags38() const;
        uint8_t getTrackType() const;
        World::Pos3 getTrackLoc() const;
        TrackAndDirection getTrackAndDirection() const;
        RoutingHandle getRoutingHandle() const;
        EntityId getHead() const;
        int32_t getRemainingDistance() const;
        void setNextCar(const EntityId newNextCar);
        EntityId getNextCar() const;
        bool has38Flags(Flags38 flagsToTest) const;
        bool hasVehicleFlags(VehicleFlags flagsToTest) const;
        VehicleBase* nextVehicle();
        VehicleBase* nextVehicleComponent();
        VehicleBase* previousVehicleComponent();
        void explodeComponent();
        void destroyTrain();
        uint8_t sub_47D959(const World::Pos3& loc, const TrackAndDirection::_RoadAndDirection rad, const bool setOccupied);
        UpdateMotionResult updateTrackMotion(int32_t unk1, bool isVeh2UnkM15);
    };
    static_assert(sizeof(VehicleBase) <= sizeof(Entity));

    struct VehicleSound
    {
        SoundObjectId_t drivingSoundId;
        uint8_t drivingSoundVolume;     // channel attribute volume related
        uint16_t drivingSoundFrequency; // channel attribute frequency
        uint16_t objectId;              // vehicle object (used for sound)
        SoundFlags soundFlags;
        Ui::WindowNumber_t soundWindowNumber;
        Ui::WindowType soundWindowType;
    };

    struct VehicleCargo
    {
        uint32_t acceptedTypes;
        uint8_t type;
        uint8_t maxQty;
        StationId townFrom;
        uint8_t numDays;
        uint8_t qty;
    };

    struct Sub4ACEE7Result
    {
        uint8_t status;
        uint8_t flags;
        StationId stationId;
    };

    struct AirplaneApproachTargetParams
    {
        uint16_t targetZ = 0U;                    // 0x01136168
        uint32_t manhattanDistanceToStation = 0U; // 0x011360D0
        uint8_t targetYaw = 0U;                   // 0x0113646D
        bool isHeliTakeOffEnd = false;            // 0x00525BB0
    };

    struct CarUpdateState
    {
        VehicleBogie* frontBogie; // 0x01136124
        VehicleBogie* backBogie;  // 0x01136128
        bool hasBogieMoved;       // 0x01136237 has either of the bogies moved this tick
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
        void sub_4AD93A();
        void sub_4ADB47(bool unk);
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
        Sub4ACEE7Result sub_4ACEE7(uint32_t unk1, uint32_t var_113612C, bool isPlaceDown);

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
        bool sub_4A8CB6();
        bool sub_4A8C81();
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
        AirplaneApproachTargetParams sub_427122();
        std::pair<AirportMovementNodeFlags, World::Pos3> airportGetMovementEdgeTarget(StationId targetStation, uint8_t curEdge);
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
        Status sub_427BF2();
        void produceLeavingDockSound();
        void produceTouchdownAirportSound();
        uint8_t sub_4AA36A();
        bool sub_4AC1C2();
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

    struct VehicleBody : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::body_continued;
        ColourScheme colourScheme;
        uint8_t objectSpriteType;
        uint16_t objectId;
        int16_t var_44;
        uint8_t animationFrame; //  roll/animation sprite index
        uint8_t cargoFrame;     //  cargo sprite index
        VehicleCargo primaryCargo;
        uint8_t bodyIndex;
        int8_t chuffSoundIndex;
        uint32_t creationDay;
        uint32_t var_5A;
        uint8_t wheelSlipping; // timeout that counts up
        BreakdownFlags breakdownFlags;
        uint32_t refundCost;
        uint8_t breakdownTimeout; // (likely unused)

        const VehicleObject* getObject() const;
        bool update(const CarUpdateState& carState);
        void secondaryAnimationUpdate(const Vehicle& train, const CarUpdateState& carState, const int32_t unkDistance);
        void updateSegmentCrashed(const CarUpdateState& carState);
        void sub_4AAB0B(const CarUpdateState& carState, const int32_t unkDistance);
        void updateCargoSprite();
        constexpr bool hasBreakdownFlags(BreakdownFlags flagsToTest) const
        {
            return (breakdownFlags & flagsToTest) != BreakdownFlags::none;
        }
        void sub_4AC255(VehicleBogie* backBogie, VehicleBogie* frontBogie);

    private:
        void animationUpdate(const CarUpdateState& carState, const int32_t unkDistance);
        void steamPuffsAnimationUpdate(const Vehicle& train, const CarUpdateState& carState, const int32_t unkDistance, uint8_t num, int32_t var_05);
        void dieselExhaust1AnimationUpdate(const Vehicle& train, const CarUpdateState& carState, uint8_t num, int32_t var_05);
        void dieselExhaust2AnimationUpdate(const Vehicle& train, const CarUpdateState& carState, uint8_t num, int32_t var_05);
        void electricSpark1AnimationUpdate(const Vehicle& train, const CarUpdateState& carState, const int32_t unkDistance, uint8_t num, int32_t var_05);
        void electricSpark2AnimationUpdate(const Vehicle& train, const CarUpdateState& carState, const int32_t unkDistance, uint8_t num, int32_t var_05);
        void shipWakeAnimationUpdate(const Vehicle& train, uint8_t num, int32_t var_05);
        Pitch updateSpritePitchSteepSlopes(uint16_t xyOffset, int16_t zOffset);
        Pitch updateSpritePitch(uint16_t xyOffset, int16_t zOffset);
    };
    static_assert(sizeof(VehicleBody) <= sizeof(Entity));

    uint8_t calculateYaw0FromVector(int16_t xDiff, int16_t yDiff);
    uint8_t calculateYaw1FromVectorPlane(int16_t xDiff, int16_t yDiff);
    uint8_t calculateYaw1FromVector(int16_t xDiff, int16_t yDiff);
    uint8_t calculateYaw4FromVector(int16_t xOffset, int16_t yOffset);

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

    void sub_4BA873(VehicleBogie& vehBogie);

    struct VehicleTail : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::tail;
        VehicleSound sound;
        uint16_t trainDanglingTimeout; // counts up when no cars on train

        bool update();
    };
    static_assert(sizeof(VehicleTail) <= sizeof(Entity));

    void liftUpTail(VehicleTail& tail);

    struct CarComponent
    {
        VehicleBogie* front = nullptr;
        VehicleBogie* back = nullptr;
        VehicleBody* body = nullptr;
        CarComponent(VehicleBase*& component);
        CarComponent() = default;

        template<typename TFunc>
        void applyToComponents(TFunc&& func) const
        {
            func(*front);
            func(*back);
            func(*body);
        }
    };

    struct Car : public CarComponent
    {
        class CarComponentIter
        {
        private:
            CarComponent current;
            VehicleBase* nextVehicleComponent = nullptr;

        public:
            CarComponentIter(const CarComponent* carComponent)
            {
                if (carComponent == nullptr)
                {
                    nextVehicleComponent = nullptr;
                    return;
                }
                current = *carComponent;
                nextVehicleComponent = current.body->nextVehicleComponent();
            }

            CarComponentIter& operator++()
            {
                if (nextVehicleComponent == nullptr)
                {
                    return *this;
                }
                if (nextVehicleComponent->getSubType() == VehicleEntityType::tail)
                {
                    nextVehicleComponent = nullptr;
                    return *this;
                }
                CarComponent next{ nextVehicleComponent };
                if (next.body == nullptr || next.body->getSubType() == VehicleEntityType::body_start)
                {
                    nextVehicleComponent = nullptr;
                    return *this;
                }
                current = next;
                return *this;
            }

            CarComponentIter operator++(int)
            {
                CarComponentIter retval = *this;
                ++(*this);
                return retval;
            }

            bool operator==(CarComponentIter other) const
            {
                return nextVehicleComponent == other.nextVehicleComponent;
            }

            constexpr CarComponent& operator*()
            {
                return current;
            }
            // iterator traits
            using difference_type = std::ptrdiff_t;
            using value_type = CarComponent;
            using pointer = CarComponent*;
            using reference = CarComponent&;
            using iterator_category = std::forward_iterator_tag;
        };

        CarComponentIter begin() const
        {
            return CarComponentIter(this);
        }
        CarComponentIter end() const
        {
            return CarComponentIter(nullptr);
        }

        Car(VehicleBase*& component)
            : CarComponent(component)
        {
        }
        Car() = default;

        template<typename TFunc>
        void applyToComponents(TFunc&& func) const
        {
            for (auto& carComponent : *this)
            {
                carComponent.applyToComponents(func);
            }
        }
    };

    struct Vehicle
    {
        struct Cars
        {
            Car firstCar;
            class CarIter
            {
            private:
                Car current;
                VehicleBase* nextVehicleComponent = nullptr;

            public:
                CarIter(const Car* carComponent)
                {
                    if (carComponent == nullptr || carComponent->body == nullptr)
                    {
                        nextVehicleComponent = nullptr;
                        return;
                    }
                    current = *carComponent;
                    nextVehicleComponent = current.body->nextVehicleComponent();
                }

                CarIter& operator++()
                {
                    if (nextVehicleComponent == nullptr)
                    {
                        return *this;
                    }
                    while (nextVehicleComponent->getSubType() != VehicleEntityType::tail)
                    {
                        Car next{ nextVehicleComponent };
                        if (next.body == nullptr)
                        {
                            break;
                        }
                        if (next.body->getSubType() == VehicleEntityType::body_start)
                        {
                            current = next;
                            return *this;
                        }
                    }
                    nextVehicleComponent = nullptr;
                    return *this;
                }

                CarIter operator++(int)
                {
                    CarIter retval = *this;
                    ++(*this);
                    return retval;
                }

                bool operator==(CarIter other) const
                {
                    return nextVehicleComponent == other.nextVehicleComponent;
                }

                constexpr Car& operator*()
                {
                    return current;
                }
                // iterator traits
                using difference_type = std::ptrdiff_t;
                using value_type = Car;
                using pointer = Car*;
                using reference = Car&;
                using iterator_category = std::forward_iterator_tag;
            };

            CarIter begin() const
            {
                return CarIter(&firstCar);
            }
            CarIter end() const
            {
                return CarIter(nullptr);
            }

            std::size_t size() const
            {
                if (firstCar.body == nullptr)
                {
                    return 0;
                }
                return std::distance(begin(), end());
            }

            bool empty() const
            {
                if (firstCar.body == nullptr)
                {
                    return true;
                }
                return false;
            }

            Cars(Car&& _firstCar)
                : firstCar(_firstCar)
            {
            }
            Cars() = default;

            template<typename TFunc>
            void applyToComponents(TFunc&& func) const
            {
                for (auto& car : *this)
                {
                    car.applyToComponents(func);
                }
            }
        };

        VehicleHead* head;
        Vehicle1* veh1;
        Vehicle2* veh2;
        VehicleTail* tail;
        Cars cars;

        Vehicle(const VehicleHead& _head)
            : Vehicle(_head.id)
        {
        }
        Vehicle(EntityId _head);

        // Call if the cars order may have changed
        void refreshCars();

        template<typename TFunc>
        void applyToComponents(TFunc&& func) const
        {
            func(*head);
            func(*veh1);
            func(*veh2);
            cars.applyToComponents(func);
            func(*tail);
        }
    };

    // TODO: move this?
    uint32_t getNumUnitsForCargo(uint32_t maxPrimaryCargo, uint8_t primaryCargoId, uint8_t newCargoId);
    void removeAllCargo(CarComponent& carComponent);

    /* flipCar
     * Reverses a Car in-place and returns the new front bogie
     * frontBogie: front bogie of the Car
     * returns new front bogie of the Car
     */
    VehicleBogie* flipCar(VehicleBogie& frontBogie);

    /* insertCarBefore
     * Takes source vehicle out of its train and puts it in front of the destination vehicle in the destination train.
     * Source and destination trains can be the same.
     * source: front bogie of the Car to move
     * dest: VehicleBogie or VehicleTail to place Car before
     * returns nothing
     */
    void insertCarBefore(VehicleBogie& source, VehicleBase& dest);

    bool canVehiclesCouple(const uint16_t newVehicleTypeId, const uint16_t sourceVehicleTypeId);
    void connectJacobsBogies(VehicleHead& head);

    void applyVehicleObjectLength(Vehicle& train);
    bool positionVehicleOnTrack(VehicleHead& head, const bool isPlaceDown);
}
