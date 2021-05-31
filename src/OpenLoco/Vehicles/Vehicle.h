#pragma once

#include "../Audio/Audio.h"
#include "../Company.h"
#include "../Entities/Entity.h"
#include "../Objects/VehicleObject.h"
#include "../Speed.hpp"
#include "../Types.hpp"
#include "../Ui/WindowType.h"
#include "../Window.h"

namespace OpenLoco::Vehicles
{
    constexpr auto max_vehicle_length = 176; // TODO: Units?

    void create(OpenLoco::Interop::registers& regs);
    void orderSkip(OpenLoco::Interop::registers& regs);
    void cloneVehicle(OpenLoco::Interop::registers& regs);
    void rename(OpenLoco::Interop::registers& regs);

    namespace Flags0C // commands?
    {
        constexpr uint8_t commandStop = 1 << 1; // commanded to stop??
        constexpr uint8_t sorted = 1 << 3;      // vehicle list
        constexpr uint8_t unk_5 = 1 << 5;
        constexpr uint8_t manualControl = 1 << 6;
    }

    namespace Flags38
    {
        constexpr uint8_t unk_0 = 1 << 0;
        constexpr uint8_t isReversed = 1 << 1;
        constexpr uint8_t unk_3 = 1 << 3;
        constexpr uint8_t isGhost = 1 << 4;
    }

    namespace Flags73 // veh2 Train breakdown flags
    {
        constexpr uint8_t isBrokenDown = 1 << 0;
        constexpr uint8_t isStillPowered = 1 << 1;
    }

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

    struct Vehicle2or6;

    namespace Flags5F
    {
        constexpr uint8_t unk_0 = 1 << 0;
        constexpr uint8_t breakdown_pending = 1 << 1;
        constexpr uint8_t broken_down = 1 << 2;
        constexpr uint8_t unk_3 = 1 << 3;
    }

    enum class VehicleThingType : uint8_t
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
        string_id status1;
        uint32_t status1Args;
        string_id status2;
        uint32_t status2Args;
    };

    constexpr uint8_t cAirportMovementNodeNull = 0xFF;

