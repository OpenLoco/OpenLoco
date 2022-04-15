#pragma once

#include "../Economy/Currency.h"
#include "../Map/Map.hpp"
#include "../Objects/SteamObject.h"
#include "Entity.h"

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

    enum class MiscEntityType : uint8_t
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
    struct MiscBase : EntityBase
    {
        static constexpr auto kBaseType = EntityBaseType::misc;

    private:
        template<typename TType, MiscEntityType TClass>
        TType* as() const
        {
            return getSubType() == TClass ? (TType*)this : nullptr;
        }

    public:
        MiscEntityType getSubType() const { return MiscEntityType(EntityBase::getSubType()); }
        void setSubType(const MiscEntityType newType) { EntityBase::setSubType(static_cast<uint8_t>(newType)); }
        Exhaust* asExhaust() const { return as<Exhaust, MiscEntityType::exhaust>(); }
        MoneyEffect* asRedGreenCurrency() const { return as<MoneyEffect, MiscEntityType::redGreenCurrency>(); }
        MoneyEffect* asWindowCurrency() const { return as<MoneyEffect, MiscEntityType::windowCurrency>(); }
        VehicleCrashParticle* asVehicleCrashParticle() const { return as<VehicleCrashParticle, MiscEntityType::vehicleCrashParticle>(); }
        ExplosionCloud* asExplosionCloud() const { return as<ExplosionCloud, MiscEntityType::explosionCloud>(); }
        Splash* asSplash() const { return as<Splash, MiscEntityType::splash>(); }
        Fireball* asFireball() const { return as<Fireball, MiscEntityType::fireball>(); }
        ExplosionSmoke* asExplosionSmoke() const { return as<ExplosionSmoke, MiscEntityType::explosionSmoke>(); }
        Smoke* asSmoke() const { return as<Smoke, MiscEntityType::smoke>(); }
        void update();
    };

    struct Exhaust : MiscBase
    {
        uint8_t pad_24[0x26 - 0x24];
        int16_t var_26;
        int16_t var_28;
        uint8_t pad_2A[0x32 - 0x2A];
        int16_t var_32;
        int16_t var_34;
        int16_t var_36;
        uint8_t pad_38[0x49 - 0x38];
        uint8_t objectId; // 0x49

        const SteamObject* getObject() const;
        void update();

        static Exhaust* create(Map::Pos3 loc, uint8_t type);
    };
    static_assert(sizeof(Exhaust) == 0x4A);

    struct MoneyEffect : MiscBase
    {
        static constexpr uint32_t kLifetime = 160;        // windowCurrency
        static constexpr uint32_t kRedGreenLifetime = 55; // redGreen (RCT2 legacy) Note: due to delay it is technically 55 * 2

        uint8_t pad_24[0x26 - 0x24];
        union
        {
            uint16_t frame;     // 0x26
            uint16_t moveDelay; // 0x26 Note: this is only used by redGreen money (RCT2 legacy)
        };
        uint16_t numMovements; // 0x28 Note: this is only used by redGreen money (RCT2 legacy)
        int32_t amount;        // 0x2A - currency amount in British pounds - different currencies are probably getting recalculated
        CompanyId var_2E;      // company colour?
        uint8_t pad_2F[0x44 - 0x2F];
        int16_t offsetX; // 0x44
        uint16_t wiggle; // 0x46

        void update();

        static MoneyEffect* create(const Map::Pos3& loc, const CompanyId company, const currency32_t amount);
    };
    static_assert(sizeof(MoneyEffect) == 0x48);

    struct VehicleCrashParticle : MiscBase
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28
        uint8_t pad_2A[0x2E - 0x2A];
        ColourScheme colourScheme;  // 0x2E
        uint16_t crashedSpriteBase; // 0x30 crashed_sprite_base

        void update();
    };
    static_assert(sizeof(VehicleCrashParticle) == 0x32);

    struct ExplosionCloud : MiscBase
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();
    };
    static_assert(sizeof(ExplosionCloud) == 0x2A);

    struct Splash : MiscBase
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();
    };
    static_assert(sizeof(Splash) == 0x2A);

    struct Fireball : MiscBase
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();
    };
    static_assert(sizeof(Fireball) == 0x2A);

    struct ExplosionSmoke : MiscBase
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();

        static ExplosionSmoke* create(const Map::Pos3& loc);
    };
    static_assert(sizeof(ExplosionSmoke) == 0x2A);

    struct Smoke : MiscBase
    {
        uint8_t pad_24[0x28 - 0x24];
        uint16_t frame; // 0x28

        void update();

        static Smoke* create(Map::Pos3 loc);
    };
    static_assert(sizeof(Smoke) == 0x2A);
#pragma pack(pop)
}
