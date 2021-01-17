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
        if (dpi->zoom_level > 1) // 00440331-0044033C
        {
            return;
        }
        Exhaust* exhaustObject = base->as_exhaust();

        SteamObject* steamObject = exhaustObject->object(); // 00440342-00440349
        static_assert(offsetof(SteamObject, var_16) == 0x16);
        static_assert(offsetof(SteamObject, var_1A) == 0x1A);
        static_assert(offsetof(SteamObject, sound_effect) == 0x1E);

        uint8_t* edi = (exhaustObject->object_id & 0x80) == 0 ? steamObject->var_16 : steamObject->var_1A; // 00440354-0044035D
        uint32_t imageId = edi[2 * exhaustObject->var_26];                                                 // 00440350, 00440360
        imageId = imageId + steamObject->baseImageId + steamObject->var_0A;                                // 00440364-00440367

        if ((steamObject->var_08 & 8) != 0) // 0044036A-00440370
        {
            session.addToPlotListAsParent(imageId, { 0, 0, base->z }, { 1, 1, 0 }); // 00440372-00440386
        }
        else
        {
            session.addToPlotListAsParent(imageId, { 0, 0, base->z }, { 1, 1, 0 }, { -12, -12, base->z }); // 0044038F-004403BC
        }
    }

    static void paintRedGreenCurrencyEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        int8_t* wiggleYOffsets = reinterpret_cast<int8_t*>(0x4FAAC8);
        if (dpi->zoom_level > 1) // 004403C5-004403D0
        {
            return;
        }
        MoneyEffect* moneyEffect = base->asRedGreenCurrency();
        const string_id stringId = moneyEffect->amount >= 0 ? StringIds::format_currency_income_green : StringIds::format_currency_expense_red_negative; // 004403D2-004403DF
        uint32_t currencyAmount = abs(moneyEffect->amount);
        int8_t* yOffsets = &wiggleYOffsets[moneyEffect->wiggle];

        session.addToStringPlotList(currencyAmount, stringId, moneyEffect->y, moneyEffect->z, yOffsets, moneyEffect->offsetX); // 004403F7
    }

    static void paintWindowCurrencyEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        int8_t* wiggleYOffsets = reinterpret_cast<int8_t*>(0x4FAAC8);
        if (dpi->zoom_level > 1) // 00440400-0044040B
        {
            return;
        }
        MoneyEffect* moneyEffect = base->asWindowCurrency();
        const string_id stringId = moneyEffect->amount >= 0 ? StringIds::format_currency_income_in_company_colour : StringIds::format_currency_expense_in_company_colour_negative; // 0044040D-0044041A
        uint32_t currencyAmount = abs(moneyEffect->amount);
        int8_t* yOffsets = &wiggleYOffsets[moneyEffect->wiggle];
        uint16_t companyColour = CompanyManager::getCompanyColour(moneyEffect->var_2E); // 00440428-0044042C

        session.addToStringPlotList(currencyAmount, stringId, moneyEffect->y, moneyEffect->z, yOffsets, moneyEffect->offsetX, companyColour); // 00440445
    }

    static void paintVehicleCrashParticleEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level != 0) // 0044044E-00440459
        {
            return;
        }

        VehicleCrashParticle* particle = base->asVehicleCrashParticle();

        constexpr uint32_t IMAGE_TYPE_REMAP = (1 << 29);
        constexpr uint32_t IMAGE_TYPE_REMAP_2_PLUS = (1u << 31);
        static_assert((IMAGE_TYPE_REMAP | IMAGE_TYPE_REMAP_2_PLUS) == 0x0A0000000);

        loco_global<int32_t[5], 0x4FAAB4> vehicle_particle_base_sprites;
        uint32_t imageId = vehicle_particle_base_sprites[particle->crashedSpriteBase] + particle->frame / 256; // 0044045F-0044046A
        imageId = Gfx::recolour2(imageId, particle->colourScheme);                                             // 00440471-00440487
        // imageId = imageId | (particle->colourScheme.primary << 19) | (particle->colourScheme.secondary << 24) | IMAGE_TYPE_REMAP | IMAGE_TYPE_REMAP_2_PLUS; // 00440471-00440487

        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 }); // 00440489-0044049D
    }

    static void paintExplosionCloudEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 2) // 0044051C-00440527
        {
            return;
        }
        ExplosionCloud* particle = base->asExplosionCloud();
        uint32_t imageId = 3372 + (particle->frame / 256);                          // 0044052D-00440534
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 }); // 0044053A-0044054E
    }

    static void paintSparkEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 2) // 00440557-00440562
        {
            return;
        }
        Spark* particle = base->asSpark();
        uint32_t imageId = 3421 + (particle->frame / 256);                          // 00440568-0044056F
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 }); // 00440575-00440589
    }

    static void paintFireballEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 2) // 00440592-0044059D
        {
            return;
        }
        Fireball* particle = base->asFireball();
        uint32_t imageId = 3390 + (particle->frame / 256);                          // 004405A3-004405AA
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 }); // 004405B0-004405C4
    }

    static void paintExplosionSmokeEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 1) // 004404A6-004404B7
        {
            return;
        }
        ExplosionSmoke* particle = base->asExplosionSmoke();
        uint32_t imageId = 3362 + (particle->frame / 256);                          // 004404B7-004404BE
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 }); // 004404C4-004404D8
    }

    static void paintSmokeEntity(PaintSession& session, MiscBase* base)
    {
        Gfx::drawpixelinfo_t* dpi = session.getContext();
        if (dpi->zoom_level > 1) // 004404E1-004404EC
        {
            return;
        }
        Smoke* particle = base->as_smoke();
        uint32_t imageId = 3465 + (particle->frame / 256);                          // 004404F2-004404F9
        session.addToPlotListAsParent(imageId, { 0, 0, particle->z }, { 1, 1, 0 }); // 004404FF-00440513
    }

    // 0x00440325
    void paintMiscEntity(PaintSession& session, MiscBase* base)
    {
        switch (base->getSubType()) // 00440326-0044032A
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
