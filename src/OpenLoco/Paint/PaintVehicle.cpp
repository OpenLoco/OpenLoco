#include "PaintVehicle.h"
#include "../CompanyManager.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/VehicleObject.h"
#include "../Things/Vehicle.h"
#include "Paint.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Things::Vehicle;

namespace OpenLoco::Paint
{
    // 0x004B0CFC
    static void paintBogie(PaintSession& session, vehicle_bogie* bogie)
    {
        auto* vehObject = ObjectManager::get<vehicle_object>(bogie->object_id);
        registers regs{};
        regs.ax = bogie->x;
        regs.cx = bogie->y;
        regs.dx = bogie->z;
        regs.ebx = (bogie->sprite_yaw + (session.getRotation() << 4)) & 0x3F;
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
