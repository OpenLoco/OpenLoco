#include "../Entities/EntityManager.h"
#include "../Interop/Interop.hpp"
#include "Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    static loco_global<VehicleBogie*, 0x01136124> vehicleUpdate_frontBogie;
    static loco_global<VehicleBogie*, 0x01136128> vehicleUpdate_backBogie;
    static loco_global<bool, 0x01136237> vehicleUpdate_frontBogieHasMoved; // remainingDistance related?
    static loco_global<bool, 0x01136238> vehicleUpdate_backBogieHasMoved;  // remainingDistance related?
    static loco_global<int32_t, 0x0113612C> vehicleUpdate_var_113612C;     // Speed
    static loco_global<uint32_t, 0x01136114> vehicleUpdate_var_1136114;
    static loco_global<int32_t, 0x01136130> vehicleUpdate_var_1136130; // Speed
    static loco_global<EntityId, 0x0113610E> vehicleUpdate_collisionCarComponent;

    // 0x004AA008
    bool VehicleBogie::update()
    {
        vehicleUpdate_backBogie = vehicleUpdate_frontBogie;
        vehicleUpdate_frontBogie = this;

        if (mode == TransportMode::air || mode == TransportMode::water)
        {
            return true;
        }

        const auto oldPos = position;
        vehicleUpdate_var_1136114 = 0;
        sub_4B15FF(vehicleUpdate_var_113612C);

        const auto hasMoved = oldPos != position;
        vehicleUpdate_backBogieHasMoved = vehicleUpdate_frontBogieHasMoved;
        vehicleUpdate_frontBogieHasMoved = hasMoved;

        const auto stash1136130 = vehicleUpdate_var_1136130;
        if (var_5E != 0)
        {
            auto unk = var_5E;
            if (unk > 32)
            {
                unk = 64 - unk;
            }
            vehicleUpdate_var_1136130 = 500 + unk * 320;
        }

        updateRoll();
        vehicleUpdate_var_1136130 = stash1136130;
        if (vehicleUpdate_var_1136114 & (1 << 1))
        {
            sub_4AA464();
            return false;
        }
        else if (!(vehicleUpdate_var_1136114 & (1 << 2)))
        {
            return true;
        }

        sub_4AA464();
        explodeComponent();
        var_0C |= Flags0C::unk_5;
        var_5A &= ~(1u << 31);
        var_5A >>= 3;
        var_5A |= (1u << 31);

        Vehicle train(head);
        bool end = false;
        for (auto& car : train.cars)
        {
            for (auto& carComponent : car)
            {
                if (carComponent.front == this || carComponent.back == this)
                {
                    carComponent.body->explodeComponent();
                    carComponent.body->var_5A &= ~(1u << 31);
                    carComponent.body->var_5A >>= 3;
                    carComponent.body->var_5A |= (1u << 31);
                    end = true;
                    break;
                }
            }
            if (end)
            {
                break;
            }
        }

        auto* collideEntity = EntityManager::get<EntityBase>(vehicleUpdate_collisionCarComponent);
        auto* collideCarComponent = collideEntity->asVehicle();
        if (collideCarComponent != nullptr)
        {
            Vehicle collideTrain(collideCarComponent->getHead());
            if (collideTrain.head->status != Status::crashed)
            {
                collideCarComponent->sub_4AA464();
            }

            end = false;
            for (auto& car : train.cars)
            {
                for (auto& carComponent : car)
                {
                    if (carComponent.front == collideCarComponent)
                    {
                        carComponent.front->explodeComponent();
                        carComponent.front->var_5A &= ~(1u << 31);
                        carComponent.front->var_5A >>= 3;
                        carComponent.front->var_5A |= (1u << 31);
                    }
                    if (carComponent.back == collideCarComponent)
                    {
                        carComponent.back->explodeComponent();
                        carComponent.back->var_5A &= ~(1u << 31);
                        carComponent.back->var_5A >>= 3;
                        carComponent.back->var_5A |= (1u << 31);
                    }
                    if (carComponent.front == collideCarComponent || carComponent.back == collideCarComponent || carComponent.body == collideCarComponent)
                    {
                        carComponent.body->explodeComponent();
                        carComponent.body->var_5A &= ~(1u << 31);
                        carComponent.body->var_5A >>= 3;
                        carComponent.body->var_5A |= (1u << 31);
                        end = true;
                        break;
                    }
                }
                if (end)
                {
                    break;
                }
            }
        }
        return false;
    }

    // 0x004AAC02
    void VehicleBogie::updateRoll()
    {
        registers regs;
        regs.esi = X86Pointer(this);

        call(0x004AAC02, regs);
    }
}
