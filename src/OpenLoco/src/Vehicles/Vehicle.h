#pragma once

#include "Entities/Entity.h"
#include "Map/Track/TrackModSection.h"
#include "Objects/VehicleObject.h"
#include "Routing.h"
#include "Speed.hpp"
#include "Types.hpp"
#include "Ui/Window.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Exception.hpp>

namespace OpenLoco::Vehicles
{
    constexpr auto kMaxRoadVehicleLength = 176;    // TODO: Units?
    constexpr uint8_t kWheelSlippingDuration = 64; // In ticks

    enum class Flags38 : uint8_t
    {
        none = 0U,
        isBody = 1U << 0,
        isReversed = 1U << 1,
        unk_2 = 1U << 2,
        jacobsBogieAvailable = 1U << 3,
        isGhost = 1U << 4,
        fasterAroundCurves = 1U << 5,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(Flags38);

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
    void updateSignalOccupancyBasedOnBlockOccupancy(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType);
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
        bool isVehicleHead() const;
        VehicleHead* asVehicleHead() const;
        bool isVehicle1() const;
        Vehicle1* asVehicle1() const;
        bool isVehicle2() const;
        Vehicle2* asVehicle2() const;
        bool isVehicleBogie() const;
        VehicleBogie* asVehicleBogie() const;
        bool isVehicleBody() const;
        VehicleBody* asVehicleBody() const;
        bool hasSoundPlayer();
        VehicleSound* getVehicleSound();
        bool isVehicleTail() const;
        VehicleTail* asVehicleTail() const;
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
        uint8_t updateRoadTileOccupancy(const World::Pos3& loc, const TrackAndDirection::_RoadAndDirection rad, const bool setOccupied);
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

    struct VehicleUpdateDistances
    {
        int32_t unkDistance1; // 0x0113612C
        int32_t unkDistance2; // 0x01136130
    };

    // Don't use outside of vehicle files
    VehicleUpdateDistances& getVehicleUpdateDistances();

    uint8_t calculateYaw0FromVector(int16_t xDiff, int16_t yDiff);
    uint8_t calculateYaw1FromVectorPlane(int16_t xDiff, int16_t yDiff);
    uint8_t calculateYaw1FromVector(int16_t xDiff, int16_t yDiff);
    uint8_t calculateYaw4FromVector(int16_t xOffset, int16_t yOffset);

    void calculateTimeoutToBreakdown(VehicleBogie& vehBogie);

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
            CarComponentIter(const CarComponent* carComponent);

            CarComponentIter& operator++();

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
                CarIter(const Car* carComponent);

                CarIter& operator++();

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

        Vehicle(const VehicleHead& _head);
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