#pragma pack(push, 1)
    struct VehicleBase : EntityBase
    {
    private:
        template<VehicleThingType SubType>
        bool is() const
        {
            return getSubType() == SubType;
        }

        template<typename TType, VehicleThingType TClass>
        TType* as() const
        {
            // This can not use reinterpret_cast due to being a const member without considerable more code
            if (!is<TClass>())
            {
                throw std::runtime_error("Malformed vehicle. Incorrect subType!");
            }
            return (TType*)this;
        }

        template<typename TType>
        TType* as() const
        {
            return as<TType, TType::vehicleThingType>();
        }

    public:
        VehicleThingType getSubType() const { return VehicleThingType(EntityBase::getSubType()); }
        void setSubType(const VehicleThingType newType) { EntityBase::setSubType(static_cast<uint8_t>(newType)); }
        bool isVehicleHead() const { return is<VehicleThingType::head>(); }
        VehicleHead* asVehicleHead() const { return as<VehicleHead>(); }
        bool isVehicle1() const { return is<VehicleThingType::vehicle_1>(); }
        Vehicle1* asVehicle1() const { return as<Vehicle1>(); }
        bool isVehicle2() const { return is<VehicleThingType::vehicle_2>(); }
        Vehicle2* asVehicle2() const { return as<Vehicle2>(); }
        bool isVehicleBogie() const { return is<VehicleThingType::bogie>(); }
        VehicleBogie* asVehicleBogie() const { return as<VehicleBogie>(); }
        bool isVehicleBody() const { return is<VehicleThingType::body_start>() || is<VehicleThingType::body_continued>(); }
        VehicleBody* asVehicleBody() const
        {
            if (is<VehicleThingType::body_start>())
            {
                return as<VehicleBody, VehicleThingType::body_start>();
            }

            return as<VehicleBody, VehicleThingType::body_continued>();
        }
        bool isVehicle2Or6() { return is<VehicleThingType::vehicle_2>() || is<VehicleThingType::tail>(); }
        Vehicle2or6* asVehicle2Or6() const
        {
            if (is<VehicleThingType::vehicle_2>())
            {
                return as<Vehicle2or6, VehicleThingType::vehicle_2>();
            }

            return as<Vehicle2or6, VehicleThingType::tail>();
        }
        bool isVehicleTail() const { return is<VehicleThingType::tail>(); }
        VehicleTail* asVehicleTail() const { return as<VehicleTail>(); }
        TransportMode getTransportMode() const;
        uint8_t getFlags38() const;
        uint8_t getTrackType() const;
        uint16_t getVar36() const;
        EntityId_t getHead() const;
        void setNextCar(const EntityId_t newNextCar);

        VehicleBase* nextVehicle();
        VehicleBase* nextVehicleComponent();
        bool updateComponent();
        void sub_4AA464();
    };

    struct Vehicle2or6 : VehicleBase
    {
        uint8_t pad_24[0x44 - 0x24];
        SoundObjectId_t drivingSoundId;         // 0x44
        uint8_t drivingSoundVolume;             // 0x45 channel attribute volume related
        uint16_t drivingSoundFrequency;         // 0x46 channel attribute frequency
        uint16_t objectId;                      // 0x48 vehicle object (used for sound)
        uint16_t var_4A;                        // sound-related flag(s)
        Ui::WindowNumber_t sound_window_number; // 0x4C
        Ui::WindowType sound_window_type;       // 0x4E
        uint8_t pad_4F[0x56 - 0x4F];
        uint32_t var_56;
        uint8_t pad_5A[0x73 - 0x5A];
        uint8_t var_73;
    };
    static_assert(sizeof(Vehicle2or6) == 0x74); // Can't use offset_of change this to last field if more found

    struct VehicleCargo
    {
        uint32_t acceptedTypes; // 0x48
        uint8_t type;           // 0x4C
        uint8_t maxQty;         // 0x4D
        StationId_t townFrom;   // 0x4E
        uint8_t numDays;        // 0x50
        uint8_t qty;            // 0x51
    };
    static_assert(sizeof(VehicleCargo) == 0xA);

    struct VehicleHead : VehicleBase
    {
        static constexpr auto vehicleThingType = VehicleThingType::head;
        uint8_t pad_24[0x26 - 0x24];
        EntityId_t head; // 0x26
        uint32_t var_28;
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles orderId * max_num_routing_steps
        uint8_t var_38;
        uint8_t pad_39;         // 0x39
        EntityId_t next_car_id; // 0x3A
        uint32_t var_3C;        // 0x3C
        uint8_t pad_40[0x2];    // 0x40
        TransportMode mode;     // 0x42 field same in all vehicles
        uint8_t pad_43;
        int16_t ordinalNumber;     // 0x44
        uint32_t orderTableOffset; // 0x46 offset into Order Table
        uint16_t currentOrder;     // 0x4A offset, combine with orderTableOffset
        uint16_t sizeOfOrderTable; // 0x4C size of Order Table
        uint32_t var_4E;           // 0x4E
        uint8_t var_52;
        uint8_t pad_53;
        StationId_t stationId;         // 0x54
        uint16_t cargoTransferTimeout; // 0x56
        uint32_t var_58;
        uint8_t var_5C;
        Status status;           // 0x5D
        VehicleType vehicleType; // 0x5E
        uint8_t var_5F;          // 0x5F
        uint8_t var_60;
        uint16_t var_61;
        uint8_t pad_63[0x68 - 0x63];
        uint8_t airportMovementEdge; // 0x68
        uint32_t var_69;
        uint8_t pad_6D;
        int8_t var_6E;             // manual speed/brake
        int16_t var_6F;            // 0x6F x
        int16_t var_71;            // 0x6F y
        uint32_t var_73;           // 0x73 ticks since journey start
        uint16_t lastAverageSpeed; // 0x77
        uint8_t var_79;            // 0x79 timeout before auto starting trams/buses

    public:
        bool isVehicleTypeCompatible(const uint16_t vehicleTypeId);
        void updateBreakdown();
        void updateVehicle();
        bool update();
        VehicleStatus getStatus() const;
        OrderRingView getCurrentOrders() const;
        bool isPlaced() const { return tile_x != -1 && !(var_38 & Flags38::isGhost); }
        char* generateCargoTotalString(char* buffer);
        bool canBeModified() const;
        void liftUpVehicle();
        void sub_4B7CC3();
        currency32_t calculateRunningCost() const;

    private:
        void applyBreakdownToTrain();
        void updateDrivingSounds();
        void updateDrivingSound(Vehicle2or6* vehType2or6);
        void updateDrivingSoundNone(Vehicle2or6* vehType2or6);
        void updateDrivingSoundFriction(Vehicle2or6* vehType2or6, VehicleObjectFrictionSound* snd);
        void updateDrivingSoundEngine1(Vehicle2or6* vehType2or6, VehicleObjectEngine1Sound* snd);
        void updateDrivingSoundEngine2(Vehicle2or6* vehType2or6, VehicleObjectEngine2Sound* snd);
        void removeDanglingTrain();
        bool updateLand();
        bool sub_4A8DB7();
        bool sub_4A8F22();
        bool sub_4A8CB6();
        bool sub_4A8C81();
        bool landTryBeginUnloading();
        bool landLoadingUpdate();
        bool landNormalMovementUpdate();
        bool trainNormalMovementUpdate(uint8_t al, uint8_t flags, StationId_t nextStation);
        bool roadNormalMovementUpdate(uint8_t al, StationId_t nextStation);
        bool landReverseFromSignal();
        bool updateAir();
        bool airplaneLoadingUpdate();
        bool sub_4A95CB();
        bool sub_4A9348(uint8_t newMovementEdge, uint16_t targetZ);
        bool airplaneApproachTarget(uint16_t targetZ);
        std::pair<Status, Speed16> airplaneGetNewStatus();
        uint8_t airportGetNextMovementEdge(uint8_t curEdge);
        std::tuple<uint32_t, uint16_t, uint8_t> sub_427122();
        std::pair<uint32_t, Map::Pos3> airportGetMovementEdgeTarget(StationId_t targetStation, uint8_t curEdge);
        bool updateWater();
        uint32_t getVehicleTotalLength();
        void tryCreateInitialMovementSound();
        void setStationVisitedTypes();
        void checkIfAtOrderStation();
        void updateLastJourneyAverageSpeed();
        void beginUnloading();
        void beginLoading();
        void movePlaneTo(const Map::Pos3& newLoc, const uint8_t newYaw, const Pitch newPitch);
        uint32_t updateWaterMotion(uint32_t flags);
        void moveBoatTo(const Map::Pos3& loc, const uint8_t yaw, const Pitch pitch);
        uint8_t getLoadingModifier(const VehicleBogie* bogie);
        bool updateUnloadCargoComponent(VehicleCargo& cargo, VehicleBogie* bogie);
        void updateUnloadCargo();
        bool updateLoadCargoComponent(VehicleCargo& cargo, VehicleBogie* bogie);
        bool updateLoadCargo();
        void beginNewJourney();
        void advanceToNextRoutableOrder();
        Status sub_427BF2();
        void produceLeavingDockSound();
        std::tuple<StationId_t, Map::Pos2, Map::Pos3> sub_427FC9();
        void produceTouchdownAirportSound();
        uint8_t sub_4AA36A();
        void sub_4AD778();
        void sub_4AA625();
        std::tuple<uint8_t, uint8_t, StationId_t> sub_4ACEE7(uint32_t unk1, uint32_t var_113612C);
        bool sub_4AC1C2();
        bool sub_4AC0A3();
        bool sub_4ACCDC();
        void sub_4AD93A();
        StationId_t manualFindTrainStationAtLocation();
        bool sub_4BADE4();
        bool isOnExpectedRoadOrTrack();
        void sub_4ADB47(bool unk);
        VehicleStatus getStatusTravelling() const;
        void getSecondStatus(VehicleStatus& vehStatus) const;
        void updateLastIncomeStats(uint8_t cargoType, uint16_t cargoQty, uint16_t cargoDist, uint8_t cargoAge, currency32_t profit);
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
        static constexpr auto vehicleThingType = VehicleThingType::vehicle_1;
        uint8_t pad_24[0x26 - 0x24];
        EntityId_t head; // 0x26
        uint32_t var_28;
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t pad_39;         // 0x39
        EntityId_t next_car_id; // 0x3A
        uint32_t var_3C;        // 0x3C
        uint8_t pad_40[0x2];    // 0x40
        TransportMode mode;     // 0x42 field same in all vehicles
        uint8_t pad_43;
        Speed16 var_44;
        uint16_t timeAtSignal; // 0x46
        uint8_t var_48;
        uint8_t var_49;
        uint32_t dayCreated; // 0x4A
        uint16_t var_4E;
        uint16_t var_50;
        uint8_t var_52;
        IncomeStats lastIncome; // 0x53
    };
    static_assert(sizeof(Vehicle1) == 0x7F); // Can't use offset_of change this to last field if more found

    struct Vehicle2 : VehicleBase
    {
        static constexpr auto vehicleThingType = VehicleThingType::vehicle_2;
        uint8_t pad_24[0x26 - 0x24];
        EntityId_t head; // 0x26
        uint32_t var_28;
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t pad_39;              // 0x39
        EntityId_t next_car_id;      // 0x3A
        uint8_t pad_3C[0x42 - 0x3C]; // 0x3C
        TransportMode mode;          // 0x42 field same in all vehicles
        uint8_t pad_43;
        SoundObjectId_t drivingSoundId;         // 0x44
        uint8_t drivingSoundVolume;             // 0x45 channel attribute volume related
        uint16_t drivingSoundFrequency;         // 0x46 channel attribute frequency
        uint16_t objectId;                      // 0x48 vehicle object (used for sound)
        uint16_t var_4A;                        // sound-related flag(s) common with tail
        Ui::WindowNumber_t sound_window_number; // 0x4C common with tail
        Ui::WindowType sound_window_type;       // 0x4E common with tail
        uint8_t pad_4F;
        uint16_t totalPower;  // 0x50 maybe not used by aircraft and ship
        uint16_t totalWeight; // 0x52
        Speed16 maxSpeed;     // 0x54
        Speed32 currentSpeed; // 0x56
        uint8_t var_5A;
        uint8_t var_5B;
        Speed16 rackRailMaxSpeed;    // 0x5C
        currency32_t lifetimeProfit; // 0x5E
        currency32_t profit[4];      // 0x62 last 4 months profit
        uint8_t reliability;         // 0x72
        uint8_t var_73;              // 0x73 (bit 0 = broken down, bit 1 = still powered)

        currency32_t totalRecentProfit() const
        {
            return profit[0] + profit[1] + profit[2] + profit[3];
        }
    };
    static_assert(sizeof(Vehicle2) == 0x74); // Can't use offset_of change this to last field if more found

    struct VehicleBody : VehicleBase
    {
        static constexpr auto vehicleThingType = VehicleThingType::body_continued;
        ColourScheme colour_scheme; // 0x24
        EntityId_t head;            // 0x26
        uint8_t pad_28[0x2C - 0x28];
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t object_sprite_type; // 0x39
        EntityId_t next_car_id;     // 0x3A
        uint8_t pad_3C[0x40 - 0x3C];
        uint16_t object_id; // 0x40
        TransportMode mode; // 0x42
        uint8_t pad_43;
        int16_t var_44;
        uint8_t var_46;            // 0x46 roll/animation sprite index
        uint8_t var_47;            // 0x47 cargo sprite index
        VehicleCargo primaryCargo; // 0x48
        uint8_t pad_52[0x54 - 0x52];
        uint8_t body_index; // 0x54
        int8_t var_55;
        uint32_t creation_day; // 0x56
        uint8_t pad_5A[0x5E - 0x5A];
        uint8_t var_5E;
        uint8_t var_5F;

        VehicleObject* object() const;
        int32_t update();
        void secondaryAnimationUpdate();
        void sub_4AAB0B();
        void updateCargoSprite();

    private:
        void animationUpdate();
        void sub_4AC255(VehicleBogie* back_bogie, VehicleBogie* front_bogie);
        void steamPuffsAnimationUpdate(uint8_t num, int32_t var_05);
        void dieselExhaust1AnimationUpdate(uint8_t num, int32_t var_05);
        void dieselExhaust2AnimationUpdate(uint8_t num, int32_t var_05);
        void electricSpark1AnimationUpdate(uint8_t num, int32_t var_05);
        void electricSpark2AnimationUpdate(uint8_t num, int32_t var_05);
        void shipWakeAnimationUpdate(uint8_t num, int32_t var_05);
        Pitch updateSpritePitchSteepSlopes(uint16_t xy_offset, int16_t z_offset);
        Pitch updateSpritePitch(uint16_t xy_offset, int16_t z_offset);
    };
    static_assert(sizeof(VehicleBody) == 0x60); // Can't use offset_of change this to last field if more found

    uint8_t calculateYaw1FromVectorPlane(int16_t xDiff, int16_t yDiff);
    uint8_t calculateYaw4FromVector(int16_t x_offset, int16_t y_offset);

    struct VehicleBogie : VehicleBase
    {
        static constexpr auto vehicleThingType = VehicleThingType::bogie;
        ColourScheme colour_scheme; // 0x24
        EntityId_t head;            // 0x26
        uint8_t pad_28[0x2C - 0x28];
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t object_sprite_type; // 0x39
        EntityId_t next_car_id;     // 0x3A
        uint8_t pad_3C[0x40 - 0x3C];
        uint16_t object_id; // 0x40
        TransportMode mode; // 0x42 field same in all vehicles
        uint8_t pad_43;
        uint16_t var_44;
        uint8_t var_46; // 0x46 roll
        uint8_t var_47;
        VehicleCargo secondaryCargo; // 0x48 Note back bogie cannot carry cargo always check type
        uint8_t pad_52[0x54 - 0x52];
        uint8_t body_index; // 0x54
        uint8_t pad_55;
        uint32_t creation_day; // 0x56
        uint8_t pad_5A[0x5E - 0x5A];
        uint8_t var_5E;
        uint8_t var_5F;
        uint8_t var_60;
        uint8_t var_61;
        uint32_t refund_cost; // 0x62 front bogies only
        uint16_t reliability; // 0x66 front bogies only
        uint16_t var_68;
        uint8_t var_6A;

    public:
        uint16_t getPlaneType();
    };
    static_assert(sizeof(VehicleBogie) == 0x6B); // Can't use offset_of change this to last field if more found

    struct VehicleTail : VehicleBase
    {
        static constexpr auto vehicleThingType = VehicleThingType::tail;
        uint8_t pad_24[0x26 - 0x24];
        EntityId_t head; // 0x26
        uint32_t var_28;
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t pad_39;              // 0x39
        EntityId_t next_car_id;      // 0x3A
        uint8_t pad_3C[0x42 - 0x3C]; // 0x3C
        TransportMode mode;          // 0x42 field same in all vehicles
        uint8_t pad_43;
        SoundObjectId_t drivingSoundId;         // 0x44
        uint8_t drivingSoundVolume;             // 0x45 channel attribute volume related
        uint16_t drivingSoundFrequency;         // 0x46 channel attribute frequency
        uint16_t objectId;                      // 0x48 vehicle object (used for sound)
        uint16_t var_4A;                        // sound-related flag(s) common with veh_2
        Ui::WindowNumber_t sound_window_number; // 0x4C common with veh_2
        Ui::WindowType sound_window_type;       // 0x4E common with veh_2
        uint16_t trainDanglingTimeout;          // 0x4F counts up when no cars on train
    };
    static_assert(sizeof(VehicleTail) == 0x51); // Can't use offset_of change this to last field if more found

