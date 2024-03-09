#include "MoneyEffect.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Entities/EntityManager.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"

namespace OpenLoco
{
    // 0x004FADD0
    static constexpr World::Pos2 _wiggleAmounts[] = {
        { 1, -1 },
        { 1, 1 },
        { -1, 1 },
        { -1, -1 },
    };

    // clang-format off
    // 0x004FAD21
    static constexpr int8_t _wiggleZAmounts[MoneyEffect::kLifetime] = {
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
    };
    // clang-format on

    // 0x0044063E
    void MoneyEffect::update()
    {
        if (getSubType() == EffectType::windowCurrency)
        {
            invalidateSprite();
            if (wiggle == 0)
            {
                wiggle = 21;
            }
            else
            {
                wiggle--;
            }

            if (frame >= kLifetime)
            {
                EntityManager::freeEntity(this);
                return;
            }
            const auto nudge = _wiggleAmounts[Ui::WindowManager::getCurrentRotation()] * ((frame & 1) ? 0 : 1);
            const auto nudgeZ = _wiggleZAmounts[frame];
            moveTo(position + World::Pos3{ nudge.x, nudge.y, nudgeZ });
            frame++;
        }
        else
        {
            invalidateSprite();
            if (wiggle == 22)
            {
                wiggle = 0;
            }
            else
            {
                wiggle++;
            }
            moveDelay++;
            if (moveDelay < 2)
            {
                return;
            }
            moveDelay = 0;

            const auto nudge = _wiggleAmounts[Ui::WindowManager::getCurrentRotation()];
            moveTo(position + World::Pos3{ nudge.x, nudge.y, position.z });
            numMovements++;
            if (numMovements >= kRedGreenLifetime)
            {
                EntityManager::freeEntity(this);
            }
        }
    }

    // 0x00440A74
    // company : updatingCompanyId global
    // loc : ax, cx, dx
    // amount : ebx
    MoneyEffect* MoneyEffect::create(const World::Pos3& loc, const CompanyId company, const currency32_t amount)
    {
        if (isTitleMode())
        {
            return nullptr;
        }

        auto* m = static_cast<MoneyEffect*>(EntityManager::createEntityMoney());
        if (m != nullptr)
        {
            m->amount = amount;
            m->spriteWidth = 64;
            m->spriteHeightNegative = 20;
            m->spriteHeightPositive = 30;
            m->baseType = EntityBaseType::effect;
            m->var_2E = company;
            m->moveTo(loc);
            m->setSubType(EffectType::windowCurrency);
            m->frame = 0;
            m->numMovements = 0;

            StringId strFormat = (amount < 0) ? StringIds::format_currency_expense_red_negative : StringIds::format_currency_income_green;
            char buffer[255] = {};
            FormatArguments args{};
            args.push(amount);
            StringManager::formatString(buffer, strFormat, &args);

            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
            drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
            m->offsetX = -drawingCtx.getStringWidth(buffer) / 2;
            m->wiggle = 0;
        }
        return m;
    }

}
