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

    bool hasUpdateVar1136114Flags(UpdateVar1136114Flags flags);
    void resetUpdateVar1136114Flags();
    void setUpdateVar1136114Flags(UpdateVar1136114Flags flags);
    void unsetUpdateVar1136114Flags(UpdateVar1136114Flags flags);

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

    struct VehicleSoundPlayer;

    enum class BreakdownFlags : uint8_t
    {
        none = 0U,
        unk_0 = 1U << 0,
        breakdownPending = 1U << 1,
        brokenDown = 1U << 2,
        journeyStarted = 1U << 3, // The journey start meta data has been filled in
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(BreakdownFlags);

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

#pragma pack(push, 1)
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

    struct VehicleBase : EntityBase
    {
        static constexpr auto kBaseType = EntityBaseType::vehicle;

    private:
        template<VehicleEntityType SubType>
        bool is() const
        {
            return getSubType() == SubType;
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
        VehicleEntityType getSubType() const { return VehicleEntityType(EntityBase::getSubType()); }
        void setSubType(const VehicleEntityType newType) { EntityBase::setSubType(static_cast<uint8_t>(newType)); }
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
        VehicleSoundPlayer* getSoundPlayer() const
        {
            if (is<VehicleEntityType::vehicle_2>())
            {
                return as<VehicleSoundPlayer, VehicleEntityType::vehicle_2>();
            }

            return as<VehicleSoundPlayer, VehicleEntityType::tail>();
        }
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
        ColourScheme getColourScheme();
        void setColourScheme(ColourScheme colourScheme);
        bool updateComponent();
        void explodeComponent();
        void destroyTrain();
        uint8_t sub_47D959(const World::Pos3& loc, const TrackAndDirection::_RoadAndDirection trackAndDirection, const bool setOccupied);
        int32_t updateTrackMotion(int32_t unk1);
    };

    struct VehicleSoundPlayer : VehicleBase
    {
        uint8_t pad_24[0x44 - 0x24];
        SoundObjectId_t drivingSoundId;       // 0x44
        uint8_t drivingSoundVolume;           // 0x45 channel attribute volume related
        uint16_t drivingSoundFrequency;       // 0x46 channel attribute frequency
        uint16_t objectId;                    // 0x48 vehicle object (used for sound)
        SoundFlags soundFlags;                // 0x4A
        Ui::WindowNumber_t soundWindowNumber; // 0x4C
        Ui::WindowType soundWindowType;       // 0x4E
        uint8_t pad_4F[0x56 - 0x4F];
        uint32_t var_56;
        uint8_t pad_5A[0x73 - 0x5A];
        uint8_t var_73;
    };
    static_assert(sizeof(VehicleSoundPlayer) == 0x74); // Can't use offset_of change this to last field if more found

    struct VehicleCargo
    {
        uint32_t acceptedTypes; // 0x48
        uint8_t type;           // 0x4C
        uint8_t maxQty;         // 0x4D
        StationId townFrom;     // 0x4E
        uint8_t numDays;        // 0x50
        uint8_t qty;            // 0x51
    };
    static_assert(sizeof(VehicleCargo) == 0xA);

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

    struct VehicleHead : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::head;
        uint8_t pad_24[0x26 - 0x24];
        EntityId head;                       // 0x26
        int32_t remainingDistance;           // 0x28
        TrackAndDirection trackAndDirection; // 0x2C
        uint16_t subPosition;                // 0x2E
        int16_t tileX;                       // 0x30
        int16_t tileY;                       // 0x32
        World::SmallZ tileBaseZ;             // 0x34
        uint8_t trackType;                   // 0x35 field same in all vehicles
        RoutingHandle routingHandle;         // 0x36 field same in all vehicles orderId * maxNumRoutingSteps
        Flags38 var_38;
        uint8_t pad_39;      // 0x39
        EntityId nextCarId;  // 0x3A
        int32_t var_3C;      // 0x3C
        uint8_t pad_40[0x2]; // 0x40
        TransportMode mode;  // 0x42 field same in all vehicles
        uint8_t pad_43;
        int16_t ordinalNumber;            // 0x44
        uint32_t orderTableOffset;        // 0x46 offset into Order Table
        uint16_t currentOrder;            // 0x4A offset, combine with orderTableOffset
        uint16_t sizeOfOrderTable;        // 0x4C size of Order Table
        uint32_t trainAcceptedCargoTypes; // 0x4E
        uint8_t var_52;
        uint8_t var_53;                // 0x53 mods?
        StationId stationId;           // 0x54
        uint16_t cargoTransferTimeout; // 0x56
        uint32_t var_58;
        uint8_t var_5C;
        Status status;                     // 0x5D
        VehicleType vehicleType;           // 0x5E
        BreakdownFlags breakdownFlags;     // 0x5F
        uint8_t aiThoughtId;               // 0x60 0xFFU for null
        World::Pos2 aiPlacementPos;        // 0x61
        uint16_t aiPlacementTaD;           // 0x65 for air/water this is just rotation
        uint8_t aiPlacementBaseZ;          // 0x67
        uint8_t airportMovementEdge;       // 0x68
        uint32_t totalRefundCost;          // 0x69
        uint8_t crashedTimeout;            // 0x6D
        int8_t manualPower;                // 0x6E manual power control VehicleFlags::manualControl
        World::Pos2 journeyStartPos;       // 0x6F journey start position
        uint32_t journeyStartTicks;        // 0x73 ticks since journey start
        Speed16 lastAverageSpeed;          // 0x77
        uint8_t restartStoppedCarsTimeout; // 0x79 timeout before auto starting trams/buses

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
        void updateSegmentCrashed();
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
        void updateDrivingSound(VehicleSoundPlayer* soundPlayer);
        void updateDrivingSoundNone(VehicleSoundPlayer* soundPlayer);
        void updateDrivingSoundFriction(VehicleSoundPlayer* soundPlayer, const VehicleObjectFrictionSound* snd);
        void updateSimpleMotorSound(VehicleSoundPlayer* soundPlayer, const VehicleSimpleMotorSound* snd);
        void updateGearboxMotorSound(VehicleSoundPlayer* soundPlayer, const VehicleGearboxMotorSound* snd);
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
    static_assert(sizeof(VehicleHead) == 0x7A); // Can't use offset_of change this to last field if more found

    struct IncomeStats
    {
        int32_t day;                  // 0x53
        uint8_t cargoTypes[4];        // 0x57
        uint16_t cargoQtys[4];        // 0x5B
        uint16_t cargoDistances[4];   // 0x63
        uint8_t cargoAges[4];         // 0x6B
        currency32_t cargoProfits[4]; // 0x6F
        void beginNewIncome();
        bool addToStats(uint8_t cargoType, uint16_t cargoQty, uint16_t cargoDist, uint8_t cargoAge, currency32_t profit);
    };
    static_assert(sizeof(IncomeStats) == 0x2C);

    struct Vehicle1 : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::vehicle_1;
        uint8_t pad_24[0x26 - 0x24];
        EntityId head;                       // 0x26
        int32_t remainingDistance;           // 0x28
        TrackAndDirection trackAndDirection; // 0x2C
        uint16_t subPosition;                // 0x2E
        int16_t tileX;                       // 0x30
        int16_t tileY;                       // 0x32
        World::SmallZ tileBaseZ;             // 0x34
        uint8_t trackType;                   // 0x35 field same in all vehicles
        RoutingHandle routingHandle;         // 0x36 field same in all vehicles
        Flags38 var_38;
        uint8_t pad_39;      // 0x39
        EntityId nextCarId;  // 0x3A
        int32_t var_3C;      // 0x3C
        uint8_t pad_40[0x2]; // 0x40
        TransportMode mode;  // 0x42 field same in all vehicles
        uint8_t pad_43;
        Speed16 targetSpeed;   // 0x44
        uint16_t timeAtSignal; // 0x46
        Flags48 var_48;
        uint8_t var_49;      // 0x49 rackrail mod?
        uint32_t dayCreated; // 0x4A
        uint16_t var_4E;
        uint16_t var_50;
        uint8_t var_52;
        IncomeStats lastIncome; // 0x53

        bool update();
        bool updateRoad();
        bool updateRail();
        int32_t updateRoadMotion(int32_t distance);
    };
    static_assert(sizeof(Vehicle1) == 0x7F); // Can't use offset_of change this to last field if more found

    struct Vehicle2 : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::vehicle_2;
        uint8_t pad_24[0x26 - 0x24];
        EntityId head;                       // 0x26
        int32_t remainingDistance;           // 0x28
        TrackAndDirection trackAndDirection; // 0x2C
        uint16_t subPosition;                // 0x2E
        int16_t tileX;                       // 0x30
        int16_t tileY;                       // 0x32
        World::SmallZ tileBaseZ;             // 0x34
        uint8_t trackType;                   // 0x35 field same in all vehicles
        RoutingHandle routingHandle;         // 0x36 field same in all vehicles
        Flags38 var_38;
        uint8_t pad_39;              // 0x39
        EntityId nextCarId;          // 0x3A
        int32_t var_3C;              // 0x3C field same in all vehicles unread for veh2
        uint8_t pad_40[0x42 - 0x40]; // 0x40
        TransportMode mode;          // 0x42 field same in all vehicles
        uint8_t pad_43;
        SoundObjectId_t drivingSoundId;       // 0x44
        uint8_t drivingSoundVolume;           // 0x45 channel attribute volume related
        uint16_t drivingSoundFrequency;       // 0x46 channel attribute frequency
        uint16_t objectId;                    // 0x48 vehicle object (used for sound)
        SoundFlags soundFlags;                // 0x4A common with tail
        Ui::WindowNumber_t soundWindowNumber; // 0x4C common with tail
        Ui::WindowType soundWindowType;       // 0x4E common with tail
        int8_t var_4F;
        uint16_t totalPower;  // 0x50 maybe not used by aircraft and ship
        uint16_t totalWeight; // 0x52
        Speed16 maxSpeed;     // 0x54
        Speed32 currentSpeed; // 0x56
        MotorState motorState;
        uint8_t brakeLightTimeout;
        Speed16 rackRailMaxSpeed;     // 0x5C
        currency32_t curMonthRevenue; // 0x5E monthly revenue
        currency32_t profit[4];       // 0x62 last 4 months net profit
        uint8_t reliability;          // 0x72
        Flags73 var_73;               // 0x73 (bit 0 = broken down, bit 1 = still powered)

        bool has73Flags(Flags73 flagsToTest) const;

        bool update();
        bool sub_4A9F20();
        currency32_t totalRecentProfit() const
        {
            return profit[0] + profit[1] + profit[2] + profit[3];
        }
    };
    static_assert(sizeof(Vehicle2) == 0x74); // Can't use offset_of change this to last field if more found

    struct VehicleBody : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::body_continued;
        ColourScheme colourScheme;           // 0x24
        EntityId head;                       // 0x26
        int32_t remainingDistance;           // 0x28
        TrackAndDirection trackAndDirection; // 0x2C
        uint16_t subPosition;                // 0x2E
        int16_t tileX;                       // 0x30
        int16_t tileY;                       // 0x32
        World::SmallZ tileBaseZ;             // 0x34
        uint8_t trackType;                   // 0x35 field same in all vehicles
        RoutingHandle routingHandle;         // 0x36 field same in all vehicles
        Flags38 var_38;
        uint8_t objectSpriteType; // 0x39
        EntityId nextCarId;       // 0x3A
        int32_t var_3C;           // 0x3C field same in all vehicles unread for body
        uint16_t objectId;        // 0x40
        TransportMode mode;       // 0x42
        uint8_t pad_43;
        int16_t var_44;
        uint8_t animationFrame;    // 0x46 roll/animation sprite index
        uint8_t cargoFrame;        // 0x47 cargo sprite index
        VehicleCargo primaryCargo; // 0x48
        uint8_t pad_52[0x54 - 0x52];
        uint8_t bodyIndex; // 0x54
        int8_t chuffSoundIndex;
        uint32_t creationDay; // 0x56
        uint32_t var_5A;
        uint8_t wheelSlipping;         // 0x5E timeout that counts up
        BreakdownFlags breakdownFlags; // 0x5F
        uint16_t pad_60;               // 0x60
        uint32_t refundCost;           // 0x62
        uint8_t pad_66[0x6A - 0x66];
        uint8_t breakdownTimeout; // 0x6A (likely unused)

        const VehicleObject* getObject() const;
        bool update();
        void secondaryAnimationUpdate();
        void updateSegmentCrashed();
        void sub_4AAB0B();
        void updateCargoSprite();
        constexpr bool hasBreakdownFlags(BreakdownFlags flagsToTest) const
        {
            return (breakdownFlags & flagsToTest) != BreakdownFlags::none;
        }
        void sub_4AC255(VehicleBogie* backBogie, VehicleBogie* frontBogie);

    private:
        void animationUpdate();
        void steamPuffsAnimationUpdate(uint8_t num, int32_t var_05);
        void dieselExhaust1AnimationUpdate(uint8_t num, int32_t var_05);
        void dieselExhaust2AnimationUpdate(uint8_t num, int32_t var_05);
        void electricSpark1AnimationUpdate(uint8_t num, int32_t var_05);
        void electricSpark2AnimationUpdate(uint8_t num, int32_t var_05);
        void shipWakeAnimationUpdate(uint8_t num, int32_t var_05);
        Pitch updateSpritePitchSteepSlopes(uint16_t xyOffset, int16_t zOffset);
        Pitch updateSpritePitch(uint16_t xyOffset, int16_t zOffset);
    };
    static_assert(sizeof(VehicleBody) == 0x6B); // Can't use offset_of change this to last field if more found

    uint8_t calculateYaw0FromVector(int16_t xDiff, int16_t yDiff);
    uint8_t calculateYaw1FromVectorPlane(int16_t xDiff, int16_t yDiff);
    uint8_t calculateYaw1FromVector(int16_t xDiff, int16_t yDiff);
    uint8_t calculateYaw4FromVector(int16_t xOffset, int16_t yOffset);

    struct VehicleBogie : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::bogie;
        ColourScheme colourScheme;           // 0x24
        EntityId head;                       // 0x26
        int32_t remainingDistance;           // 0x28
        TrackAndDirection trackAndDirection; // 0x2C
        uint16_t subPosition;                // 0x2E
        int16_t tileX;                       // 0x30
        int16_t tileY;                       // 0x32
        World::SmallZ tileBaseZ;             // 0x34
        uint8_t trackType;                   // 0x35 field same in all vehicles
        RoutingHandle routingHandle;         // 0x36 field same in all vehicles
        Flags38 var_38;
        uint8_t objectSpriteType; // 0x39
        EntityId nextCarId;       // 0x3A
        int32_t var_3C;           // 0x3C field same in all vehicles unread for bogie
        uint16_t objectId;        // 0x40
        TransportMode mode;       // 0x42 field same in all vehicles
        uint8_t pad_43;
        uint16_t var_44;
        uint8_t animationIndex;      // 0x46 animation index
        uint8_t var_47;              // 0x47 cargo sprite index (unused)
        VehicleCargo secondaryCargo; // 0x48 Note back bogie cannot carry cargo always check type
        uint16_t totalCarWeight;     // 0x52 only valid for first bogie of car
        uint8_t bodyIndex;           // 0x54
        uint8_t pad_55;
        uint32_t creationDay; // 0x56
        uint32_t var_5A;
        uint8_t wheelSlipping; // 0x5E timeout that counts up
        BreakdownFlags breakdownFlags;
        uint8_t var_60;
        uint8_t var_61;
        uint32_t refundCost;         // 0x62 front bogies only
        uint16_t reliability;        // 0x66 front bogies only
        uint16_t timeoutToBreakdown; // 0x68 front bogies only (days) counts down to the next breakdown 0xFFFFU disables this
        uint8_t breakdownTimeout;    // 0x6A front bogies only (days)

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
        void updateRoll();
        void collision();
    };
    static_assert(sizeof(VehicleBogie) == 0x6B); // Can't use offset_of change this to last field if more found

    void sub_4BA873(VehicleBogie& vehBogie);

    struct VehicleTail : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::tail;
        uint8_t pad_24[0x26 - 0x24];
        EntityId head;                       // 0x26
        int32_t remainingDistance;           // 0x28
        TrackAndDirection trackAndDirection; // 0x2C
        uint16_t subPosition;                // 0x2E
        int16_t tileX;                       // 0x30
        int16_t tileY;                       // 0x32
        World::SmallZ tileBaseZ;             // 0x34
        uint8_t trackType;                   // 0x35 field same in all vehicles
        RoutingHandle routingHandle;         // 0x36 field same in all vehicles
        Flags38 var_38;
        uint8_t pad_39;              // 0x39
        EntityId nextCarId;          // 0x3A
        int32_t var_3C;              // 0x3C field same in all vehicles unread for tail
        uint8_t pad_40[0x42 - 0x40]; // 0x40
        TransportMode mode;          // 0x42 field same in all vehicles
        uint8_t pad_43;
        SoundObjectId_t drivingSoundId;       // 0x44
        uint8_t drivingSoundVolume;           // 0x45 channel attribute volume related
        uint16_t drivingSoundFrequency;       // 0x46 channel attribute frequency
        uint16_t objectId;                    // 0x48 vehicle object (used for sound)
        SoundFlags soundFlags;                // 0x4A common with veh_2
        Ui::WindowNumber_t soundWindowNumber; // 0x4C common with veh_2
        Ui::WindowType soundWindowType;       // 0x4E common with veh_2
        uint16_t trainDanglingTimeout;        // 0x4F counts up when no cars on train

        bool update();
    };
    static_assert(sizeof(VehicleTail) == 0x51); // Can't use offset_of change this to last field if more found

    void liftUpTail(VehicleTail& tail);

#pragma pack(pop)

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

    struct VehicleCommon;

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
