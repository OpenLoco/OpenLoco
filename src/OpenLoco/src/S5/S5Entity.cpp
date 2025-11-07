#include "S5Entity.h"
#include "Effects/ExhaustEffect.h"
#include "Effects/ExplosionEffect.h"
#include "Effects/ExplosionSmokeEffect.h"
#include "Effects/FireballEffect.h"
#include "Effects/MoneyEffect.h"
#include "Effects/SmokeEffect.h"
#include "Effects/SplashEffect.h"
#include "Effects/VehicleCrashEffect.h"
#include "Entities/Entity.h"
#include "Vehicles/Vehicle.h"
#include <ranges>

namespace OpenLoco::S5
{
    static Entity exportNullEntity(const OpenLoco::Entity& src)
    {
        Entity dst{};
        dst.base.baseType = enumValue(src.baseType);
        dst.base.nextEntityId = enumValue(src.nextEntityId);
        dst.base.llPreviousId = enumValue(src.llPreviousId);
        dst.base.id = enumValue(src.id);
        dst.base.linkedListOffset = src.linkedListOffset;

        return dst;
    }

    static EntityBase exportEntityBase(const OpenLoco::EntityBase& src, const uint8_t type)
    {
        EntityBase dst{};
        dst.baseType = enumValue(src.baseType);
        dst.type = type;
        dst.nextQuadrantId = enumValue(src.nextQuadrantId);
        dst.nextEntityId = enumValue(src.nextEntityId);
        dst.llPreviousId = enumValue(src.llPreviousId);
        dst.linkedListOffset = src.linkedListOffset;
        dst.spriteHeightNegative = src.spriteHeightNegative;
        dst.id = enumValue(src.id);
        dst.vehicleFlags = enumValue(src.vehicleFlags);
        dst.position = src.position;
        dst.spriteWidth = src.spriteWidth;
        dst.spriteHeightPositive = src.spriteHeightPositive;
        dst.spriteLeft = src.spriteLeft;
        dst.spriteTop = src.spriteTop;
        dst.spriteRight = src.spriteRight;
        dst.spriteBottom = src.spriteBottom;
        dst.spriteYaw = src.spriteYaw;
        dst.spritePitch = enumValue(src.spritePitch);
        dst.owner = enumValue(src.owner);
        dst.name = src.name;

        return dst;
    }

    static Entity exportExhaust(const OpenLoco::Exhaust& src)
    {
        Entity dst{};
        S5::Exhaust& dstExhaust = reinterpret_cast<S5::Exhaust&>(dst);
        dstExhaust.base = exportEntityBase(src, enumValue(EffectType::exhaust));
        dstExhaust.frameNum = src.frameNum;
        dstExhaust.stationaryProgress = src.stationaryProgress;
        dstExhaust.windProgress = src.windProgress;
        dstExhaust.var_34 = src.var_34;
        dstExhaust.var_36 = src.var_36;
        dstExhaust.objectId = src.objectId;
        return dst;
    }

    static Entity exportMoneyEffect(const OpenLoco::MoneyEffect& src)
    {
        Entity dst{};
        S5::MoneyEffect& dstMoney = reinterpret_cast<S5::MoneyEffect&>(dst);
        dstMoney.base = exportEntityBase(src, enumValue(src.getSubType()));
        dstMoney.amount = src.amount;
        dstMoney.frame = src.frame;
        dstMoney.numMovements = src.numMovements;
        dstMoney.amount = src.amount;
        dstMoney.var_2E = enumValue(src.var_2E);
        dstMoney.offsetX = src.offsetX;
        dstMoney.wiggle = src.wiggle;

        return dst;
    }

    static Entity exportVehicleCrashParticle(const OpenLoco::VehicleCrashParticle& src)
    {
        Entity dst{};
        S5::VehicleCrashParticle& dstParticle = reinterpret_cast<S5::VehicleCrashParticle&>(dst);
        dstParticle.base = exportEntityBase(src, enumValue(EffectType::vehicleCrashParticle));
        dstParticle.timeToLive = src.timeToLive;
        dstParticle.frame = src.frame;
        dstParticle.colourSchemePrimary = enumValue(src.colourScheme.primary);
        dstParticle.colourSchemeSecondary = enumValue(src.colourScheme.secondary);
        dstParticle.crashedSpriteBase = src.crashedSpriteBase;
        dstParticle.velocity = src.velocity;
        dstParticle.accelerationX = src.accelerationX;
        dstParticle.accelerationY = src.accelerationY;
        dstParticle.accelerationZ = src.accelerationZ;
        return dst;
    }

