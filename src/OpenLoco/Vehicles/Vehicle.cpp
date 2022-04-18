#include "Vehicle.h"
#include "../Entities/EntityManager.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    static loco_global<uint8_t[128], 0x004F7358> _4F7358; // trackAndDirection without the direction 0x1FC
    static loco_global<uint32_t, 0x00525FBC> _525FBC;     // RoadObjectId bits

#pragma pack(push, 1)
    // There are some common elements in the vehicle components at various offsets these can be accessed via VehicleBase
    struct VehicleCommon : VehicleBase
    {
        uint8_t pad_24[0x24 - 0x22];
        EntityId head;               // 0x26
        uint32_t remainingDistance;  // 0x28
        TrackAndDirection var_2C;    // 0x2C
        uint16_t subPosition;        // 0x2E
        int16_t tileX;               // 0x30
        int16_t tileY;               // 0x32
        uint8_t tileBaseZ;           // 0x34
        uint8_t trackType;           // 0x35 field same in all vehicles
        RoutingHandle routingHandle; // 0x36 field same in all vehicles
        uint8_t var_38;              // 0x38
        uint8_t pad_39;
        EntityId nextCarId; // 0x3A
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
        return EntityManager::get<VehicleBase>(veh->nextCarId);
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
        return veh->trackType;
    }

    Map::Pos3 VehicleBase::getTrackLoc() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return Map::Pos3(veh->tileX, veh->tileY, veh->tileBaseZ * 4);
    }

    TrackAndDirection VehicleBase::getVar2C() const
    {
        const auto* veh = reinterpret_cast<const VehicleCommon*>(this);
        return veh->var_2C;
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
        veh->nextCarId = newNextCar;
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

    // 0x004AA407
    void VehicleBase::explodeComponent()
    {
        assert(getSubType() == VehicleThingType::bogie || getSubType() == VehicleThingType::body_start || getSubType() == VehicleThingType::body_continued);
        registers regs;
        regs.esi = X86Pointer(this);
        call(0x004AA407, regs);
    }

    // 0x0047D959
    // ax : loc.x
    // cx : loc.y
    // dl : loc.z / 4
    // bp : trackAndDirection
    // ebp : bp | (setOccupied << 31)
    // returns dh : trackType
    uint8_t VehicleBase::sub_47D959(const Map::Pos3& loc, const TrackAndDirection::_RoadAndDirection trackAndDirection, const bool setOccupied)
    {
        auto trackType = getTrackType();
        auto tile = Map::TileManager::get(loc);
        for (auto& el : tile)
        {
            auto* elRoad = el.as<Map::RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }

            const auto heightDiff = std::abs(loc.z / 4 - elRoad->baseZ());
            if (heightDiff > 4)
            {
                continue;
            }

            if (elRoad->unkDirection() != trackAndDirection.cardinalDirection())
            {
                continue;
            }

            if (elRoad->roadId() != trackAndDirection.id())
            {
                continue;
            }

            if (elRoad->isFlag5())
            {
                continue;
            }

            const auto newUnk4u = _4F7358[trackAndDirection._data >> 2] >> 4;
            if (setOccupied)
            {
                elRoad->setUnk4u(elRoad->unk4u() | newUnk4u);
            }
            else
            {
                elRoad->setUnk4u(elRoad->unk4u() & (~newUnk4u));
            }

            if (getTrackType() == 0xFF)
            {
                if (*_525FBC & (1 << elRoad->roadObjectId()))
                {
                    elRoad->setUnk7_40(true);
                    trackType = elRoad->roadObjectId();
                }
            }
            else
            {
                trackType = getTrackType();
            }
        }
        return trackType;
    }

    bool VehicleBase::updateComponent()
    {
        switch (getSubType())
        {
            case VehicleThingType::head:
                return !asVehicleHead()->update();
            case VehicleThingType::vehicle_1:
                return !asVehicle1()->update();
            case VehicleThingType::vehicle_2:
                return !asVehicle2()->update();
            case VehicleThingType::bogie:
                return !asVehicleBogie()->update();
            case VehicleThingType::body_start:
            case VehicleThingType::body_continued:
                return !asVehicleBody()->update();
            case VehicleThingType::tail:
                return !asVehicleTail()->update();
            default:
                break;
        }
        return false;
    }

    CarComponent::CarComponent(VehicleBase*& component)
    {
        front = component->asVehicleBogie();
        if (front == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        back = component->asVehicleBogie();
        if (back == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        body = component->asVehicleBody();
        if (body == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
    }

    Vehicle::Vehicle(EntityId _head)
    {
        auto component = EntityManager::get<VehicleBase>(_head);
        if (component == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        head = component->asVehicleHead();
        if (head == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        veh1 = component->asVehicle1();
        if (veh1 == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        veh2 = component->asVehicle2();
        if (veh2 == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        component = component->nextVehicleComponent();
        if (component == nullptr)
        {
            throw std::runtime_error("Bad vehicle structure");
        }
        if (component->getSubType() != VehicleThingType::tail)
        {
            cars = Cars{ Car{ component } };
        }
        while (component->getSubType() != VehicleThingType::tail)
        {
            component = component->nextVehicleComponent();
            if (component == nullptr)
            {
                throw std::runtime_error("Bad vehicle structure");
            }
        }
        tail = component->asVehicleTail();
    }

    // 0x00426790
    uint16_t VehicleBogie::getPlaneType()
    {
        auto* vehObj = ObjectManager::get<VehicleObject>(objectId);
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
