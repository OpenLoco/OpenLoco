#include "Effects/Effect.h"
#include "Effects/ExhaustEffect.h"
#include "Effects/ExplosionEffect.h"
#include "Effects/ExplosionSmokeEffect.h"
#include "Effects/FireballEffect.h"
#include "Effects/MoneyEffect.h"
#include "Effects/SmokeEffect.h"
#include "Effects/SplashEffect.h"
#include "Effects/VehicleCrashEffect.h"

namespace OpenLoco
{
    // 0x004405CD
    void EffectEntity::tick()
    {
        switch (getSubType())
        {
            case EffectType::exhaust:
                asExhaust()->tick();
                break;
            case EffectType::redGreenCurrency:
                asRedGreenCurrency()->tick();
                break;
            case EffectType::windowCurrency:
                asWindowCurrency()->tick();
                break;
            case EffectType::vehicleCrashParticle:
                asVehicleCrashParticle()->tick();
                break;
            case EffectType::explosionCloud:
                asExplosionCloud()->tick();
                break;
            case EffectType::splash:
                asSplash()->tick();
                break;
            case EffectType::fireball:
                asFireball()->tick();
                break;
            case EffectType::explosionSmoke:
                asExplosionSmoke()->tick();
                break;
            case EffectType::smoke:
                asSmoke()->tick();
                break;
        }
    }

}
