#include "PaintVehicle.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../Graphics/Colour.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/VehicleObject.h"
#include "../Things/Vehicle.h"
#include "Paint.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Things::Vehicle;

namespace OpenLoco::Paint
{
    // 0x00500160
    const Pitch _reversePitch[13]{
        Pitch::flat,
        Pitch::down6deg,
        Pitch::down12deg,
        Pitch::down18deg,
        Pitch::down25deg,
        Pitch::up6deg,
        Pitch::up12deg,
        Pitch::up18deg,
        Pitch::up25deg,
        Pitch::down10deg,
        Pitch::up10deg,
        Pitch::down20deg,
        Pitch::up20deg,
    };

    // 0x004FFAE8
    static uint32_t applyGhostToImage(uint32_t imageId)
    {
        if (Config::get().construction_marker)
        {
            return Gfx::recolour(imageId, PaletteIndex::index_2C);
        }
        else
        {
            return Gfx::recolourTranslucent(imageId, PaletteIndex::index_31);
        }
    }

    // 0x004B0CFC
    static void paintBogie(PaintSession& session, vehicle_bogie* bogie)
    {
        auto* vehObject = ObjectManager::get<vehicle_object>(bogie->object_id);
        if (bogie->object_sprite_type == 0xFF)
        {
            return;
        }

        auto& sprite = vehObject->bogie_sprites[bogie->object_sprite_type];
        uint8_t yaw = (bogie->sprite_yaw + (session.getRotation() << 4)) & 0x3F;
        auto pitch = bogie->sprite_pitch;

        if (bogie->getFlags38() & Flags38::isReversed)
        {
            // Flip the highest bit to reverse the yaw
            yaw ^= (1 << 5);
            pitch = _reversePitch[static_cast<uint8_t>(bogie->sprite_pitch)];
        }
        auto yawIndex = (yaw >> 1) & 0x1F;

        switch (pitch)
        {
            case Pitch::flat:
            case Pitch::up12deg:
            case Pitch::up10deg:
            case Pitch::up25deg:
            case Pitch::up20deg:
                break;
            default:
                if (sprite.flags & BogieSpriteFlags::rotationalSymmetry)
                {
                    // Rotational symmetry will have 1 fewer bits of yaw
                    yawIndex ^= (1 << 4);
                }
                else
                {
                    yawIndex ^= (1 << 5);
                }
                break;
        }
        switch (pitch)
        {
            case Pitch::flat:
            {
                if (sprite.flags & BogieSpriteFlags::rotationalSymmetry)
                {
                    yawIndex &= 0xF;
                }
                auto imageId = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.flatImageIds;
                if (bogie->getFlags38() & Flags38::isGhost)
                {
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::t_0);
                    imageId = applyGhostToImage(imageId);
                }
                else if (bogie->var_0C & Flags0C::unk_5)
                {
                    imageId = Gfx::recolour(imageId, PaletteIndex::index_74);
                }
                else if (bogie->getTransportMode() == TransportMode::air)
                {
                    // Airplane bogies are the shadows of the plane

                    if (bogie->getFlags38() & Flags38::isGhost)
                    {
                        // Ghosts don't cast shadows
                        return;
                    }
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::t_0);
                    imageId = Gfx::recolourTranslucent(imageId, PaletteIndex::index_32);
                    session.addToPlotList4FD200(imageId, { 0, 0, bogie->z }, { 8, 8, static_cast<coord_t>(bogie->z + 6) }, { 48, 48, 2 });
                    return;
                }
                else
                {
                    imageId = Gfx::recolour2(imageId, bogie->colour_scheme.primary, bogie->colour_scheme.secondary);
                }

                if (sprite.flags & BogieSpriteFlags::unk_4)
                {
                    // larger sprite
                    session.addToPlotListAsParent(imageId, { 0, 0, bogie->z }, { -9, -9, static_cast<coord_t>(bogie->z + 3) }, { 18, 18, 5 });
                }
                else
                {
                    // smaller sprite
                    session.addToPlotListAsParent(imageId, { 0, 0, bogie->z }, { -6, -6, static_cast<coord_t>(bogie->z + 3) }, { 12, 12, 1 });
                }
                break;
            }
            case Pitch::up12deg:
            case Pitch::up10deg:
            case Pitch::down12deg:
            case Pitch::down10deg:
            {
                auto imageId = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.gentleImageIds;
                if (bogie->getFlags38() & Flags38::isGhost)
                {
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::t_0);
                    imageId = applyGhostToImage(imageId);
                }
                else
                {
                    imageId = Gfx::recolour2(imageId, bogie->colour_scheme.primary, bogie->colour_scheme.secondary);
                }
                if (sprite.flags & BogieSpriteFlags::unk_4)
                {
                    // larger sprite
                    session.addToPlotListAsParent(imageId, { 0, 0, bogie->z }, { -8, -8, static_cast<coord_t>(bogie->z + 3) }, { 16, 16, 1 });
                }
                else
                {
                    // smaller sprite
                    session.addToPlotListAsParent(imageId, { 0, 0, bogie->z }, { -6, -6, static_cast<coord_t>(bogie->z + 3) }, { 12, 12, 1 });
                }
                break;
            }
            default:
            {
                auto imageId = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.steepImageIds;
                if (bogie->getFlags38() & Flags38::isGhost)
                {
                    session.setItemType(Ui::ViewportInteraction::InteractionItem::t_0);
                    imageId = applyGhostToImage(imageId);
                }
                else
                {
                    imageId = Gfx::recolour2(imageId, bogie->colour_scheme.primary, bogie->colour_scheme.secondary);
                }
                session.addToPlotListAsParent(imageId, { 0, 0, bogie->z }, { -6, -6, static_cast<coord_t>(bogie->z + 3) }, { 12, 12, 1 });
                break;
            }
        }
    }

    // 0x004B103C
    static void paintBody(PaintSession& session, vehicle_body* body)
    {
        auto* vehObject = ObjectManager::get<vehicle_object>(body->object_id);
        registers regs{};
        regs.ax = body->x;
        regs.cx = body->y;
        regs.dx = body->z;
        regs.ebx = (body->sprite_yaw + (session.getRotation() << 4)) & 0x3F;
        regs.esi = reinterpret_cast<int32_t>(body);
        regs.ebp = reinterpret_cast<int32_t>(vehObject);
        call(0x004B0CCE, regs); // Cant call 0x004B103C due to stack
    }

    // 0x004B0CCE
    void paintVehicleEntity(PaintSession& session, vehicle_base* base)
    {
        if (base->getFlags38() & Flags38::isGhost)
        {
            if (base->getOwner() != CompanyManager::getControllingId())
            {
                return;
            }
        }
        switch (base->getSubType())
        {
            case VehicleThingType::head:
            case VehicleThingType::vehicle_1:
            case VehicleThingType::vehicle_2:
            case VehicleThingType::tail:
                break;
            case VehicleThingType::bogie:
                paintBogie(session, base->asVehicleBogie());
                break;
            case VehicleThingType::body_start:
            case VehicleThingType::body_continued:
                paintBody(session, base->asVehicleBody());
                break;
        }
    }
}
