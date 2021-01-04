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
    const uint8_t _reversePitch[16]{
        0, 5, 6, 7, 8, 1, 2, 3, 4, 10, 9, 12, 11, 0, 0, 0
    };

    // 0x004B0CFC
    static void paintBogie(PaintSession& session, vehicle_bogie* bogie)
    {
        auto* vehObject = ObjectManager::get<vehicle_object>(bogie->object_id);
        if (bogie->object_sprite_type == 0xFF)
        {
            return;
        }

        auto& sprite = vehObject->bogie_sprites[bogie->object_sprite_type];
        //uint8_t edi = bogie_sprites[0].flags[bogie->object_sprite_type]; ?? why ??
        uint8_t yaw = (bogie->sprite_yaw + (session.getRotation() << 4)) & 0x3F;
        uint8_t pitch = bogie->sprite_pitch;

        // Reverse??
        if (bogie->getFlags38() & Flags38::isReversed)
        {
            yaw ^= (1 << 5);
            pitch = _reversePitch[bogie->sprite_pitch];
        }
        auto yawIndex = (yaw >> 1) & 0x1F;
        switch (pitch)
        {
            case 0:
            {

                if (sprite.flags & BogieSpriteFlags::rotationalSymmetry)
                {
                    yawIndex &= 0xF;
                }
                auto imageId = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.flatImageIds;
                if (bogie->getFlags38() & Flags38::isGhost)
                {
                    Config::get().construction_marker;
                    //imageId |= select ghost or invis;
                }
                else if (bogie->var_0C & Flags0C::unk_5)
                {
                    imageId = Gfx::recolour(imageId, PaletteIndex::index_74);
                }
                else
                {
                    imageId = Gfx::recolour2(imageId, bogie->colour_scheme.primary, bogie->colour_scheme.secondary);
                }

                if (bogie->getTransportMode() == TransportMode::air)
                {
                    if (bogie->getFlags38() & Flags38::isGhost)
                    {
                        return;
                    }
                    // special code
                }
                else
                {
                    if (sprite.flags & BogieSpriteFlags::unk_4)
                    {
                        // larger sprite
                    }
                    else
                    {
                        // smaller sprite
                    }
                }
                break;
            }
            case 2:
            case 9:
                // jumps into 6/10 at auto imageid
                break;
            case 6:
            case 10:
            {

                yawIndex ^= (1 << 5);
                if (sprite.flags & BogieSpriteFlags::rotationalSymmetry)
                {
                    yawIndex ^= (1 << 5) | (1 << 4);
                }
                auto imageId = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.gentleImageIds;
                if (bogie->getFlags38() & Flags38::isGhost)
                {
                    Config::get().construction_marker;
                    //imageId |= select ghost or invis;
                }
                else
                {
                    imageId = Gfx::recolour2(imageId, bogie->colour_scheme.primary, bogie->colour_scheme.secondary);
                }
                if (sprite.flags & BogieSpriteFlags::unk_4)
                {
                    // larger sprite
                }
                else
                {
                    // smaller sprite
                }
                break;
            }
            case 4:
            case 11:
                // jumps into default at auto imageid
                break;
            default:
            {

                yawIndex ^= (1 << 5);
                if (sprite.flags & BogieSpriteFlags::rotationalSymmetry)
                {
                    yawIndex ^= (1 << 5) | (1 << 4);
                }
                auto imageId = sprite.numRollSprites * yawIndex + bogie->var_46 + sprite.steepImageIds;
                if (bogie->getFlags38() & Flags38::isGhost)
                {
                    Config::get().construction_marker;
                    //imageId |= select ghost or invis;
                }
                else
                {
                    imageId = Gfx::recolour2(imageId, bogie->colour_scheme.primary, bogie->colour_scheme.secondary);
                }

                break;
            }
        }
        registers regs{};
        regs.ax = bogie->x;
        regs.cx = bogie->y;
        regs.dx = bogie->z;
        regs.esi = reinterpret_cast<int32_t>(bogie);
        regs.ebp = reinterpret_cast<int32_t>(vehObject);
        call(0x004B0CCE, regs); // Cant call 0x004B0CFC due to stack
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
