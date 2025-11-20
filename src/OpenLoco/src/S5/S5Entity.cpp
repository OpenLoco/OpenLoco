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
#include <algorithm>

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
        return {};
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
        dst.type = src.type;
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
        dstBogie.objectSpriteType = src.objectSpriteType;
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
        dstBody.objectSpriteType = src.objectSpriteType;
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
        return {};
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

    static OpenLoco::Entity importNullEntity(const S5::Entity& src)
    {
        OpenLoco::Entity dst{};
        dst.baseType = EntityBaseType::null;
        dst.nextEntityId = static_cast<EntityId>(src.base.nextEntityId);
        dst.llPreviousId = static_cast<EntityId>(src.base.llPreviousId);
        dst.id = static_cast<EntityId>(src.base.id);
        dst.linkedListOffset = src.base.linkedListOffset;
        return dst;
    }

    static OpenLoco::Entity importEntityBase(const S5::EntityBase& src)
    {
        OpenLoco::Entity dst{};
        dst.baseType = static_cast<EntityBaseType>(src.baseType);
        dst.nextQuadrantId = static_cast<EntityId>(src.nextQuadrantId);
        dst.nextEntityId = static_cast<EntityId>(src.nextEntityId);
        dst.llPreviousId = static_cast<EntityId>(src.llPreviousId);
        dst.linkedListOffset = src.linkedListOffset;
        dst.spriteHeightNegative = src.spriteHeightNegative;
        dst.id = static_cast<EntityId>(src.id);
        dst.vehicleFlags = static_cast<VehicleFlags>(src.vehicleFlags);
        dst.position = src.position;
        dst.spriteWidth = src.spriteWidth;
        dst.spriteHeightPositive = src.spriteHeightPositive;
        dst.spriteLeft = src.spriteLeft;
        dst.spriteTop = src.spriteTop;
        dst.spriteRight = src.spriteRight;
        dst.spriteBottom = src.spriteBottom;
        dst.spriteYaw = src.spriteYaw;
        dst.spritePitch = static_cast<Pitch>(src.spritePitch);
        dst.owner = static_cast<CompanyId>(src.owner);
        dst.name = src.name;
        return dst;
    }

    static void importExhaustEffect(OpenLoco::Exhaust& dst, const S5::Exhaust& src)
    {
        dst.frameNum = src.frameNum;
        dst.stationaryProgress = src.stationaryProgress;
        dst.windProgress = src.windProgress;
        dst.var_34 = src.var_34;
        dst.var_36 = src.var_36;
        dst.objectId = src.objectId;
    }

    static void importMoneyEffect(OpenLoco::MoneyEffect& dst, const S5::MoneyEffect& src)
    {
        dst.frame = src.frame;
        dst.numMovements = src.numMovements;
        dst.amount = src.amount;
        dst.var_2E = static_cast<CompanyId>(src.var_2E);
        dst.offsetX = src.offsetX;
        dst.wiggle = src.wiggle;
    }

    static void importVehicleCrashEffect(OpenLoco::VehicleCrashParticle& dst, const S5::VehicleCrashParticle& src)
    {
        dst.timeToLive = src.timeToLive;
        dst.frame = src.frame;
        dst.colourScheme.primary = static_cast<Colour>(src.colourSchemePrimary);
        dst.colourScheme.secondary = static_cast<Colour>(src.colourSchemeSecondary);
        dst.crashedSpriteBase = src.crashedSpriteBase;
        dst.velocity = src.velocity;
        dst.accelerationX = src.accelerationX;
        dst.accelerationY = src.accelerationY;
        dst.accelerationZ = src.accelerationZ;
    }

    static void importExplosionCloud(OpenLoco::ExplosionCloud& dst, const S5::ExplosionCloud& src)
    {
        dst.frame = src.frame;
    }

    static void importSplashEffect(OpenLoco::Splash& dst, const S5::Splash& src)
    {
        dst.frame = src.frame;
    }

    static void importFireballEffect(OpenLoco::Fireball& dst, const S5::Fireball& src)
    {
        dst.frame = src.frame;
    }

    static void importExplosionSmokeEffect(OpenLoco::ExplosionSmoke& dst, const S5::ExplosionSmoke& src)
    {
        dst.frame = src.frame;
    }

    static void importSmokeEffect(OpenLoco::Smoke& dst, const S5::Smoke& src)
    {
        dst.frame = src.frame;
    }

    static OpenLoco::Entity importEffectEntity(const S5::Entity& src)
    {
        const auto effectType = static_cast<EffectType>(src.base.type);
        auto dst = importEntityBase(src.base);
        auto* effectEntity = dst.asBase<EffectEntity>();
        effectEntity->setSubType(effectType);

        switch (effectType)
        {
            case EffectType::exhaust:
                importExhaustEffect(*effectEntity->asExhaust(), reinterpret_cast<const S5::Exhaust&>(src));
                break;
            case EffectType::redGreenCurrency:
                importMoneyEffect(*effectEntity->asRedGreenCurrency(), reinterpret_cast<const S5::MoneyEffect&>(src));
                break;
            case EffectType::windowCurrency:
                importMoneyEffect(*effectEntity->asWindowCurrency(), reinterpret_cast<const S5::MoneyEffect&>(src));
                break;
            case EffectType::vehicleCrashParticle:
                importVehicleCrashEffect(*effectEntity->asVehicleCrashParticle(), reinterpret_cast<const S5::VehicleCrashParticle&>(src));
                break;
            case EffectType::explosionCloud:
                importExplosionCloud(*effectEntity->asExplosionCloud(), reinterpret_cast<const S5::ExplosionCloud&>(src));
                break;
            case EffectType::splash:
                importSplashEffect(*effectEntity->asSplash(), reinterpret_cast<const S5::Splash&>(src));
                break;
            case EffectType::fireball:
                importFireballEffect(*effectEntity->asFireball(), reinterpret_cast<const S5::Fireball&>(src));
                break;
            case EffectType::explosionSmoke:
                importExplosionSmokeEffect(*effectEntity->asExplosionSmoke(), reinterpret_cast<const S5::ExplosionSmoke&>(src));
                break;
            case EffectType::smoke:
                importSmokeEffect(*effectEntity->asSmoke(), reinterpret_cast<const S5::Smoke&>(src));
                break;
        }
        return dst;
    }

    static void importHeadVehicle(OpenLoco::Vehicles::VehicleHead& dst, const S5::VehicleHead& src)
    {
        dst.head = static_cast<EntityId>(src.head);
        dst.remainingDistance = src.remainingDistance;
        dst.trackAndDirection.track._data = src.trackAndDirection;
        dst.subPosition = src.subPosition;
        dst.tileX = src.tileX;
        dst.tileY = src.tileY;
        dst.tileBaseZ = src.tileBaseZ;
        dst.trackType = src.trackType;
        dst.routingHandle._data = src.routingHandle;
        dst.var_38 = static_cast<Vehicles::Flags38>(src.var_38);
        dst.nextCarId = static_cast<EntityId>(src.nextCarId);
        dst.var_3C = src.var_3C;
        dst.mode = static_cast<TransportMode>(src.mode);
        dst.ordinalNumber = src.ordinalNumber;
        dst.orderTableOffset = src.orderTableOffset;
        dst.currentOrder = src.currentOrder;
        dst.sizeOfOrderTable = src.sizeOfOrderTable;
        dst.trainAcceptedCargoTypes = src.trainAcceptedCargoTypes;
        dst.var_52 = src.var_52;
        dst.var_53 = src.var_53;
        dst.stationId = static_cast<StationId>(src.stationId);
        dst.cargoTransferTimeout = src.cargoTransferTimeout;
        dst.var_58 = src.var_58;
        dst.var_5C = src.var_5C;
        dst.status = static_cast<Vehicles::Status>(src.status);
        dst.vehicleType = static_cast<VehicleType>(src.vehicleType);
        dst.breakdownFlags = static_cast<Vehicles::BreakdownFlags>(src.breakdownFlags);
        dst.aiThoughtId = src.aiThoughtId;
        dst.aiPlacementPos = src.aiPlacementPos;
        dst.aiPlacementTaD = src.aiPlacementTaD;
        dst.aiPlacementBaseZ = src.aiPlacementBaseZ;
        dst.airportMovementEdge = src.airportMovementEdge;
        dst.totalRefundCost = src.totalRefundCost;
        dst.crashedTimeout = src.crashedTimeout;
        dst.manualPower = src.manualPower;
        dst.journeyStartPos = src.journeyStartPos;
        dst.journeyStartTicks = src.journeyStartTicks;
        dst.lastAverageSpeed = Speed16(src.lastAverageSpeed);
        dst.restartStoppedCarsTimeout = src.restartStoppedCarsTimeout;
    }

    static OpenLoco::Vehicles::IncomeStats importIncomeStats(const S5::IncomeStats& src)
    {
        OpenLoco::Vehicles::IncomeStats dst{};
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

    static void importVehicle1(OpenLoco::Vehicles::Vehicle1& dst, const S5::Vehicle1& src)
    {
        dst.head = static_cast<EntityId>(src.head);
        dst.remainingDistance = src.remainingDistance;
        dst.trackAndDirection.track._data = src.trackAndDirection;
        dst.subPosition = src.subPosition;
        dst.tileX = src.tileX;
        dst.tileY = src.tileY;
        dst.tileBaseZ = src.tileBaseZ;
        dst.trackType = src.trackType;
        dst.routingHandle._data = src.routingHandle;
        dst.var_38 = static_cast<Vehicles::Flags38>(src.var_38);
        dst.nextCarId = static_cast<EntityId>(src.nextCarId);
        dst.var_3C = src.var_3C;
        dst.mode = static_cast<TransportMode>(src.mode);
        dst.targetSpeed = Speed16(src.targetSpeed);
        dst.timeAtSignal = src.timeAtSignal;
        dst.var_48 = static_cast<Vehicles::Flags48>(src.var_48);
        dst.var_49 = src.var_49;
        dst.dayCreated = src.dayCreated;
        dst.var_4E = src.var_4E;
        dst.var_50 = src.var_50;
        dst.var_52 = src.var_52;
        dst.lastIncome = importIncomeStats(src.lastIncome);
    }

    static void importVehicle2(OpenLoco::Vehicles::Vehicle2& dst, const S5::Vehicle2& src)
    {
        dst.head = static_cast<EntityId>(src.head);
        dst.remainingDistance = src.remainingDistance;
        dst.trackAndDirection.track._data = src.trackAndDirection;
        dst.subPosition = src.subPosition;
        dst.tileX = src.tileX;
        dst.tileY = src.tileY;
        dst.tileBaseZ = src.tileBaseZ;
        dst.trackType = src.trackType;
        dst.routingHandle._data = src.routingHandle;
        dst.var_38 = static_cast<Vehicles::Flags38>(src.var_38);
        dst.nextCarId = static_cast<EntityId>(src.nextCarId);
        dst.var_3C = src.var_3C;
        dst.mode = static_cast<TransportMode>(src.mode);
        dst.drivingSoundId = src.drivingSoundId;
        dst.drivingSoundVolume = src.drivingSoundVolume;
        dst.drivingSoundFrequency = src.drivingSoundFrequency;
        dst.objectId = src.objectId;
        dst.soundFlags = static_cast<Vehicles::SoundFlags>(src.soundFlags);
        dst.soundWindowNumber = src.soundWindowNumber;
        dst.soundWindowType = static_cast<Ui::WindowType>(src.soundWindowType);
        dst.var_4F = src.var_4F;
        dst.totalPower = src.totalPower;
        dst.totalWeight = src.totalWeight;
        dst.maxSpeed = Speed16(src.maxSpeed);
        dst.currentSpeed = Speed32(src.currentSpeed);
        dst.motorState = static_cast<Vehicles::MotorState>(src.motorState);
        dst.brakeLightTimeout = src.brakeLightTimeout;
        dst.rackRailMaxSpeed = Speed16(src.rackRailMaxSpeed);
        dst.curMonthRevenue = src.curMonthRevenue;
        std::ranges::copy(src.profit, dst.profit);
        dst.reliability = src.reliability;
        dst.var_73 = static_cast<Vehicles::Flags73>(src.var_73);
    }

    static OpenLoco::Vehicles::VehicleCargo importVehicleCargo(const S5::VehicleCargo& src)
    {
        OpenLoco::Vehicles::VehicleCargo dst{};
        dst.acceptedTypes = src.acceptedTypes;
        dst.type = src.type;
        dst.maxQty = src.maxQty;
        dst.townFrom = static_cast<StationId>(src.townFrom);
        dst.numDays = src.numDays;
        dst.qty = src.qty;
        return dst;
    }

    static void importVehicleBogie(OpenLoco::Vehicles::VehicleBogie& dst, const S5::VehicleBogie& src)
    {
        dst.colourScheme.primary = static_cast<Colour>(src.colourSchemePrimary);
        dst.colourScheme.secondary = static_cast<Colour>(src.colourSchemeSecondary);
        dst.head = static_cast<EntityId>(src.head);
        dst.remainingDistance = src.remainingDistance;
        dst.trackAndDirection.track._data = src.trackAndDirection;
        dst.subPosition = src.subPosition;
        dst.tileX = src.tileX;
        dst.tileY = src.tileY;
        dst.tileBaseZ = src.tileBaseZ;
        dst.trackType = src.trackType;
        dst.routingHandle._data = src.routingHandle;
        dst.var_38 = static_cast<Vehicles::Flags38>(src.var_38);
        dst.objectSpriteType = src.objectSpriteType;
        dst.nextCarId = static_cast<EntityId>(src.nextCarId);
        dst.var_3C = src.var_3C;
        dst.objectId = src.objectId;
        dst.mode = static_cast<TransportMode>(src.mode);
        dst.var_44 = src.var_44;
        dst.animationIndex = src.animationIndex;
        dst.var_47 = src.var_47;
        dst.secondaryCargo = importVehicleCargo(src.secondaryCargo);
        dst.totalCarWeight = src.totalCarWeight;
        dst.bodyIndex = src.bodyIndex;
        dst.creationDay = src.creationDay;
        dst.var_5A = src.var_5A;
        dst.wheelSlipping = src.wheelSlipping;
        dst.breakdownFlags = static_cast<Vehicles::BreakdownFlags>(src.breakdownFlags);
        dst.var_60 = src.var_60;
        dst.var_61 = src.var_61;
        dst.refundCost = src.refundCost;
        dst.reliability = src.reliability;
        dst.timeoutToBreakdown = src.timeoutToBreakdown;
        dst.breakdownTimeout = src.breakdownTimeout;
    }

    static void importVehicleBody(OpenLoco::Vehicles::VehicleBody& dst, const S5::VehicleBody& src)
    {
        dst.colourScheme.primary = static_cast<Colour>(src.colourSchemePrimary);
        dst.colourScheme.secondary = static_cast<Colour>(src.colourSchemeSecondary);
        dst.head = static_cast<EntityId>(src.head);
        dst.remainingDistance = src.remainingDistance;
        dst.trackAndDirection.track._data = src.trackAndDirection;
        dst.subPosition = src.subPosition;
        dst.tileX = src.tileX;
        dst.tileY = src.tileY;
        dst.tileBaseZ = src.tileBaseZ;
        dst.trackType = src.trackType;
        dst.routingHandle._data = src.routingHandle;
        dst.var_38 = static_cast<Vehicles::Flags38>(src.var_38);
        dst.objectSpriteType = src.objectSpriteType;
        dst.nextCarId = static_cast<EntityId>(src.nextCarId);
        dst.var_3C = src.var_3C;
        dst.objectId = src.objectId;
        dst.mode = static_cast<TransportMode>(src.mode);
        dst.var_44 = src.var_44;
        dst.animationFrame = src.animationFrame;
        dst.cargoFrame = src.cargoFrame;
        dst.primaryCargo = importVehicleCargo(src.primaryCargo);
        dst.bodyIndex = src.bodyIndex;
        dst.chuffSoundIndex = src.chuffSoundIndex;
        dst.creationDay = src.creationDay;
        dst.var_5A = src.var_5A;
        dst.wheelSlipping = src.wheelSlipping;
        dst.breakdownFlags = static_cast<Vehicles::BreakdownFlags>(src.breakdownFlags);
        dst.refundCost = src.refundCost;
        dst.breakdownTimeout = src.breakdownTimeout;
    }

    static void importVehicleTail(OpenLoco::Vehicles::VehicleTail& dst, const S5::VehicleTail& src)
    {
        dst.head = static_cast<EntityId>(src.head);
        dst.remainingDistance = src.remainingDistance;
        dst.trackAndDirection.track._data = src.trackAndDirection;
        dst.subPosition = src.subPosition;
        dst.tileX = src.tileX;
        dst.tileY = src.tileY;
        dst.tileBaseZ = src.tileBaseZ;
        dst.trackType = src.trackType;
        dst.routingHandle._data = src.routingHandle;
        dst.var_38 = static_cast<Vehicles::Flags38>(src.var_38);
        dst.nextCarId = static_cast<EntityId>(src.nextCarId);
        dst.var_3C = src.var_3C;
        dst.mode = static_cast<TransportMode>(src.mode);
        dst.drivingSoundId = src.drivingSoundId;
        dst.drivingSoundVolume = src.drivingSoundVolume;
        dst.drivingSoundFrequency = src.drivingSoundFrequency;
        dst.objectId = src.objectId;
        dst.soundFlags = static_cast<Vehicles::SoundFlags>(src.soundFlags);
        dst.soundWindowNumber = src.soundWindowNumber;
        dst.soundWindowType = static_cast<Ui::WindowType>(src.soundWindowType);
        dst.trainDanglingTimeout = src.trainDanglingTimeout;
    }

    static OpenLoco::Entity importVehicleEntity(const S5::Entity& src)
    {
        const auto vehicleType = static_cast<OpenLoco::Vehicles::VehicleEntityType>(src.base.type);
        auto dst = importEntityBase(src.base);
        auto* vehicleEntity = dst.asBase<OpenLoco::Vehicles::VehicleBase>();
        vehicleEntity->setSubType(vehicleType);

        switch (vehicleType)
        {
            case OpenLoco::Vehicles::VehicleEntityType::head:
                importHeadVehicle(*vehicleEntity->asVehicleHead(), reinterpret_cast<const S5::VehicleHead&>(src));
                break;
            case OpenLoco::Vehicles::VehicleEntityType::vehicle_1:
                importVehicle1(*vehicleEntity->asVehicle1(), reinterpret_cast<const S5::Vehicle1&>(src));
                break;
            case OpenLoco::Vehicles::VehicleEntityType::vehicle_2:
                importVehicle2(*vehicleEntity->asVehicle2(), reinterpret_cast<const S5::Vehicle2&>(src));
                break;
            case OpenLoco::Vehicles::VehicleEntityType::bogie:
                importVehicleBogie(*vehicleEntity->asVehicleBogie(), reinterpret_cast<const S5::VehicleBogie&>(src));
                break;
            case OpenLoco::Vehicles::VehicleEntityType::body_start:
            case OpenLoco::Vehicles::VehicleEntityType::body_continued:
                importVehicleBody(*vehicleEntity->asVehicleBody(), reinterpret_cast<const S5::VehicleBody&>(src));
                break;
            case OpenLoco::Vehicles::VehicleEntityType::tail:
                importVehicleTail(*vehicleEntity->asVehicleTail(), reinterpret_cast<const S5::VehicleTail&>(src));
                break;
        }

        return dst;
    }

    OpenLoco::Entity importEntity(const S5::Entity& src)
    {
        const auto baseType = static_cast<EntityBaseType>(src.base.baseType);
        if (baseType == EntityBaseType::effect)
        {
            return importEffectEntity(src);
        }
        else if (baseType == EntityBaseType::vehicle)
        {
            return importVehicleEntity(src);
        }
        else
        {
            return importNullEntity(src);
        }
    }
}
