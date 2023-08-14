#include "PaintEffectEntity.h"
#include "Config.h"
#include "Effects/ExhaustEffect.h"
#include "Effects/ExplosionEffect.h"
#include "Effects/ExplosionSmokeEffect.h"
#include "Effects/FireballEffect.h"
#include "Effects/MoneyEffect.h"
#include "Effects/SmokeEffect.h"
#include "Effects/SplashEffect.h"
#include "Effects/VehicleCrashEffect.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Localisation/StringIds.h"
#include "Map/Tile.h"
#include "Objects/SteamObject.h"
#include "Paint.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>
#include <unordered_map>

using namespace OpenLoco::Interop;

namespace OpenLoco::Paint
{
    // 004FAAC8
    // This is an array of 22 signed int8 elements which are repeating 20 times, can be optimized after C++ implementation of 0x004FD120 - addToStringPlotList
    // clang-format off
    static const int8_t kWiggleYOffsets[440] = {
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
        0, 1, 2, 2, 3, 3, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -2, -2, -1,
    };
    // clang-format on

    // 0x00440331
    static void paintExhaustEntity(PaintSession& session, Exhaust* exhaustEntity)
    {
        Gfx::RenderTarget* rt = session.getRenderTarget();
        if (rt->zoomLevel > 1)
        {
            return;
        }
        const auto* steamObject = exhaustEntity->getObject();

        const auto& frameInfo = steamObject->getFramesInfo(exhaustEntity->isSubObjType1());
        const auto imageId = ImageId{ frameInfo.second[exhaustEntity->frameNum].imageOffset + steamObject->baseImageId + steamObject->var_0A };

        if (!steamObject->hasFlags(SteamObjectFlags::unk3))
        {
            session.addToPlotListAsParent(imageId, { 0, 0, exhaustEntity->position.z }, { 1, 1, 0 });
        }
        else
        {
            session.addToPlotListAsParent(imageId, { 0, 0, exhaustEntity->position.z }, { -12, -12, exhaustEntity->position.z }, { 24, 24, 0 });
        }
    }

    // 0x004403C5
    static void paintRedGreenCurrencyEntity(PaintSession& session, MoneyEffect* moneyEffect)
    {
        Gfx::RenderTarget* rt = session.getRenderTarget();
        if (rt->zoomLevel > 1)
        {
            return;
        }
        const string_id stringId = moneyEffect->amount >= 0 ? StringIds::format_currency_income_green : StringIds::format_currency_expense_red_negative;
        uint32_t currencyAmount = abs(moneyEffect->amount);
        const int8_t* yOffsets = &kWiggleYOffsets[moneyEffect->wiggle];

        session.addToStringPlotList(currencyAmount, stringId, moneyEffect->position.z, moneyEffect->offsetX, yOffsets, 0);
    }

    // 0x00440400
    static void paintWindowCurrencyEntity(PaintSession& session, MoneyEffect* moneyEffect)
    {
        if (!Config::get().cashPopupRendering)
        {
            return;
        }

        Gfx::RenderTarget* rt = session.getRenderTarget();
        if (rt->zoomLevel > 1)
        {
            return;
        }
        const string_id stringId = moneyEffect->amount >= 0 ? StringIds::format_currency_income_in_company_colour : StringIds::format_currency_expense_in_company_colour_negative;
        uint32_t currencyAmount = abs(moneyEffect->amount);
        const int8_t* yOffsets = &kWiggleYOffsets[moneyEffect->wiggle];
        auto companyColour = CompanyManager::getCompanyColour(moneyEffect->var_2E);

        session.addToStringPlotList(currencyAmount, stringId, moneyEffect->position.z, moneyEffect->offsetX, yOffsets, enumValue(companyColour));
    }

