#include "Effect.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Entities/EntityManager.h"
#include "ExhaustEffect.h"
#include "ExplosionEffect.h"
#include "ExplosionSmokeEffect.h"
#include "FireballEffect.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "MoneyEffect.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrainStationObject.h"
#include "SceneManager.h"
#include "SmokeEffect.h"
#include "SplashEffect.h"
#include "Ui/WindowManager.h"
#include "VehicleCrashEffect.h"
#include "ViewportManager.h"

using namespace OpenLoco::Interop;

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
