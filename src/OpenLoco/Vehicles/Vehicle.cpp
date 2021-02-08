#include "Vehicle.h"
#include "../Audio/Audio.h"
#include "../Config.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/VehicleObject.h"
#include "../OpenLoco.h"
#include "../Things/ThingManager.h"
#include "../Utility/Numeric.hpp"
#include "../ViewportManager.h"
#include <algorithm>
#include <cassert>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{

    vehicle_base* vehicle_base::nextVehicle()
    {
        return ThingManager::get<vehicle_base>(next_thing_id);
    }

    vehicle_base* vehicle_base::nextVehicleComponent()
    {
        auto* veh = reinterpret_cast<vehicle*>(this);
        return ThingManager::get<vehicle_base>(veh->next_car_id);
    }

    TransportMode vehicle_base::getTransportMode() const
    {
        const auto* veh = reinterpret_cast<const vehicle*>(this);
        return veh->mode;
    }

    uint8_t vehicle_base::getOwner() const
    {
        const auto* veh = reinterpret_cast<const vehicle*>(this);
        return veh->owner;
    }

    uint8_t vehicle_base::getFlags38() const
    {
        const auto* veh = reinterpret_cast<const vehicle*>(this);
        return veh->var_38;
    }

    thing_id_t vehicle_base::getHead() const
    {
        const auto* veh = reinterpret_cast<const vehicle*>(this);
        return veh->head;
    }

    vehicle_object* vehicle::object() const
    {
        return ObjectManager::get<vehicle_object>(object_id);
    }

    vehicle_object* vehicle_body::object() const
    {
        return ObjectManager::get<vehicle_object>(object_id);
    }

    bool vehicle_base::updateComponent()
    {
        int32_t result = 0;
        registers regs;
        regs.esi = (int32_t)this;
        switch (getSubType())
        {
            case VehicleThingType::head:
                result = asVehicleHead()->update();
                break;
            case VehicleThingType::vehicle_1:
                result = call(0x004A9788, regs);
                break;
            case VehicleThingType::vehicle_2:
                result = call(0x004A9B0B, regs);
                break;
            case VehicleThingType::bogie:
                result = call(0x004AA008, regs);
                break;
            case VehicleThingType::body_start:
            case VehicleThingType::body_continued:
                result = asVehicleBody()->update();
                break;
            case VehicleThingType::tail:
                result = call(0x004AA24A, regs);
                break;
            default:
                break;
        }
        return (result & (1 << 8)) != 0;
    }

    CarComponent::CarComponent(vehicle_base*& component)
    {
        front = component->asVehicleBogie();
        back = front->nextVehicleComponent()->asVehicleBogie();
        body = back->nextVehicleComponent()->asVehicleBody();
        component = body->nextVehicleComponent();
    }

    Vehicle::Vehicle(uint16_t _head)
    {
        auto component = ThingManager::get<vehicle_base>(_head);
        if (component == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        head = component->asVehicleHead();
        veh1 = head->nextVehicleComponent()->asVehicle1();
        veh2 = veh1->nextVehicleComponent()->asVehicle2();
        component = veh2->nextVehicleComponent();
        if (component->getSubType() != VehicleThingType::tail)
        {
            cars = Cars{ Car{ component } };
        }
        while (component->getSubType() != VehicleThingType::tail)
        {
            component = component->nextVehicleComponent();
        }
        tail = component->asVehicleTail();
    }
}