    // 0x0044044E
    static void paintVehicleCrashParticleEntity(PaintSession& session, VehicleCrashParticle* particle)
    {
        Gfx::RenderTarget* rt = session.getRenderTarget();
        if (rt->zoomLevel != 0)
        {
            return;
        }

        static const std::unordered_map<int, const std::array<uint32_t, 12>> kVehicleCrashParticleImageIds = {
            { 0, { ImageIds::vehicle_crash_0_00, ImageIds::vehicle_crash_0_01, ImageIds::vehicle_crash_0_02, ImageIds::vehicle_crash_0_03, ImageIds::vehicle_crash_0_04, ImageIds::vehicle_crash_0_05, ImageIds::vehicle_crash_0_06, ImageIds::vehicle_crash_0_07, ImageIds::vehicle_crash_0_08, ImageIds::vehicle_crash_0_09, ImageIds::vehicle_crash_0_10, ImageIds::vehicle_crash_0_11 } },
            { 1, { ImageIds::vehicle_crash_1_00, ImageIds::vehicle_crash_1_01, ImageIds::vehicle_crash_1_02, ImageIds::vehicle_crash_1_03, ImageIds::vehicle_crash_1_04, ImageIds::vehicle_crash_1_05, ImageIds::vehicle_crash_1_06, ImageIds::vehicle_crash_1_07, ImageIds::vehicle_crash_1_08, ImageIds::vehicle_crash_1_09, ImageIds::vehicle_crash_1_10, ImageIds::vehicle_crash_1_11 } },
            { 2, { ImageIds::vehicle_crash_2_00, ImageIds::vehicle_crash_2_01, ImageIds::vehicle_crash_2_02, ImageIds::vehicle_crash_2_03, ImageIds::vehicle_crash_2_04, ImageIds::vehicle_crash_2_05, ImageIds::vehicle_crash_2_06, ImageIds::vehicle_crash_2_07, ImageIds::vehicle_crash_2_08, ImageIds::vehicle_crash_2_09, ImageIds::vehicle_crash_2_10, ImageIds::vehicle_crash_2_11 } },
            { 3, { ImageIds::vehicle_crash_3_00, ImageIds::vehicle_crash_3_01, ImageIds::vehicle_crash_3_02, ImageIds::vehicle_crash_3_03, ImageIds::vehicle_crash_3_04, ImageIds::vehicle_crash_3_05, ImageIds::vehicle_crash_3_06, ImageIds::vehicle_crash_3_07, ImageIds::vehicle_crash_3_08, ImageIds::vehicle_crash_3_09, ImageIds::vehicle_crash_3_10, ImageIds::vehicle_crash_3_11 } },
            { 4, { ImageIds::vehicle_crash_4_00, ImageIds::vehicle_crash_4_01, ImageIds::vehicle_crash_4_02, ImageIds::vehicle_crash_4_03, ImageIds::vehicle_crash_4_04, ImageIds::vehicle_crash_4_05, ImageIds::vehicle_crash_4_06, ImageIds::vehicle_crash_4_07, ImageIds::vehicle_crash_4_08, ImageIds::vehicle_crash_4_09, ImageIds::vehicle_crash_4_10, ImageIds::vehicle_crash_4_11 } }
        };

        assert(static_cast<size_t>(particle->frame / 256) < 12);
        assert((particle->crashedSpriteBase) < 5);

        const auto imageId = ImageId{ kVehicleCrashParticleImageIds.at(particle->crashedSpriteBase).at(particle->frame / 256), particle->colourScheme };

        session.addToPlotListAsParent(imageId, { 0, 0, particle->position.z }, { 1, 1, 0 });
    }