    static Entity exportExplosionCloud(const OpenLoco::ExplosionCloud& src)
    {
        Entity dst{};
        S5::ExplosionCloud& dstCloud = reinterpret_cast<S5::ExplosionCloud&>(dst);
        dstCloud.base = exportEntityBase(src, enumValue(EffectType::explosionCloud));
        dstCloud.frame = src.frame;
        return dst;
    }

    static Entity exportSplash(const OpenLoco::Splash& src)
    {
        Entity dst{};
        S5::Splash& dstSplash = reinterpret_cast<S5::Splash&>(dst);
        dstSplash.base = exportEntityBase(src, enumValue(EffectType::splash));
        dstSplash.frame = src.frame;
        return dst;
    }

    static Entity exportFireball(const OpenLoco::Fireball& src)
    {
        Entity dst{};
        S5::Fireball& dstFireball = reinterpret_cast<S5::Fireball&>(dst);
        dstFireball.base = exportEntityBase(src, enumValue(EffectType::fireball));
        dstFireball.frame = src.frame;
        return dst;
    }

    static Entity exportExplosionSmoke(const OpenLoco::ExplosionSmoke& src)
    {
        Entity dst{};
        S5::ExplosionSmoke& dstSmoke = reinterpret_cast<S5::ExplosionSmoke&>(dst);
        dstSmoke.base = exportEntityBase(src, enumValue(EffectType::explosionSmoke));
        dstSmoke.frame = src.frame;
        return dst;
    }

    static Entity exportSmoke(const OpenLoco::Smoke& src)
    {
        Entity dst{};
        S5::Smoke& dstSmoke = reinterpret_cast<S5::Smoke&>(dst);
        dstSmoke.base = exportEntityBase(src, enumValue(EffectType::smoke));
        dstSmoke.frame = src.frame;
        return dst;
    }

    static Entity exportEffectEntity(const OpenLoco::EffectEntity& src)
    {
        switch (src.getSubType())
        {
            case EffectType::exhaust:
                return exportExhaust(*src.asExhaust());
            case EffectType::redGreenCurrency:
                return exportMoneyEffect(*src.asRedGreenCurrency());
            case EffectType::windowCurrency:
                return exportMoneyEffect(*src.asWindowCurrency());
            case EffectType::vehicleCrashParticle:
                return exportVehicleCrashParticle(*src.asVehicleCrashParticle());
            case EffectType::explosionCloud:
                return exportExplosionCloud(*src.asExplosionCloud());
            case EffectType::splash:
                return exportSplash(*src.asSplash());
            case EffectType::fireball:
                return exportFireball(*src.asFireball());
            case EffectType::explosionSmoke:
                return exportExplosionSmoke(*src.asExplosionSmoke());
            case EffectType::smoke:
                return exportSmoke(*src.asSmoke());
        }
    }

    static Entity exportVehicleHead(const OpenLoco::Vehicles::VehicleHead& src)
    {
        Entity dst{};
        S5::VehicleHead& dstHead = reinterpret_cast<S5::VehicleHead&>(dst);
        dstHead.base = exportEntityBase(src, enumValue(src.getSubType()));
        dstHead.head = enumValue(src.head);
        dstHead.remainingDistance = src.remainingDistance;
        dstHead.trackAndDirection = src.trackAndDirection.track._data;
        dstHead.subPosition = src.subPosition;
        dstHead.tileX = src.tileX;
        dstHead.tileY = src.tileY;
        dstHead.tileBaseZ = src.tileBaseZ;
        dstHead.trackType = src.trackType;
        dstHead.routingHandle = src.routingHandle._data;
        dstHead.var_38 = enumValue(src.var_38);
        dstHead.nextCarId = enumValue(src.nextCarId);
        dstHead.var_3C = src.var_3C;
        dstHead.mode = enumValue(src.mode);
        dstHead.ordinalNumber = src.ordinalNumber;
        dstHead.orderTableOffset = src.orderTableOffset;
        dstHead.currentOrder = src.currentOrder;
        dstHead.sizeOfOrderTable = src.sizeOfOrderTable;
        dstHead.trainAcceptedCargoTypes = src.trainAcceptedCargoTypes;
        dstHead.var_52 = src.var_52;
        dstHead.var_53 = src.var_53;
        dstHead.stationId = enumValue(src.stationId);
        dstHead.cargoTransferTimeout = src.cargoTransferTimeout;
        dstHead.var_58 = src.var_58;
        dstHead.var_5C = src.var_5C;
        dstHead.status = enumValue(src.status);
        dstHead.vehicleType = enumValue(src.vehicleType);
        dstHead.breakdownFlags = enumValue(src.breakdownFlags);
        dstHead.aiThoughtId = src.aiThoughtId;
        dstHead.aiPlacementPos = src.aiPlacementPos;
        dstHead.aiPlacementTaD = src.aiPlacementTaD;
        dstHead.aiPlacementBaseZ = src.aiPlacementBaseZ;
        dstHead.airportMovementEdge = src.airportMovementEdge;
        dstHead.totalRefundCost = src.totalRefundCost;
        dstHead.crashedTimeout = src.crashedTimeout;
        dstHead.manualPower = src.manualPower;
        dstHead.journeyStartPos = src.journeyStartPos;
        dstHead.journeyStartTicks = src.journeyStartTicks;
        dstHead.lastAverageSpeed = src.lastAverageSpeed.getRaw();
        dstHead.restartStoppedCarsTimeout = src.restartStoppedCarsTimeout;

        return dst;
    }

