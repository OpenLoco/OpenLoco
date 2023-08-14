#pragma once

#include "Economy/Currency.h"
#include "Entities/Entity.h"
#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco
{
    struct Exhaust;
    struct MoneyEffect;
    struct VehicleCrashParticle;
    struct ExplosionCloud;
    struct Splash;
    struct Fireball;
    struct ExplosionSmoke;
    struct Smoke;

    enum class EffectType : uint8_t
    {
        exhaust = 0, // Steam from the exhaust
        redGreenCurrency = 1,
        windowCurrency = 2,       // currency which is created in the company's colour when a transaction is made (for example the train arrives with a passengers into the station)
        vehicleCrashParticle = 3, // parts (particles) of vehicle after crash which they fall to the ground after explosion
        explosionCloud = 4,       // explosion which is created when two trains (or maybe other vehicles) crash to each other
        splash = 5,               // splash when particles after explosion land to water and creates a splash (exploding train on the bridge)
        fireball = 6,
        explosionSmoke = 7,
        smoke = 8 // Smoke from broken down train
    };

#pragma pack(push, 1)
    struct EffectEntity : EntityBase
    {
        static constexpr auto kBaseType = EntityBaseType::effect;

    private:
        template<typename TType, EffectType TClass>
        TType* as() const
        {
            return getSubType() == TClass ? (TType*)this : nullptr;
        }

    public:
        EffectType getSubType() const { return EffectType(EntityBase::getSubType()); }
        void setSubType(const EffectType newType) { EntityBase::setSubType(static_cast<uint8_t>(newType)); }
        Exhaust* asExhaust() const { return as<Exhaust, EffectType::exhaust>(); }
        MoneyEffect* asRedGreenCurrency() const { return as<MoneyEffect, EffectType::redGreenCurrency>(); }
        MoneyEffect* asWindowCurrency() const { return as<MoneyEffect, EffectType::windowCurrency>(); }
        VehicleCrashParticle* asVehicleCrashParticle() const { return as<VehicleCrashParticle, EffectType::vehicleCrashParticle>(); }
        ExplosionCloud* asExplosionCloud() const { return as<ExplosionCloud, EffectType::explosionCloud>(); }
        Splash* asSplash() const { return as<Splash, EffectType::splash>(); }
        Fireball* asFireball() const { return as<Fireball, EffectType::fireball>(); }
        ExplosionSmoke* asExplosionSmoke() const { return as<ExplosionSmoke, EffectType::explosionSmoke>(); }
        Smoke* asSmoke() const { return as<Smoke, EffectType::smoke>(); }
        void update();
    };

    struct ExplosionCloud : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();
    };
    static_assert(sizeof(ExplosionCloud) == 0x2A);

    struct Splash : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();
    };
    static_assert(sizeof(Splash) == 0x2A);

    struct Fireball : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();
    };
    static_assert(sizeof(Fireball) == 0x2A);

    struct ExplosionSmoke : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();

        static ExplosionSmoke* create(const World::Pos3& loc);
    };
    static_assert(sizeof(ExplosionSmoke) == 0x2A);

    struct Smoke : EffectEntity
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();

        static Smoke* create(World::Pos3 loc);
    };
    static_assert(sizeof(Smoke) == 0x2A);
#pragma pack(pop)
}
