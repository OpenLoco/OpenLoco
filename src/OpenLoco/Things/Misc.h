#pragma once

#include "../Company.h"
#include "../Objects/SteamObject.h"
#include "Thing.h"

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

    enum class MiscThingType : uint8_t
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
    struct MiscBase : thing_base
    {
    private:
        template<typename TType, MiscThingType TClass>
        TType* as() const
        {
            return getSubType() == TClass ? (TType*)this : nullptr;
        }

    public:
        MiscThingType getSubType() const { return MiscThingType(thing_base::getSubType()); }
        void setSubType(const MiscThingType newType) { thing_base::setSubType(static_cast<uint8_t>(newType)); }
        Exhaust* asExhaust() const { return as<Exhaust, MiscThingType::exhaust>(); }
        MoneyEffect* asRedGreenCurrency() const { return as<MoneyEffect, MiscThingType::redGreenCurrency>(); }
        MoneyEffect* asWindowCurrency() const { return as<MoneyEffect, MiscThingType::windowCurrency>(); }
        VehicleCrashParticle* asVehicleCrashParticle() const { return as<VehicleCrashParticle, MiscThingType::vehicleCrashParticle>(); }
        ExplosionCloud* asExplosionCloud() const { return as<ExplosionCloud, MiscThingType::explosionCloud>(); }
        Splash* asSplash() const { return as<Splash, MiscThingType::splash>(); }
        Fireball* asFireball() const { return as<Fireball, MiscThingType::fireball>(); }
        ExplosionSmoke* asExplosionSmoke() const { return as<ExplosionSmoke, MiscThingType::explosionSmoke>(); }
        Smoke* asSmoke() const { return as<Smoke, MiscThingType::smoke>(); }
    };

    struct Exhaust : MiscBase
    {
        uint8_t pad_20[0x26 - 0x20];
        int16_t var_26;
        int16_t var_28;
        uint8_t pad_2A[0x32 - 0x2A];
        int16_t var_32;
        int16_t var_34;
        int16_t var_36;
        uint8_t pad_38[0x49 - 0x38];
        uint8_t object_id; // 0x49

        SteamObject* object() const;

        static Exhaust* create(loc16 loc, uint8_t type);
    };

    struct MoneyEffect : MiscBase
    {
        uint8_t pad_20[0x2A - 0x20];
        int32_t amount; // 0x2A - currency amount in British pounds - different currencies are probably getting recalculated
        int8_t var_2E;  // company colour?
        uint8_t pad_2F[0x44 - 0x2F];
        int16_t offsetX; // 0x44
        uint16_t wiggle; // 0x46

        //static MoneyEffect* create(loc16 loc, uint8_t type);
    };

    struct VehicleCrashParticle : MiscBase
    {
        uint8_t pad_20[0x28 - 0x20];
        uint16_t frame; // 0x28
        uint8_t pad_2A[0x2E - 0x2A];
        ColourScheme colourScheme;  // 0x2E
        uint16_t crashedSpriteBase; // 0x30 crashed_sprite_base

        //static VehicleCrashParticle* create(loc16 loc);
    };

    struct ExplosionCloud : MiscBase
    {
        uint8_t pad_20[0x28 - 0x20];
        uint16_t frame; // 0x28

        //static ExplosionCloud* create(loc16 loc);
    };

    struct Splash : MiscBase
    {
        uint8_t pad_20[0x28 - 0x20];
        uint16_t frame; // 0x28

        //static Splash* create(loc16 loc);
    };

    struct Fireball : MiscBase
    {
        uint8_t pad_20[0x28 - 0x20];
        uint16_t frame; // 0x28

        //static Fireball* create(loc16 loc);
    };

    struct ExplosionSmoke : MiscBase
    {
        uint8_t pad_20[0x28 - 0x20];
        uint16_t frame; // 0x28

        //static ExplosionSmoke* create(loc16 loc);
    };

    struct Smoke : MiscBase
    {
        uint8_t pad_20[0x28 - 0x20];
        uint16_t frame; // 0x28

        static Smoke* create(loc16 loc);
    };
#pragma pack(pop)
}