    static IncomeStats exportIncomeStats(const OpenLoco::Vehicles::IncomeStats& src)
    {
        IncomeStats dst{};
        dst.day = src.day;
        for (size_t i = 0; i < std::size(src.cargoTypes); i++)
        {
            dst.cargoTypes[i] = src.cargoTypes[i];
            dst.cargoQtys[i] = src.cargoQtys[i];
            dst.cargoDistances[i] = src.cargoDistances[i];
            dst.cargoAges[i] = src.cargoAges[i];
            dst.cargoProfits[i] = src.cargoProfits[i];
        }
        return dst;
    }

    static Entity exportVehicle1(const OpenLoco::Vehicles::Vehicle1& src)
    {
        Entity dst{};
        S5::Vehicle1& dstVehicle1 = reinterpret_cast<S5::Vehicle1&>(dst);
        dstVehicle1.base = exportEntityBase(src, enumValue(src.getSubType()));
        dstVehicle1.head = enumValue(src.head);
        dstVehicle1.remainingDistance = src.remainingDistance;
        dstVehicle1.trackAndDirection = src.trackAndDirection.track._data;
        dstVehicle1.subPosition = src.subPosition;
        dstVehicle1.tileX = src.tileX;
        dstVehicle1.tileY = src.tileY;
        dstVehicle1.tileBaseZ = src.tileBaseZ;
        dstVehicle1.trackType = src.trackType;
        dstVehicle1.routingHandle = src.routingHandle._data;
        dstVehicle1.var_38 = enumValue(src.var_38);
        dstVehicle1.nextCarId = enumValue(src.nextCarId);
        dstVehicle1.var_3C = src.var_3C;
        dstVehicle1.mode = enumValue(src.mode);
        dstVehicle1.targetSpeed = src.targetSpeed.getRaw();
        dstVehicle1.timeAtSignal = src.timeAtSignal;
        dstVehicle1.var_48 = enumValue(src.var_48);
        dstVehicle1.var_49 = src.var_49;
        dstVehicle1.dayCreated = src.dayCreated;
        dstVehicle1.var_4E = src.var_4E;
        dstVehicle1.var_50 = src.var_50;
        dstVehicle1.var_52 = src.var_52;
        dstVehicle1.lastIncome = exportIncomeStats(src.lastIncome);
        return dst;
    }

