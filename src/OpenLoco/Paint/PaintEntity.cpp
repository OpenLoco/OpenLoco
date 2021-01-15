#include "PaintEntity.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../Things/Misc.h"
#include "../Things/ThingManager.h"
#include "Paint.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    // 0x004B0CCE
    static void paintVehicleEntity(PaintSession& session, vehicle_base* base)
    {
        registers regs{};
        regs.ax = base->x;
        regs.cx = base->y;
        regs.dx = base->z;
        regs.ebx = (base->sprite_yaw + (session.getRotation() << 4)) & 0x3F;
        regs.esi = reinterpret_cast<int32_t>(base);
        call(0x004B0CCE, regs);
    }

    // 0x00440325
    static void paintMiscEntity(PaintSession& session, MiscBase* base)
    {
        registers regs{};
        regs.ax = base->x;
        regs.cx = base->y;
        regs.dx = base->z;
        regs.ebx = (base->sprite_yaw + (session.getRotation() << 4)) & 0x3F;
        regs.esi = reinterpret_cast<int32_t>(base);
        //call(0x00440325, regs);
        //return;

        Gfx::drawpixelinfo_t* dpi = session.getContext();

        switch (base->getSubType()) // 00440326-0044032A
        {
            case MiscThingType::exhaust: // 0
            {
                if (dpi->zoom_level > 1) // 00440331-0044033C
                {
                    return;
                }
                exhaust* exhaustObject = base->as_exhaust();
                steam_object* steamObject = ObjectManager::get<steam_object>(exhaustObject->object_id & 0x7f); // 00440342-00440349
                static_assert(offsetof(steam_object, var_16) == 0x16);
                static_assert(offsetof(steam_object, var_1A) == 0x1A);
                static_assert(offsetof(steam_object, sound_effect) == 0x1E);

                uint8_t* edi = reinterpret_cast<uint8_t*>((exhaustObject->object_id & 0x80) == 0 ? steamObject->var_16 : steamObject->var_1A); // 00440354-0044035D
                uint32_t imageId = edi[2 * exhaustObject->var_26];                                                                             // 00440350, 00440360
                imageId = imageId + steamObject->var_0E + steamObject->var_0A;                                                                 // 00440364-00440367

                if ((steamObject->var_08 & 8) != 0) // 0044036A-00440370
                {
                    session.addToPlotList2(imageId, { 0, 0, base->z }, { 1, 1, 0 }); // 00440372-00440386
                }
                else
                {
                    session.addToPlotListAsParent(imageId, { 0, 0, base->z }, { 1, 1, 0 }, { -12, -12, base->z }); // 0044038F-004403BC
                }
                break;
            }

            case MiscThingType::redGreenCurrency: // 1
            {
                if (dpi->zoom_level > 1) // 004403C5-004403D0
                {
                    return;
                }
                MoneyEffect* moneyEffect = base->asRedGreenCurrency();
                const string_id stringId = moneyEffect->amount >= 0 ? StringIds::format_currency_income_green : StringIds::format_currency_expense_red_negative; // 004403D2-004403DF
                uint32_t currencyAmount = abs(moneyEffect->amount);
                uint32_t yOffsets = 0x4FAAC8 + moneyEffect->wiggle; // 004403E3

                session.PaintFloatingMoneyEffect(currencyAmount, stringId, moneyEffect->y, moneyEffect->z, yOffsets, moneyEffect->offsetX); // 004403F7
                break;
            }

            case MiscThingType::windowCurrency: // 2
            {
                if (dpi->zoom_level > 1) // 00440400-0044040B
                {
                    return;
                }
                MoneyEffect* moneyEffect = base->asWindowCurrency();
                const string_id stringId = moneyEffect->amount >= 0 ? StringIds::format_currency_income_in_company_colour : StringIds::format_currency_expense_in_company_colour_negative; // 0044040D-0044041A
                uint32_t currencyAmount = abs(moneyEffect->amount);
                uint32_t yOffsets = 0x4FAAC8 + moneyEffect->wiggle;                             // 0044041E-00440422
                uint16_t companyColour = CompanyManager::getCompanyColour(moneyEffect->var_2E); // 00440428-0044042C
                addr<0xE3F0A8, int16_t>() = companyColour;                                      // 00440434

                session.PaintFloatingMoneyEffect(currencyAmount, stringId, moneyEffect->y, moneyEffect->z, yOffsets, moneyEffect->offsetX); // 00440445
                break;
            }
        }
    }

    template<typename FilterType>
    static void paintEntitiesWithFilter(PaintSession& session, const Map::map_pos& loc, FilterType&& filter)
    {
        auto* dpi = session.getContext();
        if (Config::get().vehicles_min_scale < dpi->zoom_level)
        {
            return;
        }

        if (loc.x >= 0x4000 || loc.y >= 0x4000)
        {
            return;
        }

        ThingManager::ThingTileList entities(loc);
        for (auto* entity : entities)
        {
            // TODO: Create a rect from dpi dims
            auto left = dpi->x;
            auto top = dpi->y;
            auto right = left + dpi->width;
            auto bottom = top + dpi->height;

            // TODO: Create a rect from sprite dims and use a contains function
            if (entity->sprite_top > bottom)
            {
                continue;
            }
            if (entity->sprite_bottom <= top)
            {
                continue;
            }
            if (entity->sprite_left > right)
            {
                continue;
            }
            if (entity->sprite_right <= left)
            {
                continue;
            }
            if (!filter(entity))
            {
                continue;
            }
            session.setCurrentItem(entity);
            session.setEntityPosition({ entity->x, entity->y });
            session.setItemType(InteractionItem::thing);
            switch (entity->base_type)
            {
                case thing_base_type::vehicle:
                    paintVehicleEntity(session, entity->asVehicle());
                    break;
                case thing_base_type::misc:
                    paintMiscEntity(session, entity->asMisc());
                    break;
            }
        }
    }

    // 0x0046FA88
    void paintEntities(PaintSession& session, const Map::map_pos& loc)
    {
        paintEntitiesWithFilter(session, loc, [](const thing_base*) { return true; });
    }

    static bool isEntityFlyingOrFloating(const thing_base* entity)
    {
        auto* vehicle = entity->asVehicle();
        if (vehicle == nullptr)
        {
            return false;
        }
        switch (vehicle->getTransportMode())
        {
            case TransportMode::air:
            case TransportMode::water:
                return true;
            case TransportMode::rail:
            case TransportMode::road:
                return false;
        }
        return false;
    }

    // 0x0046FB67
    void paintEntities2(PaintSession& session, const Map::map_pos& loc)
    {
        paintEntitiesWithFilter(session, loc, isEntityFlyingOrFloating);
    }
}
