#include "Effect.h"
#include "ExhaustEffect.h"
#include "ExplosionEffect.h"
#include "ExplosionSmokeEffect.h"
#include "FireballEffect.h"
#include "MoneyEffect.h"
#include "SmokeEffect.h"
#include "SplashEffect.h"
#include "VehicleCrashEffect.h"

namespace OpenLoco
{
    // 0x004405CD
    void EffectEntity::update()
    {
        switch (getSubType())
        {
            case EffectType::exhaust:
                asExhaust()->update();
                break;
            case EffectType::redGreenCurrency:
                asRedGreenCurrency()->update();
                break;
            case EffectType::windowCurrency:
                asWindowCurrency()->update();
                break;
            case EffectType::vehicleCrashParticle:
                asVehicleCrashParticle()->update();
                break;
            case EffectType::explosionCloud:
                asExplosionCloud()->update();
                break;
            case EffectType::splash:
                asSplash()->update();
                break;
            case EffectType::fireball:
                asFireball()->update();
                break;
            case EffectType::explosionSmoke:
                asExplosionSmoke()->update();
                break;
            case EffectType::smoke:
                asSmoke()->update();
                break;
        }
    }

}