    static Entity exportVehicle2(const OpenLoco::Vehicles::Vehicle2& src)
    {
        Entity dst{};
        S5::Vehicle2& dstVehicle2 = reinterpret_cast<S5::Vehicle2&>(dst);
        dstVehicle2.base = exportEntityBase(src, enumValue(src.getSubType()));
        dstVehicle2.head = enumValue(src.head);
        dstVehicle2.remainingDistance = src.remainingDistance;
        dstVehicle2.trackAndDirection = src.trackAndDirection.track._data;
        dstVehicle2.subPosition = src.subPosition;
        dstVehicle2.tileX = src.tileX;
        dstVehicle2.tileY = src.tileY;
        dstVehicle2.tileBaseZ = src.tileBaseZ;
        dstVehicle2.trackType = src.trackType;
        dstVehicle2.routingHandle = src.routingHandle._data;
        dstVehicle2.var_38 = enumValue(src.var_38);
        dstVehicle2.nextCarId = enumValue(src.nextCarId);
        dstVehicle2.var_3C = src.var_3C;
        dstVehicle2.mode = enumValue(src.mode);
        dstVehicle2.drivingSoundId = src.drivingSoundId;
        dstVehicle2.drivingSoundVolume = src.drivingSoundVolume;
        dstVehicle2.drivingSoundFrequency = src.drivingSoundFrequency;
        dstVehicle2.objectId = src.objectId;
        dstVehicle2.soundFlags = enumValue(src.soundFlags);
        dstVehicle2.soundWindowNumber = src.soundWindowNumber;
        dstVehicle2.soundWindowType = enumValue(src.soundWindowType);
        dstVehicle2.var_4F = src.var_4F;
        dstVehicle2.totalPower = src.totalPower;
        dstVehicle2.totalWeight = src.totalWeight;
        dstVehicle2.maxSpeed = src.maxSpeed.getRaw();
        dstVehicle2.currentSpeed = src.currentSpeed.getRaw();
        dstVehicle2.motorState = enumValue(src.motorState);
        dstVehicle2.brakeLightTimeout = src.brakeLightTimeout;
        dstVehicle2.rackRailMaxSpeed = src.rackRailMaxSpeed.getRaw();
        dstVehicle2.curMonthRevenue = src.curMonthRevenue;
        std::ranges::copy(src.profit, dstVehicle2.profit);
        dstVehicle2.reliability = src.reliability;
        dstVehicle2.var_73 = enumValue(src.var_73);

        return dst;
    }

    static VehicleCargo exportVehicleCargo(const OpenLoco::Vehicles::VehicleCargo& src)
    {
        VehicleCargo dst{};
        dst.acceptedTypes = src.acceptedTypes;
        dst.type = enumValue(src.type);
        dst.maxQty = src.maxQty;
        dst.townFrom = enumValue(src.townFrom);
        dst.numDays = src.numDays;
        dst.qty = src.qty;
        return dst;
    }

    static Entity exportVehicleBogie(const OpenLoco::Vehicles::VehicleBogie& src)
    {
        Entity dst{};
        S5::VehicleBogie& dstBogie = reinterpret_cast<S5::VehicleBogie&>(dst);
        dstBogie.base = exportEntityBase(src, enumValue(src.getSubType()));
        dstBogie.colourSchemePrimary = enumValue(src.colourScheme.primary);
        dstBogie.colourSchemeSecondary = enumValue(src.colourScheme.secondary);
        dstBogie.head = enumValue(src.head);
        dstBogie.remainingDistance = src.remainingDistance;
        dstBogie.trackAndDirection = src.trackAndDirection.track._data;
        dstBogie.subPosition = src.subPosition;
        dstBogie.tileX = src.tileX;
        dstBogie.tileY = src.tileY;
        dstBogie.tileBaseZ = src.tileBaseZ;
        dstBogie.trackType = src.trackType;
        dstBogie.routingHandle = src.routingHandle._data;
        dstBogie.var_38 = enumValue(src.var_38);
        dstBogie.nextCarId = enumValue(src.nextCarId);
        dstBogie.var_3C = src.var_3C;
        dstBogie.objectId = src.objectId;
        dstBogie.mode = enumValue(src.mode);
        dstBogie.var_44 = src.var_44;
        dstBogie.animationIndex = src.animationIndex;
        dstBogie.var_47 = src.var_47;
        dstBogie.secondaryCargo = exportVehicleCargo(src.secondaryCargo);
        dstBogie.totalCarWeight = src.totalCarWeight;
        dstBogie.bodyIndex = src.bodyIndex;
        dstBogie.creationDay = src.creationDay;
        dstBogie.var_5A = src.var_5A;
        dstBogie.wheelSlipping = src.wheelSlipping;
        dstBogie.breakdownFlags = enumValue(src.breakdownFlags);
        dstBogie.var_60 = src.var_60;
        dstBogie.var_61 = src.var_61;
        dstBogie.refundCost = src.refundCost;
        dstBogie.reliability = src.reliability;
        dstBogie.timeoutToBreakdown = src.timeoutToBreakdown;
        dstBogie.breakdownTimeout = src.breakdownTimeout;

        return dst;
    }