    // 0x0044051C
    static void paintExplosionCloudEntity(PaintSession& session, ExplosionCloud* particle)
    {
        Gfx::RenderTarget* rt = session.getRenderTarget();
        if (rt->zoomLevel > 2)
        {
            return;
        }

        static const std::array<uint32_t, 18> kExplosionCloudImageIds = {
            ImageIds::explosion_cloud_00,
            ImageIds::explosion_cloud_01,
            ImageIds::explosion_cloud_02,
            ImageIds::explosion_cloud_03,
            ImageIds::explosion_cloud_04,
            ImageIds::explosion_cloud_05,
            ImageIds::explosion_cloud_06,
            ImageIds::explosion_cloud_07,
            ImageIds::explosion_cloud_08,
            ImageIds::explosion_cloud_09,
            ImageIds::explosion_cloud_10,
            ImageIds::explosion_cloud_11,
            ImageIds::explosion_cloud_12,
            ImageIds::explosion_cloud_13,
            ImageIds::explosion_cloud_14,
            ImageIds::explosion_cloud_15,
            ImageIds::explosion_cloud_16,
            ImageIds::explosion_cloud_17
        };

        assert(static_cast<size_t>(particle->frame / 256) < kExplosionCloudImageIds.size());
        const auto imageId = ImageId{ kExplosionCloudImageIds.at(particle->frame / 256) };
        session.addToPlotListAsParent(imageId, { 0, 0, particle->position.z }, { 1, 1, 0 });
    }

    // 0x00440557
    static void paintSplashEntity(PaintSession& session, Splash* particle)
    {
        Gfx::RenderTarget* rt = session.getRenderTarget();
        if (rt->zoomLevel > 2)
        {
            return;
        }

        static const std::array<uint32_t, 28> kSplashImageIds = {
            ImageIds::splash_00,
            ImageIds::splash_01,
            ImageIds::splash_02,
            ImageIds::splash_03,
            ImageIds::splash_04,
            ImageIds::splash_05,
            ImageIds::splash_06,
            ImageIds::splash_07,
            ImageIds::splash_08,
            ImageIds::splash_09,
            ImageIds::splash_10,
            ImageIds::splash_11,
            ImageIds::splash_12,
            ImageIds::splash_13,
            ImageIds::splash_14,
            ImageIds::splash_15,
            ImageIds::splash_16,
            ImageIds::splash_17,
            ImageIds::splash_18,
            ImageIds::splash_19,
            ImageIds::splash_20,
            ImageIds::splash_21,
            ImageIds::splash_22,
            ImageIds::splash_23,
            ImageIds::splash_24,
            ImageIds::splash_25,
            ImageIds::splash_26,
            ImageIds::splash_27
        };

        assert(static_cast<size_t>(particle->frame / 256) < kSplashImageIds.size());
        const auto imageId = ImageId{ kSplashImageIds.at(particle->frame / 256) };
        session.addToPlotListAsParent(imageId, { 0, 0, particle->position.z }, { 1, 1, 0 });
    }

    // 0x00440592
    static void paintFireballEntity(PaintSession& session, Fireball* particle)
    {
        Gfx::RenderTarget* rt = session.getRenderTarget();
        if (rt->zoomLevel > 2)
        {
            return;
        }

        static const std::array<uint32_t, 31> kFireballImageIds = {
            ImageIds::fireball_00,
            ImageIds::fireball_01,
            ImageIds::fireball_02,
            ImageIds::fireball_03,
            ImageIds::fireball_04,
            ImageIds::fireball_05,
            ImageIds::fireball_06,
            ImageIds::fireball_07,
            ImageIds::fireball_08,
            ImageIds::fireball_09,
            ImageIds::fireball_10,
            ImageIds::fireball_11,
            ImageIds::fireball_12,
            ImageIds::fireball_13,
            ImageIds::fireball_14,
            ImageIds::fireball_15,
            ImageIds::fireball_16,
            ImageIds::fireball_17,
            ImageIds::fireball_18,
            ImageIds::fireball_19,
            ImageIds::fireball_20,
            ImageIds::fireball_21,
            ImageIds::fireball_22,
            ImageIds::fireball_23,
            ImageIds::fireball_24,
            ImageIds::fireball_25,
            ImageIds::fireball_26,
            ImageIds::fireball_27,
            ImageIds::fireball_28,
            ImageIds::fireball_29,
            ImageIds::fireball_30
        };

        assert(static_cast<size_t>(particle->frame / 256) < kFireballImageIds.size());
        const auto imageId = ImageId{ kFireballImageIds.at(particle->frame / 256) };
        session.addToPlotListAsParent(imageId, { 0, 0, particle->position.z }, { 1, 1, 0 });
    }

