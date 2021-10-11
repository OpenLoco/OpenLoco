#include "Vehicle.h"
#include "../Entities/EntityManager.h"
#include "../Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
#pragma pack(push, 1)
    // There are some common elements in the vehicle components at various offsets these can be accessed via VehicleBase
    struct VehicleCommon : VehicleBase
    {
        uint8_t pad_24[0x24 - 0x22];
        EntityId head;               // 0x26
        uint32_t remainingDistance;  // 0x28
        TrackAndDirection var_2C;    // 0x2C
        uint16_t subPosition;        // 0x2E
        int16_t tile_x;              // 0x30
        int16_t tile_y;              // 0x32
        uint8_t tile_base_z;         // 0x34
        uint8_t track_type;          // 0x35 field same in all vehicles
        RoutingHandle routingHandle; // 0x36 field same in all vehicles
        uint8_t var_38;              // 0x38
        uint8_t pad_39;
        EntityId next_car_id; // 0x3A
        uint8_t pad_3C[0x42 - 0x3C];
        TransportMode mode; // 0x42 field same in all vehicles
    };
    static_assert(sizeof(VehicleCommon) == 0x43); // Can't use offset_of change this to last field if more found
#pragma pack(pop)

    VehicleBase* VehicleBase::nextVehicle()
    {
        return EntityManager::get<VehicleBase>(next_thing_id);
    }

    VehicleBase* VehicleBase::nextVehicleComponent()
    {
        auto* veh = reinterpret_cast<VehicleCommon*>(this);
        return EntityManager::get<VehicleBase>(veh->next_car_id);
    }

    TransportMode VehicleBase::getTransportMode() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->mode;
    }

    uint8_t VehicleBase::getFlags38() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->var_38;
    }

    uint8_t VehicleBase::getTrackType() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->track_type;
    }

    RoutingHandle VehicleBase::getRoutingHandle() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->routingHandle;
    }

    EntityId VehicleBase::getHead() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->head;
    }

    void VehicleBase::setNextCar(const EntityId newNextCar)
    {
        auto* veh = reinterpret_cast<VehicleCommon*>(this);
        veh->next_car_id = newNextCar;
    }

    // 0x004AA464
    void VehicleBase::sub_4AA464()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        call(0x004AA464, regs);
    }

    // 0x004B15FF
    uint32_t VehicleBase::sub_4B15FF(uint32_t unk1)
    {
        registers regs;
        regs.eax = unk1;
        regs.esi = X86Pointer(this);
        call(0x004B15FF, regs);
        return regs.eax;
    }

    // 0x0047D959
    void VehicleBase::sub_47D959(const Map::Pos3& loc, const TrackAndDirection::_RoadAndDirection trackAndDirection, const bool setOccupied)
    {
        registers regs;
        regs.ax = loc.x;
        regs.cx = loc.y;
        regs.dl = loc.z / 4;
        regs.ebp = trackAndDirection._data | (setOccupied ? 1 << 31 : 0);
        regs.esi = X86Pointer(this);
        call(0x0047D959, regs);
    }

    bool VehicleBase::updateComponent()
    {
        int32_t result = 0;
        registers regs;
        regs.esi = X86Pointer(this);
        switch (getSubType())
        {
            case VehicleThingType::head:
                return !asVehicleHead()->update();
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
                return !asVehicleBody()->update();
                break;
            case VehicleThingType::tail:
                return !asVehicleTail()->update();
                break;
            default:
                break;
        }
        return (result & (1 << 8)) != 0;
    }

    CarComponent::CarComponent(VehicleBase*& component)
    {
        front = component->asVehicleBogie();
        back = front->nextVehicleComponent()->asVehicleBogie();
        body = back->nextVehicleComponent()->asVehicleBody();
        component = body->nextVehicleComponent();
    }

    Vehicle::Vehicle(EntityId _head)
    {
        auto component = EntityManager::get<VehicleBase>(_head);
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

    // 0x00426790
    uint16_t VehicleBogie::getPlaneType()
    {
        auto* vehObj = ObjectManager::get<VehicleObject>(object_id);
        if (vehObj->flags & FlagsE0::isHelicopter)
        {
            return 1 << 4;
        }
        if (vehObj->weight < 50)
        {
            return (1 << 3) | (1 << 2);
        }
        return 1 << 3;
    }
}
