#include "PaintMiscEntity.h"
#include "../CompanyManager.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/Tile.h"
#include "Paint.h"
#include <cassert>

using namespace OpenLoco::Interop;

namespace OpenLoco::Paint
{
    static void paintExhaustEntity(PaintSession& session, Exhaust* exhaustObject)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 1)
        {
            return;
        }
        SteamObject* steamObject = exhaustObject->object();

        uint8_t* edi = (exhaustObject->object_id & 0x80) == 0 ? steamObject->var_16 : steamObject->var_1A;
        uint32_t imageId = edi[2 * exhaustObject->var_26];
        imageId = imageId + steamObject->baseImageId + steamObject->var_0A;

        if ((steamObject->var_08 & 8) != 0)
        {
            session.addToPlotListAsParent(imageId, { 0, 0, exhaustObject->z }, { 1, 1, 0 });
        }
        else
        {
            session.addToPlotListAsParent(imageId, { 0, 0, exhaustObject->z }, { 1, 1, 0 }, { -12, -12, exhaustObject->z });
        }
    }

    static void paintRedGreenCurrencyEntity(PaintSession& session, MoneyEffect* moneyEffect)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        int8_t* wiggleYOffsets = reinterpret_cast<int8_t*>(0x4FAAC8);
        if (dpi->zoom_level > 1)
        {
            return;
        }
        const string_id stringId = moneyEffect->amount >= 0 ? StringIds::format_currency_income_green : StringIds::format_currency_expense_red_negative;
        uint32_t currencyAmount = abs(moneyEffect->amount);
        int8_t* yOffsets = &wiggleYOffsets[moneyEffect->wiggle];

        session.addToStringPlotList(currencyAmount, stringId, moneyEffect->y, moneyEffect->z, yOffsets, moneyEffect->offsetX);
    }

    static void paintWindowCurrencyEntity(PaintSession& session, MoneyEffect* moneyEffect)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        int8_t* wiggleYOffsets = reinterpret_cast<int8_t*>(0x4FAAC8);
        if (dpi->zoom_level > 1)
        {
            return;
        }
        const string_id stringId = moneyEffect->amount >= 0 ? StringIds::format_currency_income_in_company_colour : StringIds::format_currency_expense_in_company_colour_negative;
        uint32_t currencyAmount = abs(moneyEffect->amount);
        int8_t* yOffsets = &wiggleYOffsets[moneyEffect->wiggle];
        uint16_t companyColour = CompanyManager::getCompanyColour(moneyEffect->var_2E);

        session.addToStringPlotList(currencyAmount, stringId, moneyEffect->y, moneyEffect->z, yOffsets, moneyEffect->offsetX, companyColour);
    }

    static void paintVehicleCrashParticleEntity(PaintSession& session, VehicleCrashParticle* particle)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level != 0)
        {
            return;
        }

        loco_global<int32_t[5], 0x4FAAB4> vehicle_particle_base_sprites;
        uint32_t imageId = vehicle_particle_base_sprites[particle->crashedSpriteBase] + particle->frame / 256;
        imageId = Gfx::recolour2(imageId, particle->colourScheme);

        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    static void paintExplosionCloudEntity(PaintSession& session, ExplosionCloud* particle)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 2)
        {
            return;
        }

        static const std::array<uint32_t, 18> explosionCloudImageIds = {
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

        assert(static_cast<size_t>(particle->frame / 256) < explosionCloudImageIds.size());
        uint32_t imageId = explosionCloudImageIds.at(particle->frame / 256);
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    static void paintSparkEntity(PaintSession& session, Spark* particle)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 2)
        {
            return;
        }

        static const std::array<uint32_t, 44> sparkImageIds = {
            ImageIds::spark_00,
            ImageIds::spark_01,
            ImageIds::spark_02,
            ImageIds::spark_03,
            ImageIds::spark_04,
            ImageIds::spark_05,
            ImageIds::spark_06,
            ImageIds::spark_07,
            ImageIds::spark_08,
            ImageIds::spark_09,
            ImageIds::spark_10,
            ImageIds::spark_11,
            ImageIds::spark_12,
            ImageIds::spark_13,
            ImageIds::spark_14,
            ImageIds::spark_15,
            ImageIds::spark_16,
            ImageIds::spark_17,
            ImageIds::spark_18,
            ImageIds::spark_19,
            ImageIds::spark_20,
            ImageIds::spark_21,
            ImageIds::spark_22,
            ImageIds::spark_23,
            ImageIds::spark_24,
            ImageIds::spark_25,
            ImageIds::spark_26,
            ImageIds::spark_27,
            ImageIds::spark_28,
            ImageIds::spark_29,
            ImageIds::spark_30,
            ImageIds::spark_31,
            ImageIds::spark_32,
            ImageIds::spark_33,
            ImageIds::spark_34,
            ImageIds::spark_35,
            ImageIds::spark_36,
            ImageIds::spark_37,
            ImageIds::spark_38,
            ImageIds::spark_39,
            ImageIds::spark_40,
            ImageIds::spark_41,
            ImageIds::spark_42,
            ImageIds::spark_43
        };

        assert(static_cast<size_t>(particle->frame / 256) < sparkImageIds.size());
        uint32_t imageId = sparkImageIds.at(particle->frame / 256);
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    static void paintFireballEntity(PaintSession& session, Fireball* particle)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 2)
        {
            return;
        }

        static const std::array<uint32_t, 31> fireballImageIds = {
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

        assert(static_cast<size_t>(particle->frame / 256) < fireballImageIds.size());
        uint32_t imageId = fireballImageIds.at(particle->frame / 256);
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    static void paintExplosionSmokeEntity(PaintSession& session, ExplosionSmoke* particle)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 1)
        {
            return;
        }

        static const std::array<uint32_t, 10> explosionSmokeImageIds = {
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

        assert(static_cast<size_t>(particle->frame / 256) < explosionSmokeImageIds.size());
        uint32_t imageId = explosionSmokeImageIds.at(particle->frame / 256);
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    static void paintSmokeEntity(PaintSession& session, Smoke* particle)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 1)
        {
            return;
        }

        static const std::array<uint32_t, 12> smokeImageIds = {
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

        assert(static_cast<size_t>(particle->frame / 256) < smokeImageIds.size());
        uint32_t imageId = smokeImageIds.at(particle->frame / 256);
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    // 0x00440325
    void paintMiscEntity(PaintSession& session, MiscBase* base)
    {
        switch (base->getSubType())
        {
            case MiscThingType::exhaust: // 0
            {
                paintExhaustEntity(session, base->asExhaust());
                break;
            }

            case MiscThingType::redGreenCurrency: // 1
            {
                paintRedGreenCurrencyEntity(session, base->asRedGreenCurrency());
                break;
            }

            case MiscThingType::windowCurrency: // 2
            {
                paintWindowCurrencyEntity(session, base->asWindowCurrency());
                break;
            }

            case MiscThingType::vehicleCrashParticle: // 3
            {
                paintVehicleCrashParticleEntity(session, base->asVehicleCrashParticle());
                break;
            }

            case MiscThingType::explosionCloud: // 4
            {
                paintExplosionCloudEntity(session, base->asExplosionCloud());
                break;
            }

            case MiscThingType::spark: // 5
            {
                paintSparkEntity(session, base->asSpark());
                break;
            }

            case MiscThingType::fireball: // 6
            {
                paintFireballEntity(session, base->asFireball());
                break;
            }

            case MiscThingType::explosionSmoke: // 7
            {
                paintExplosionSmokeEntity(session, base->asExplosionSmoke());
                break;
            }

            case MiscThingType::smoke: // 8
            {
                paintSmokeEntity(session, base->asSmoke());
                break;
            }
        }
    }
}
