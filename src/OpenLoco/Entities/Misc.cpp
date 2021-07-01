#include "Misc.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include "EntityManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // 0x004405CD
    void MiscBase::update()
    {
        registers regs;
        regs.esi = reinterpret_cast<uint32_t>(this);
        call(0x004405CD, regs);
    }

    SteamObject* Exhaust::object() const
    {
        return ObjectManager::get<SteamObject>(object_id & 0x7F);
    }

    // 0x0044080C
    Exhaust* Exhaust::create(Map::Pos3 loc, uint8_t type)
    {
        if ((uint16_t)loc.x > 12287 || (uint16_t)loc.y > 12287)
            return nullptr;
        auto surface = Map::TileManager::get(loc.x & 0xFFE0, loc.y & 0xFFE0).surface();

        if (surface == nullptr)
            return nullptr;

        if (loc.z <= surface->baseZ() * 4)
            return nullptr;

        auto _exhaust = static_cast<Exhaust*>(EntityManager::createEntityMisc());

        if (_exhaust != nullptr)
        {
            _exhaust->base_type = EntityBaseType::misc;
            _exhaust->moveTo(loc);
            _exhaust->object_id = type;
            auto obj = _exhaust->object();
            _exhaust->var_14 = obj->var_05;
            _exhaust->var_09 = obj->var_06;
            _exhaust->var_15 = obj->var_07;
            _exhaust->setSubType(MiscEntityType::exhaust);
            _exhaust->var_26 = 0;
            _exhaust->var_28 = 0;
            _exhaust->var_32 = 0;
            _exhaust->var_34 = 0;
            _exhaust->var_36 = 0;
        }
        return _exhaust;
    }

    // 0x00440BEB
    Smoke* Smoke::create(Map::Pos3 loc)
    {
        auto t = static_cast<Smoke*>(EntityManager::createEntityMisc());
        if (t != nullptr)
        {
            t->var_14 = 44;
            t->var_09 = 32;
            t->var_15 = 34;
            t->base_type = EntityBaseType::misc;
            t->moveTo(loc);
            t->setSubType(MiscEntityType::smoke);
            t->frame = 0;
        }
        return t;
    }

    static loco_global<int32_t, 0x112C876> _currentFontSpriteBase;

    // 0x00440A74
    // company : updatingCompanyId global
    // loc : ax, cx, dx
    // amount : ebx
    MoneyEffect* MoneyEffect::create(const Map::Pos3& loc, const CompanyId_t company, const currency32_t amount)
    {
        if (isTitleMode())
        {
            return nullptr;
        }

        auto* m = static_cast<MoneyEffect*>(EntityManager::createEntityMoney());
        if (m != nullptr)
        {
            m->amount = amount;
            m->var_14 = 64;
            m->var_09 = 20;
            m->var_15 = 30;
            m->base_type = EntityBaseType::misc;
            m->var_2E = company;
            m->moveTo(loc);
            m->setSubType(MiscEntityType::windowCurrency);
            m->var_26 = 0;
            m->var_28 = 0;

            string_id strFormat = (amount < 0) ? StringIds::format_currency_expense_red_negative : StringIds::format_currency_income_green;
            char buffer[255] = {};
            auto args = FormatArguments::common(amount);
            StringManager::formatString(buffer, strFormat, &args);
            _currentFontSpriteBase = Font::medium_bold;
            m->offsetX = -Gfx::getStringWidth(buffer) / 2;
            m->wiggle = 0;
        }
        return m;
    }
}
