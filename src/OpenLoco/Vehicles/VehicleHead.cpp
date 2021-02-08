#include "../Audio/Audio.h"
#include "../Config.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/VehicleObject.h"
#include "../OpenLoco.h"
#include "../Things/Misc.h"
#include "../Things/ThingManager.h"
#include "../Utility/Numeric.hpp"
#include "../ViewportManager.h"
#include "Vehicle.h"
#include <algorithm>
#include <cassert>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;

    void VehicleHead::updateVehicle()
    {
        // TODO: Refactor to use the Vehicle super class
        vehicle_base* v = this;
        while (v != nullptr)
        {
            if (v->updateComponent())
            {
                break;
            }
            v = v->nextVehicleComponent();
        }
    }

    uint16_t VehicleHead::update()
    {
        registers regs;
        regs.esi = (int32_t)this;
        return call(0x004A8B81, regs);
    }

    // 0x004BA8D4
    void VehicleHead::sub_4BA8D4()
    {
        switch (status)
        {
            case Status::unk_0:
            case Status::stopped:
            case Status::unk_3:
            case Status::unloading:
            case Status::loading:
            case Status::crashed:
            case Status::stuck:
                return;
            case Status::unk_2:
            case Status::approaching:
            case Status::brokenDown:
            case Status::landing:
            case Status::taxiing1:
            case Status::taxiing2:
            case Status::takingOff:
                break;
        }
        Vehicle train(this);
        for (auto car : train.cars)
        {
            if (car.front->var_5F & Flags5F::broken_down)
            {
                if ((scenarioTicks() & 3) == 0)
                {
                    auto v2 = car.body; // body
                    smoke::create(loc16(v2->x, v2->y, v2->z + 4));
                }
            }

            if ((car.front->var_5F & Flags5F::breakdown_pending) && !isTitleMode())
            {
                auto newConfig = Config::getNew();
                if (!newConfig.breakdowns_disabled)
                {
                    car.front->var_5F &= ~Flags5F::breakdown_pending;
                    car.front->var_5F |= Flags5F::broken_down;
                    car.front->var_6A = 5;
                    sub_4BAA76();

                    auto v2 = car.body;
                    auto soundId = (Audio::sound_id)gPrng().randNext(26, 26 + 5);
                    Audio::playSound(soundId, loc16(v2->x, v2->y, v2->z + 22));
                }
            }
        }
    }

    void VehicleHead::sub_4BAA76()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call(0x004BAA76, regs);
    }

    // 0x004B90F0
    // eax : newVehicleTypeId
    // ebx : sourceVehicleTypeId;
    static bool sub_4B90F0(const uint16_t newVehicleTypeId, const uint16_t sourceVehicleTypeId)
    {
        auto newObject = ObjectManager::get<vehicle_object>(newVehicleTypeId);       //edi
        auto sourceObject = ObjectManager::get<vehicle_object>(sourceVehicleTypeId); // esi

        if ((newObject->flags & FlagsE0::can_couple) && (sourceObject->flags & FlagsE0::can_couple))
        {
            gGameCommandErrorText = StringIds::incompatible_vehicle;
            return false;
        }

        if (newVehicleTypeId == sourceVehicleTypeId)
        {
            return true;
        }

        for (auto i = 0; i < newObject->num_compat; ++i)
        {
            if (newObject->compatible_vehicles[i] == sourceVehicleTypeId)
            {
                return true;
            }
        }

        if (sourceObject->num_compat != 0)
        {
            for (auto i = 0; i < sourceObject->num_compat; ++i)
            {
                if (sourceObject->compatible_vehicles[i] == newVehicleTypeId)
                {
                    return true;
                }
            }
        }

        if ((newObject->num_compat != 0) || (sourceObject->num_compat != 0))
        {
            gGameCommandErrorText = StringIds::incompatible_vehicle;
            return false;
        }

        return true;
    }

    // 0x004B9780
    // used by road vehicles only maybe??
    static uint32_t getVehicleTypeLength(const uint16_t vehicleTypeId)
    {
        auto vehObject = ObjectManager::get<vehicle_object>(vehicleTypeId);
        auto length = 0;
        for (auto i = 0; i < vehObject->var_04; ++i)
        {
            if (vehObject->var_24[i].body_sprite_ind == 0xFF)
            {
                continue;
            }

            auto unk = vehObject->var_24[i].body_sprite_ind & 0x7F;
            length += vehObject->bodySprites[unk].bogey_position * 2;
        }
        return length;
    }

    // 0x004B97B7
    // used by road vehicles only maybe??
    uint32_t VehicleHead::getVehicleTotalLength() // TODO: const
    {
        auto totalLength = 0;
        Vehicle train(this);
        for (const auto& car : train.cars)
        {
            totalLength += getVehicleTypeLength(car.body->object_id);
        }
        return totalLength;
    }

    // 0x004B8FA2
    // esi : self
    // ax  : vehicleTypeId
    bool VehicleHead::isVehicleTypeCompatible(const uint16_t vehicleTypeId) // TODO: const
    {
        auto newObject = ObjectManager::get<vehicle_object>(vehicleTypeId);
        if (newObject->mode == TransportMode::air || newObject->mode == TransportMode::water)
        {
            Vehicle train(this);
            if (!train.cars.empty())
            {
                gGameCommandErrorText = StringIds::incompatible_vehicle;
                return false;
            }
        }
        else
        {
            if (newObject->track_type != track_type)
            {
                gGameCommandErrorText = StringIds::incompatible_vehicle;
                return false;
            }
        }

        if (newObject->mode != mode)
        {
            gGameCommandErrorText = StringIds::incompatible_vehicle;
            return false;
        }

        if (newObject->type != vehicleType)
        {
            gGameCommandErrorText = StringIds::incompatible_vehicle;
            return false;
        }

        {
            Vehicle train(this);
            for (const auto& car : train.cars)
            {
                // The object_id is the same for all vehicle components and car components of a car
                if (!sub_4B90F0(vehicleTypeId, car.front->object_id))
                {
                    return false;
                }
            }
        }
        if (mode != TransportMode::road)
        {
            return true;
        }

        if (track_type != 0xFF)
        {
            return true;
        }

        auto curTotalLength = getVehicleTotalLength();
        auto additionalNewLength = getVehicleTypeLength(vehicleTypeId);
        if (curTotalLength + additionalNewLength > max_vehicle_length)
        {
            gGameCommandErrorText = StringIds::vehicle_too_long;
            return false;
        }
        return true;
    }

    // 0x004B671C
    VehicleStatus VehicleHead::getStatus() const
    {
        registers regs = {};
        regs.esi = reinterpret_cast<int32_t>(this);

        call(0x004B671C, regs);

        VehicleStatus vehStatus = {};
        vehStatus.status1 = regs.bx;
        vehStatus.status1Args = regs.eax;
        vehStatus.status2 = regs.cx;
        vehStatus.status2Args = regs.edx;
        return vehStatus;
    }
}