#include "PaintMiscEntity.h"
#include "../CompanyManager.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/Tile.h"
#include "Paint.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Paint
{
    static void paintExhaustEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 1)
        {
            return;
        }
        Exhaust* exhaustObject = base->as_exhaust();

        SteamObject* steamObject = exhaustObject->object();

        uint8_t* edi = (exhaustObject->object_id & 0x80) == 0 ? steamObject->var_16 : steamObject->var_1A;
        uint32_t imageId = edi[2 * exhaustObject->var_26];
        imageId = imageId + steamObject->baseImageId + steamObject->var_0A;

        if ((steamObject->var_08 & 8) != 0)
        {
            session.addToPlotListAsParent(imageId, { 0, 0, base->z }, { 1, 1, 0 });
        }
        else
        {
            session.addToPlotListAsParent(imageId, { 0, 0, base->z }, { 1, 1, 0 }, { -12, -12, base->z });
        }
    }

    static void paintRedGreenCurrencyEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        int8_t* wiggleYOffsets = reinterpret_cast<int8_t*>(0x4FAAC8);
        if (dpi->zoom_level > 1)
        {
            return;
        }
        MoneyEffect* moneyEffect = base->asRedGreenCurrency();
        const string_id stringId = moneyEffect->amount >= 0 ? StringIds::format_currency_income_green : StringIds::format_currency_expense_red_negative;
        uint32_t currencyAmount = abs(moneyEffect->amount);
        int8_t* yOffsets = &wiggleYOffsets[moneyEffect->wiggle];

        session.addToStringPlotList(currencyAmount, stringId, moneyEffect->y, moneyEffect->z, yOffsets, moneyEffect->offsetX);
    }

    static void paintWindowCurrencyEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        int8_t* wiggleYOffsets = reinterpret_cast<int8_t*>(0x4FAAC8);
        if (dpi->zoom_level > 1)
        {
            return;
        }
        MoneyEffect* moneyEffect = base->asWindowCurrency();
        const string_id stringId = moneyEffect->amount >= 0 ? StringIds::format_currency_income_in_company_colour : StringIds::format_currency_expense_in_company_colour_negative;
        uint32_t currencyAmount = abs(moneyEffect->amount);
        int8_t* yOffsets = &wiggleYOffsets[moneyEffect->wiggle];
        uint16_t companyColour = CompanyManager::getCompanyColour(moneyEffect->var_2E);

        session.addToStringPlotList(currencyAmount, stringId, moneyEffect->y, moneyEffect->z, yOffsets, moneyEffect->offsetX, companyColour);
    }

    static void paintVehicleCrashParticleEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level != 0)
        {
            return;
        }

        VehicleCrashParticle* particle = base->asVehicleCrashParticle();

        loco_global<int32_t[5], 0x4FAAB4> vehicle_particle_base_sprites;
        uint32_t imageId = vehicle_particle_base_sprites[particle->crashedSpriteBase] + particle->frame / 256;
        imageId = Gfx::recolour2(imageId, particle->colourScheme);

        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    static void paintExplosionCloudEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 2)
        {
            return;
        }
        ExplosionCloud* particle = base->asExplosionCloud();
        uint32_t imageId = 3372 + (particle->frame / 256);
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    static void paintSparkEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 2)
        {
            return;
        }
        Spark* particle = base->asSpark();
        uint32_t imageId = 3421 + (particle->frame / 256);
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    static void paintFireballEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 2)
        {
            return;
        }
        Fireball* particle = base->asFireball();
        uint32_t imageId = 3390 + (particle->frame / 256);
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    static void paintExplosionSmokeEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 1)
        {
            return;
        }
        ExplosionSmoke* particle = base->asExplosionSmoke();
        uint32_t imageId = 3362 + (particle->frame / 256);
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    static void paintSmokeEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 1)
        {
            return;
        }
        Smoke* particle = base->as_smoke();
        uint32_t imageId = 3465 + (particle->frame / 256);
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 });
    }

    // 0x00440325
    void paintMiscEntity(PaintSession& session, MiscBase* base)
    {
        switch (base->getSubType())
        {
            case MiscThingType::exhaust: // 0
            {
                paintExhaustEntity(session, base);
                break;
            }

            case MiscThingType::redGreenCurrency: // 1
            {
                paintRedGreenCurrencyEntity(session, base);
                break;
            }

            case MiscThingType::windowCurrency: // 2
            {
                paintWindowCurrencyEntity(session, base);
                break;
            }

            case MiscThingType::vehicleCrashParticle: // 3
            {
                paintVehicleCrashParticleEntity(session, base);
                break;
            }

            case MiscThingType::explosionCloud: // 4
            {
                paintExplosionCloudEntity(session, base);
                break;
            }

            case MiscThingType::spark: // 5
            {
                paintSparkEntity(session, base);
                break;
            }

            case MiscThingType::fireball: // 6
            {
                paintFireballEntity(session, base);
                break;
            }

            case MiscThingType::explosionSmoke: // 7
            {
                paintExplosionSmokeEntity(session, base);
                break;
            }

            case MiscThingType::smoke: // 8
            {
                paintSmokeEntity(session, base);
                break;
            }
        }
    }
}