#pragma pack(pop)

    struct CarComponent
    {
        VehicleBogie* front = nullptr;
        VehicleBogie* back = nullptr;
        VehicleBody* body = nullptr;
        CarComponent(VehicleBase*& component);
        CarComponent() = default;
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
                if (nextVehicleComponent->getSubType() == VehicleThingType::tail)
                {
                    nextVehicleComponent = nullptr;
                    return *this;
                }
                CarComponent next{ nextVehicleComponent };
                if (next.body == nullptr || next.body->getSubType() == VehicleThingType::body_start)
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
            bool operator!=(CarComponentIter other) const
            {
                return !(*this == other);
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
                    while (nextVehicleComponent->getSubType() != VehicleThingType::tail)
                    {
                        Car next{ nextVehicleComponent };
                        if (next.body == nullptr)
                        {
                            break;
                        }
                        if (next.body->getSubType() == VehicleThingType::body_start)
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
                bool operator!=(CarIter other) const
                {
                    return !(*this == other);
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
        };

        VehicleHead* head;
        Vehicle1* veh1;
        Vehicle2* veh2;
        VehicleTail* tail;
        Cars cars;

        Vehicle(const VehicleHead* _head)
            : Vehicle(_head->id)
        {
        }
        Vehicle(uint16_t _head);
    };
}
