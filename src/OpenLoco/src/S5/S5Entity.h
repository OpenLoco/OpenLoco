#include "Economy/Currency.h"
#include <OpenLoco/Engine/World.hpp>
#include <cstdint>

namespace OpenLoco
{
    struct Entity;
}

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct EntityBase
    {
        uint8_t baseType;             // 0x00
        uint8_t type;                 // 0x01 Use type specific getters/setters as this depends on baseType
        uint16_t nextQuadrantId;      // 0x02
        uint16_t nextEntityId;        // 0x04
        uint16_t llPreviousId;        // 0x06
        uint8_t linkedListOffset;     // 0x08
        uint8_t spriteHeightNegative; // 0x09
        uint16_t id;                  // 0x0A
        uint16_t vehicleFlags;        // 0x0C, Move these to VehicleBase after full reimplementation
        World::Pos3 position;         // 0x0E
        uint8_t spriteWidth;          // 0x14
        uint8_t spriteHeightPositive; // 0x15
        int16_t spriteLeft;           // 0x16
        int16_t spriteTop;            // 0x18
        int16_t spriteRight;          // 0x1A
        int16_t spriteBottom;         // 0x1C
        uint8_t spriteYaw;            // 0x1E
        uint8_t spritePitch;          // 0x1F
        uint8_t pad_20;               // 0x20
        uint8_t owner;                // 0x21
        uint16_t name;                // 0x22, combined with ordinalNumber on vehicles
    };
    static_assert(sizeof(EntityBase) == 0x24);

    struct Exhaust
    {
        EntityBase base; // 0x00
        uint8_t pad_24[0x26 - 0x24];
        uint16_t frameNum;          // 0x26
        int16_t stationaryProgress; // 0x28
        uint8_t pad_2A[0x32 - 0x2A];
        uint16_t windProgress; // 0x32
        int16_t var_34;
        int16_t var_36;
        uint8_t pad_38[0x49 - 0x38];
        uint8_t objectId; // 0x49
    };

    struct ExplosionCloud
    {
        EntityBase base; // 0x00
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28
    };

    struct ExplosionSmoke
    {
        EntityBase base; // 0x00
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28
    };

    struct Fireball
    {
        EntityBase base; // 0x00
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28
    };

    struct MoneyEffect
    {
        EntityBase base; // 0x00
        uint8_t pad_24[0x26 - 0x24];
        union
        {
            uint16_t frame;     // 0x26
            uint16_t moveDelay; // 0x26 Note: this is only used by redGreen money (RCT2 legacy)
        };
        uint16_t numMovements; // 0x28 Note: this is only used by redGreen money (RCT2 legacy)
        int32_t amount;        // 0x2A - currency amount in British pounds - different currencies are probably getting recalculated
        uint8_t var_2E;        // company colour?
        uint8_t pad_2F[0x44 - 0x2F];
        int16_t offsetX; // 0x44
        uint16_t wiggle; // 0x46
    };

    struct Smoke
    {
        EntityBase base; // 0x00
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28
    };

    struct Splash
    {
        EntityBase base; // 0x00
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28
    };

    struct VehicleCrashParticle
    {
        EntityBase base;               // 0x00
        uint8_t pad_24[0x02];          // 0x24
        uint16_t timeToLive;           // 0x26
        uint16_t frame;                // 0x28
        uint8_t pad_2A[0x04];          // 0x2A
        uint8_t colourSchemePrimary;   // 0x2E
        uint8_t colourSchemeSecondary; // 0x2F
        uint16_t crashedSpriteBase;    // 0x30 crashed_sprite_base
        World::Pos3 velocity;          // 0x32
        int32_t accelerationX;         // 0x38
        int32_t accelerationY;         // 0x3C
        int32_t accelerationZ;         // 0x40
    };

    struct VehicleCargo
    {
        uint32_t acceptedTypes; // 0x48
        uint8_t type;           // 0x4C
        uint8_t maxQty;         // 0x4D
        uint16_t townFrom;      // 0x4E
        uint8_t numDays;        // 0x50
        uint8_t qty;            // 0x51
    };
    static_assert(sizeof(VehicleCargo) == 0xA);

    struct VehicleHead
    {
        EntityBase base; // 0x00
        uint8_t pad_24[0x26 - 0x24];
        uint16_t head;              // 0x26
        int32_t remainingDistance;  // 0x28
        uint16_t trackAndDirection; // 0x2C
        uint16_t subPosition;       // 0x2E
        int16_t tileX;              // 0x30
        int16_t tileY;              // 0x32
        World::SmallZ tileBaseZ;    // 0x34
        uint8_t trackType;          // 0x35 field same in all vehicles
        uint16_t routingHandle;     // 0x36 field same in all vehicles orderId * maxNumRoutingSteps
        uint8_t var_38;
        uint8_t pad_39;      // 0x39
        uint16_t nextCarId;  // 0x3A
        int32_t var_3C;      // 0x3C
        uint8_t pad_40[0x2]; // 0x40
        uint8_t mode;        // 0x42 field same in all vehicles
        uint8_t pad_43;
        int16_t ordinalNumber;            // 0x44
        uint32_t orderTableOffset;        // 0x46 offset into Order Table
        uint16_t currentOrder;            // 0x4A offset, combine with orderTableOffset
        uint16_t sizeOfOrderTable;        // 0x4C size of Order Table
        uint32_t trainAcceptedCargoTypes; // 0x4E
        uint8_t var_52;
        uint8_t var_53;                // 0x53 mods?
        uint16_t stationId;            // 0x54
        uint16_t cargoTransferTimeout; // 0x56
        uint32_t var_58;
        uint8_t var_5C;
        uint8_t status;                    // 0x5D
        uint8_t vehicleType;               // 0x5E
        uint8_t breakdownFlags;            // 0x5F
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
        int16_t lastAverageSpeed;          // 0x77
        uint8_t restartStoppedCarsTimeout; // 0x79 timeout before auto starting trams/buses
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
    };
    static_assert(sizeof(IncomeStats) == 0x2C);

    struct Vehicle1
    {
        EntityBase base; // 0x00
        uint8_t pad_24[0x26 - 0x24];
        uint16_t head;              // 0x26
        int32_t remainingDistance;  // 0x28
        uint16_t trackAndDirection; // 0x2C
        uint16_t subPosition;       // 0x2E
        int16_t tileX;              // 0x30
        int16_t tileY;              // 0x32
        World::SmallZ tileBaseZ;    // 0x34
        uint8_t trackType;          // 0x35 field same in all vehicles
        uint16_t routingHandle;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t pad_39;      // 0x39
        uint16_t nextCarId;  // 0x3A
        int32_t var_3C;      // 0x3C
        uint8_t pad_40[0x2]; // 0x40
        uint8_t mode;        // 0x42 field same in all vehicles
        uint8_t pad_43;
        int16_t targetSpeed;   // 0x44
        uint16_t timeAtSignal; // 0x46
        uint8_t var_48;
        uint8_t var_49;      // 0x49 rackrail mod?
        uint32_t dayCreated; // 0x4A
        uint16_t var_4E;
        uint16_t var_50;
        uint8_t var_52;
        IncomeStats lastIncome; // 0x53
    };
    static_assert(sizeof(Vehicle1) == 0x7F); // Can't use offset_of change this to last field if more found

    struct Vehicle2
    {
        EntityBase base; // 0x00
        uint8_t pad_24[0x26 - 0x24];
        uint16_t head;              // 0x26
        int32_t remainingDistance;  // 0x28
        uint16_t trackAndDirection; // 0x2C
        uint16_t subPosition;       // 0x2E
        int16_t tileX;              // 0x30
        int16_t tileY;              // 0x32
        World::SmallZ tileBaseZ;    // 0x34
        uint8_t trackType;          // 0x35 field same in all vehicles
        uint16_t routingHandle;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t pad_39;              // 0x39
        uint16_t nextCarId;          // 0x3A
        int32_t var_3C;              // 0x3C field same in all vehicles unread for veh2
        uint8_t pad_40[0x42 - 0x40]; // 0x40
        uint8_t mode;                // 0x42 field same in all vehicles
        uint8_t pad_43;
        uint8_t drivingSoundId;         // 0x44
        uint8_t drivingSoundVolume;     // 0x45 channel attribute volume related
        uint16_t drivingSoundFrequency; // 0x46 channel attribute frequency
        uint16_t objectId;              // 0x48 vehicle object (used for sound)
        uint16_t soundFlags;            // 0x4A common with tail
        uint16_t soundWindowNumber;     // 0x4C common with tail
        uint8_t soundWindowType;        // 0x4E common with tail
        int8_t var_4F;
        uint16_t totalPower;  // 0x50 maybe not used by aircraft and ship
        uint16_t totalWeight; // 0x52
        int16_t maxSpeed;     // 0x54
        int32_t currentSpeed; // 0x56
        uint8_t motorState;
        uint8_t brakeLightTimeout;
        int16_t rackRailMaxSpeed;     // 0x5C
        currency32_t curMonthRevenue; // 0x5E monthly revenue
        currency32_t profit[4];       // 0x62 last 4 months net profit
        uint8_t reliability;          // 0x72
        uint8_t var_73;               // 0x73 (bit 0 = broken down, bit 1 = still powered)
    };
    static_assert(sizeof(Vehicle2) == 0x74); // Can't use offset_of change this to last field if more found

    struct VehicleBody
    {
        EntityBase base;               // 0x00
        uint8_t colourSchemePrimary;   // 0x24
        uint8_t colourSchemeSecondary; // 0x25
        uint16_t head;                 // 0x26
        int32_t remainingDistance;     // 0x28
        uint16_t trackAndDirection;    // 0x2C
        uint16_t subPosition;          // 0x2E
        int16_t tileX;                 // 0x30
        int16_t tileY;                 // 0x32
        World::SmallZ tileBaseZ;       // 0x34
        uint8_t trackType;             // 0x35 field same in all vehicles
        uint16_t routingHandle;        // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t objectSpriteType; // 0x39
        uint16_t nextCarId;       // 0x3A
        int32_t var_3C;           // 0x3C field same in all vehicles unread for body
        uint16_t objectId;        // 0x40
        uint8_t mode;             // 0x42
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
        uint8_t wheelSlipping;  // 0x5E timeout that counts up
        uint8_t breakdownFlags; // 0x5F
        uint16_t pad_60;        // 0x60
        uint32_t refundCost;    // 0x62
        uint8_t pad_66[0x6A - 0x66];
        uint8_t breakdownTimeout; // 0x6A (likely unused)
    };
    static_assert(sizeof(VehicleBody) == 0x6B); // Can't use offset_of change this to last field if more found

    struct VehicleBogie
    {
        EntityBase base;               // 0x00
        uint8_t colourSchemePrimary;   // 0x24
        uint8_t colourSchemeSecondary; // 0x25
        uint16_t head;                 // 0x26
        int32_t remainingDistance;     // 0x28
        uint16_t trackAndDirection;    // 0x2C
        uint16_t subPosition;          // 0x2E
        int16_t tileX;                 // 0x30
        int16_t tileY;                 // 0x32
        World::SmallZ tileBaseZ;       // 0x34
        uint8_t trackType;             // 0x35 field same in all vehicles
        uint16_t routingHandle;        // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t objectSpriteType; // 0x39
        uint16_t nextCarId;       // 0x3A
        int32_t var_3C;           // 0x3C field same in all vehicles unread for bogie
        uint16_t objectId;        // 0x40
        uint8_t mode;             // 0x42 field same in all vehicles
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
        uint8_t breakdownFlags;
        uint8_t var_60;
        uint8_t var_61;
        uint32_t refundCost;         // 0x62 front bogies only
        uint16_t reliability;        // 0x66 front bogies only
        uint16_t timeoutToBreakdown; // 0x68 front bogies only (days) counts down to the next breakdown 0xFFFFU disables this
        uint8_t breakdownTimeout;    // 0x6A front bogies only (days)
    };
    static_assert(sizeof(VehicleBogie) == 0x6B); // Can't use offset_of change this to last field if more found

    struct VehicleTail
    {
        EntityBase base; // 0x00
        uint8_t pad_24[0x26 - 0x24];
        uint16_t head;              // 0x26
        int32_t remainingDistance;  // 0x28
        uint16_t trackAndDirection; // 0x2C
        uint16_t subPosition;       // 0x2E
        int16_t tileX;              // 0x30
        int16_t tileY;              // 0x32
        World::SmallZ tileBaseZ;    // 0x34
        uint8_t trackType;          // 0x35 field same in all vehicles
        uint16_t routingHandle;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t pad_39;              // 0x39
        uint16_t nextCarId;          // 0x3A
        int32_t var_3C;              // 0x3C field same in all vehicles unread for tail
        uint8_t pad_40[0x42 - 0x40]; // 0x40
        uint8_t mode;                // 0x42 field same in all vehicles
        uint8_t pad_43;
        uint8_t drivingSoundId;         // 0x44
        uint8_t drivingSoundVolume;     // 0x45 channel attribute volume related
        uint16_t drivingSoundFrequency; // 0x46 channel attribute frequency
        uint16_t objectId;              // 0x48 vehicle object (used for sound)
        uint16_t soundFlags;            // 0x4A common with veh_2
        uint16_t soundWindowNumber;     // 0x4C common with veh_2
        uint8_t soundWindowType;        // 0x4E common with veh_2
        uint16_t trainDanglingTimeout;  // 0x4F counts up when no cars on train
    };
    static_assert(sizeof(VehicleTail) == 0x51); // Can't use offset_of change this to last field if more found

    struct Entity
    {
        EntityBase base;
        uint8_t pad_24[0x80 - 0x24]; // Type specific data
    };
    static_assert(sizeof(Entity) == 0x80);
#pragma pack(pop)

    S5::Entity exportEntity(const OpenLoco::Entity& src);
    OpenLoco::Entity importEntity(const S5::Entity& src);
}