    // 0x004404A6
    static void paintExplosionSmokeEntity(PaintSession& session, ExplosionSmoke* particle)
    {
        Gfx::RenderTarget* rt = session.getRenderTarget();
        if (rt->zoomLevel > 1)
        {
            return;
        }

        static const std::array<uint32_t, 10> kExplosionSmokeImageIds = {
            ImageIds::explosion_smoke_00,
            ImageIds::explosion_smoke_01,
            ImageIds::explosion_smoke_02,
            ImageIds::explosion_smoke_03,
            ImageIds::explosion_smoke_04,
            ImageIds::explosion_smoke_05,
            ImageIds::explosion_smoke_06,
            ImageIds::explosion_smoke_07,
            ImageIds::explosion_smoke_08,
            ImageIds::explosion_smoke_09
        };

        assert(static_cast<size_t>(particle->frame / 256) < kExplosionSmokeImageIds.size());
        const auto imageId = ImageId{ kExplosionSmokeImageIds.at(particle->frame / 256) };
        session.addToPlotListAsParent(imageId, { 0, 0, particle->position.z }, { 1, 1, 0 });
    }

    // 0x004404E1
    static void paintSmokeEntity(PaintSession& session, Smoke* particle)
    {
        Gfx::RenderTarget* rt = session.getRenderTarget();
        if (rt->zoomLevel > 1)
        {
            return;
        }

        static const std::array<uint32_t, 12> kSmokeImageIds = {
            ImageIds::smoke_00,
            ImageIds::smoke_01,
            ImageIds::smoke_02,
            ImageIds::smoke_03,
            ImageIds::smoke_04,
            ImageIds::smoke_05,
            ImageIds::smoke_06,
            ImageIds::smoke_07,
            ImageIds::smoke_08,
            ImageIds::smoke_09,
            ImageIds::smoke_10,
            ImageIds::smoke_11
        };

        assert(static_cast<size_t>(particle->frame / 256) < kSmokeImageIds.size());
        const auto imageId = ImageId{ kSmokeImageIds.at(particle->frame / 256) };
        session.addToPlotListAsParent(imageId, { 0, 0, particle->position.z }, { 1, 1, 0 });
    }

    // 0x00440325
    void paintMiscEntity(PaintSession& session, EffectEntity* base)
    {
        switch (base->getSubType())
        {
            case EffectType::exhaust: // 0
            {
                paintExhaustEntity(session, base->asExhaust());
                break;
            }

            case EffectType::redGreenCurrency: // 1
            {
                paintRedGreenCurrencyEntity(session, base->asRedGreenCurrency());
                break;
            }

            case EffectType::windowCurrency: // 2
            {
                paintWindowCurrencyEntity(session, base->asWindowCurrency());
                break;
            }

            case EffectType::vehicleCrashParticle: // 3
            {
                paintVehicleCrashParticleEntity(session, base->asVehicleCrashParticle());
                break;
            }

            case EffectType::explosionCloud: // 4
            {
                paintExplosionCloudEntity(session, base->asExplosionCloud());
                break;
            }

            case EffectType::splash: // 5
            {
                paintSplashEntity(session, base->asSplash());
                break;
            }

            case EffectType::fireball: // 6
            {
                paintFireballEntity(session, base->asFireball());
                break;
            }

            case EffectType::explosionSmoke: // 7
            {
                paintExplosionSmokeEntity(session, base->asExplosionSmoke());
                break;
            }

            case EffectType::smoke: // 8
            {
                paintSmokeEntity(session, base->asSmoke());
                break;
            }
        }
    }
}
