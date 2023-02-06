#include "Misc.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "EntityManager.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrainStationObject.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // 0x004405CD
    void MiscBase::update()
    {
        switch (getSubType())
        {
            case MiscEntityType::exhaust:
                asExhaust()->update();
                break;
            case MiscEntityType::redGreenCurrency:
                asRedGreenCurrency()->update();
                break;
            case MiscEntityType::windowCurrency:
                asWindowCurrency()->update();
                break;
            case MiscEntityType::vehicleCrashParticle:
                asVehicleCrashParticle()->update();
                break;
            case MiscEntityType::explosionCloud:
                asExplosionCloud()->update();
                break;
            case MiscEntityType::splash:
                asSplash()->update();
                break;
            case MiscEntityType::fireball:
                asFireball()->update();
                break;
            case MiscEntityType::explosionSmoke:
                asExplosionSmoke()->update();
                break;
            case MiscEntityType::smoke:
                asSmoke()->update();
                break;
        }
    }

    const SteamObject* Exhaust::getObject() const
    {
        return ObjectManager::get<SteamObject>(objectId & 0x7F);
    }

    // 0x004408C2
    void Exhaust::update()
    {
        const auto* steamObj = getObject();
        if (steamObj->hasFlags(SteamObjectFlags::applyWind))
        {
            // Wind is applied by applying a slight modification (~1 in 10 ticks a pixel change) to the x component of the exhaust
            const auto res = windProgress + 7000;
            windProgress = static_cast<uint16_t>(res);
            auto newPos = position;
            newPos.x += res / (std::numeric_limits<uint16_t>::max() + 1);
            if (newPos.x != position.x)
            {
                invalidateSprite();
                moveTo(newPos);
                invalidateSprite();
            }
        }
        stationaryProgress++;
        if (stationaryProgress < steamObj->numStationaryTicks)
        {
            return;
        }
        stationaryProgress = 0;
        frameNum++;

        auto [totalNumFrames, frameInfo] = steamObj->getFramesInfo(isSubObjType1());

        if (frameNum >= totalNumFrames)
        {
            invalidateSprite();
            EntityManager::freeEntity(this);
            return;
        }

        auto newPos = position;
        newPos.z += frameInfo[frameNum].height;
        invalidateSprite();
        moveTo(newPos);
        invalidateSprite();

        if (!steamObj->hasFlags(SteamObjectFlags::disperseOnCollision))
        {
            return;
        }

        const auto tile = Map::TileManager::get(position);
        const auto lowZ = (position.z / Map::kSmallZStep) - 3;
        const auto highZ = lowZ + 6;
        for (const auto& el : tile)
        {
            if (el.isFlag5() || el.isGhost())
            {
                continue;
            }
            if (lowZ < el.baseZ() && highZ > el.baseZ())
            {
                invalidateSprite();
                EntityManager::freeEntity(this);
                return;
            }

            auto* elTrack = el.as<Map::TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }
            if (!elTrack->hasStationElement())
            {
                continue;
            }
            auto* elStation = elTrack->next()->as<Map::StationElement>();
            if (elStation == nullptr)
            {
                continue;
            }
            if (elStation->isFlag5() || elStation->isGhost())
            {
                continue;
            }

            const auto* stationObj = ObjectManager::get<TrainStationObject>(elStation->objectId());
            if (stationObj->hasFlags(TrainStationFlags::unk1))
            {
                continue;
            }

            if (elStation->multiTileIndex() == 0)
            {
                continue;
            }

            const auto cmpZ = elTrack->baseZ() + 7;
            if (lowZ >= cmpZ)
            {
                continue;
            }
            if (highZ <= cmpZ)
            {
                continue;
            }

            invalidateSprite();
            EntityManager::freeEntity(this);
            return;
        }
    }

    // 0x0044080C
    Exhaust* Exhaust::create(Map::Pos3 loc, uint8_t type)
    {
        if (!Map::validCoords(loc))
            return nullptr;
        auto surface = Map::TileManager::get(loc.x & 0xFFE0, loc.y & 0xFFE0).surface();

        if (surface == nullptr)
            return nullptr;

        if (loc.z <= surface->baseHeight())
            return nullptr;

        auto _exhaust = static_cast<Exhaust*>(EntityManager::createEntityMisc());

        if (_exhaust != nullptr)
        {
            _exhaust->baseType = EntityBaseType::misc;
            _exhaust->moveTo(loc);
            _exhaust->objectId = type;
            const auto* obj = _exhaust->getObject();
            _exhaust->var_14 = obj->var_05;
            _exhaust->var_09 = obj->var_06;
            _exhaust->var_15 = obj->var_07;
            _exhaust->setSubType(MiscEntityType::exhaust);
            _exhaust->frameNum = 0;
            _exhaust->stationaryProgress = 0;
            _exhaust->windProgress = 0;
            _exhaust->var_34 = 0;
            _exhaust->var_36 = 0;
        }
        return _exhaust;
    }

    // 0x004407A1
    void Smoke::update()
    {
        Ui::ViewportManager::invalidate(this, ZoomLevel::half);
        moveTo(position + Map::Pos3(0, 0, 1));
        Ui::ViewportManager::invalidate(this, ZoomLevel::half);

        frame += 0x55;
        if (frame >= 0xC00)
        {
            EntityManager::freeEntity(this);
        }
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
            t->baseType = EntityBaseType::misc;
            t->moveTo(loc);
            t->setSubType(MiscEntityType::smoke);
            t->frame = 0;
        }
        return t;
    }

    // 0x004FADD0
    constexpr Map::Pos2 _wiggleAmounts[] = {
        { 1, -1 },
        { 1, 1 },
        { -1, 1 },
        { -1, -1 },
    };

    // clang-format off
    // 0x004FAD21
    constexpr int8_t _wiggleZAmounts[MoneyEffect::kLifetime] = {
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
        if (getSubType() == MiscEntityType::windowCurrency)
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
            moveTo(position + Map::Pos3{ nudge.x, nudge.y, nudgeZ });
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
            moveTo(position + Map::Pos3{ nudge.x, nudge.y, position.z });
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
    MoneyEffect* MoneyEffect::create(const Map::Pos3& loc, const CompanyId company, const currency32_t amount)
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
            m->baseType = EntityBaseType::misc;
            m->var_2E = company;
            m->moveTo(loc);
            m->setSubType(MiscEntityType::windowCurrency);
            m->frame = 0;
            m->numMovements = 0;

            string_id strFormat = (amount < 0) ? StringIds::format_currency_expense_red_negative : StringIds::format_currency_income_green;
            char buffer[255] = {};
            auto args = FormatArguments::common(amount);
            StringManager::formatString(buffer, strFormat, &args);
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
            drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
            m->offsetX = -drawingCtx.getStringWidth(buffer) / 2;
            m->wiggle = 0;
        }
        return m;
    }

    // 0x004406A0
    void VehicleCrashParticle::update()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        call(0x004406A0, regs);
    }

    // 0x004407CC
    void ExplosionCloud::update()
    {
        invalidateSprite();
        frame += 0x80;
        if (frame >= 0x1200)
        {
            EntityManager::freeEntity(this);
        }
    }

    // 0x004407E0
    void Splash::update()
    {
        invalidateSprite();
        frame += 0x55;
        if (frame >= 0x1C00)
        {
            EntityManager::freeEntity(this);
        }
    }

    // 0x004407F3
    void Fireball::update()
    {
        invalidateSprite();
        frame += 0x40;
        if (frame >= 0x1F00)
        {
            EntityManager::freeEntity(this);
        }
    }

    // 0x00440078D
    void ExplosionSmoke::update()
    {
        invalidateSprite();
        frame += 0x80;
        if (frame >= 0xA00)
        {
            EntityManager::freeEntity(this);
        }
    }

    // 0x00440BBF
    ExplosionSmoke* ExplosionSmoke::create(const Map::Pos3& loc)
    {
        auto t = static_cast<ExplosionSmoke*>(EntityManager::createEntityMisc());
        if (t != nullptr)
        {
            t->var_14 = 44;
            t->var_09 = 32;
            t->var_15 = 34;
            t->baseType = EntityBaseType::misc;
            t->moveTo(loc + Map::Pos3{ 0, 0, 4 });
            t->setSubType(MiscEntityType::explosionSmoke);
            t->frame = 0;
        }
        return t;
    }
}