    static Entity exportVehicleBody(const OpenLoco::Vehicles::VehicleBody& src)
    {
        Entity dst{};
        S5::VehicleBody& dstBody = reinterpret_cast<S5::VehicleBody&>(dst);
        dstBody.base = exportEntityBase(src, enumValue(src.getSubType()));
        dstBody.colourSchemePrimary = enumValue(src.colourScheme.primary);
        dstBody.colourSchemeSecondary = enumValue(src.colourScheme.secondary);
        dstBody.head = enumValue(src.head);
        dstBody.remainingDistance = src.remainingDistance;
        dstBody.trackAndDirection = src.trackAndDirection.track._data;
        dstBody.subPosition = src.subPosition;
        dstBody.tileX = src.tileX;
        dstBody.tileY = src.tileY;
        dstBody.tileBaseZ = src.tileBaseZ;
        dstBody.trackType = src.trackType;
        dstBody.routingHandle = src.routingHandle._data;
        dstBody.var_38 = enumValue(src.var_38);
        dstBody.objectSpriteType = enumValue(src.objectSpriteType);
        dstBody.nextCarId = enumValue(src.nextCarId);
        dstBody.var_3C = src.var_3C;
        dstBody.objectId = src.objectId;
        dstBody.mode = enumValue(src.mode);
        dstBody.var_44 = src.var_44;
        dstBody.animationFrame = src.animationFrame;
        dstBody.cargoFrame = src.cargoFrame;
        dstBody.primaryCargo = exportVehicleCargo(src.primaryCargo);
        dstBody.bodyIndex = src.bodyIndex;
        dstBody.chuffSoundIndex = src.chuffSoundIndex;
        dstBody.creationDay = src.creationDay;
        dstBody.var_5A = src.var_5A;
        dstBody.wheelSlipping = src.wheelSlipping;
        dstBody.breakdownFlags = enumValue(src.breakdownFlags);
        dstBody.refundCost = src.refundCost;
        dstBody.breakdownTimeout = src.breakdownTimeout;

        return dst;
    }

    static Entity exportVehicleTail(const OpenLoco::Vehicles::VehicleTail& src)
    {
        Entity dst{};
        S5::VehicleTail& dstTail = reinterpret_cast<S5::VehicleTail&>(dst);
        dstTail.base = exportEntityBase(src, enumValue(src.getSubType()));
        dstTail.head = enumValue(src.head);
        dstTail.remainingDistance = src.remainingDistance;
        dstTail.trackAndDirection = src.trackAndDirection.track._data;
        dstTail.subPosition = src.subPosition;
        dstTail.tileX = src.tileX;
        dstTail.tileY = src.tileY;
        dstTail.tileBaseZ = src.tileBaseZ;
        dstTail.trackType = src.trackType;
        dstTail.routingHandle = src.routingHandle._data;
        dstTail.var_38 = enumValue(src.var_38);
        dstTail.nextCarId = enumValue(src.nextCarId);
        dstTail.var_3C = src.var_3C;
        dstTail.mode = enumValue(src.mode);
        dstTail.drivingSoundId = src.drivingSoundId;
        dstTail.drivingSoundVolume = src.drivingSoundVolume;
        dstTail.drivingSoundFrequency = src.drivingSoundFrequency;
        dstTail.objectId = src.objectId;
        dstTail.soundFlags = enumValue(src.soundFlags);
        dstTail.soundWindowNumber = src.soundWindowNumber;
        dstTail.soundWindowType = enumValue(src.soundWindowType);
        dstTail.trainDanglingTimeout = src.trainDanglingTimeout;

        return dst;
    }

    static Entity exportVehicleEntity(const OpenLoco::Vehicles::VehicleBase& src)
    {
        switch (src.getSubType())
        {
            case Vehicles::VehicleEntityType::head:
                return exportVehicleHead(*src.asVehicleHead());
            case Vehicles::VehicleEntityType::vehicle_1:
                return exportVehicle1(*src.asVehicle1());
            case Vehicles::VehicleEntityType::vehicle_2:
                return exportVehicle2(*src.asVehicle2());
            case Vehicles::VehicleEntityType::bogie:
                return exportVehicleBogie(*src.asVehicleBogie());
            case Vehicles::VehicleEntityType::body_start:
            case Vehicles::VehicleEntityType::body_continued:
                return exportVehicleBody(*src.asVehicleBody());
            case Vehicles::VehicleEntityType::tail:
                return exportVehicleTail(*src.asVehicleTail());
        }
    }

    S5::Entity exportEntity(const OpenLoco::Entity& src)
    {
        if (src.baseType == EntityBaseType::effect)
        {
            return exportEffectEntity(*src.asBase<EffectEntity>());
        }
        else if (src.baseType == EntityBaseType::vehicle)
        {
            return exportVehicleEntity(*src.asBase<Vehicles::VehicleBase>());
        }
        else
        {
            return exportNullEntity(src);
        }
    }
}
