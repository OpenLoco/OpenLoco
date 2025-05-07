#include "VehicleRepaint.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Types.hpp"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static void paintComponent(Vehicles::CarComponent component, const VehicleRepaintArgs& args)
    {
        if (args.hasRepaintFlags(VehicleRepaintFlags::bodyColour) && component.body != nullptr)
        {
            component.body->colourScheme = args.colours[kBodyColour];
            component.body->invalidateSprite();
        }
        if (args.hasRepaintFlags(VehicleRepaintFlags::frontBogieColour) && component.front != nullptr)
        {
            component.front->colourScheme = args.colours[kFrontBogieColour];
            component.front->invalidateSprite();
        }
        if (args.hasRepaintFlags(VehicleRepaintFlags::backBogieColour) && component.back != nullptr)
        {
            component.back->colourScheme = args.colours[kBackBogieColour];
            component.back->invalidateSprite();
        }
    }

    static void paintEntireCar(Vehicles::Car& car, const VehicleRepaintArgs& args)
    {
        for (Vehicles::CarComponent& component : car)
        {
            paintComponent(component, args);
        }
    }

    static uint32_t vehicleRepaint(const VehicleRepaintArgs& args, const uint8_t flags)
    {
        auto entity = EntityManager::get<EntityBase>(args.head);
        auto veh = entity->asBase<Vehicles::VehicleBase>();
        if (veh == nullptr)
        {
            return FAILURE;
        }

        if (!sub_431E6A(veh->owner))
        {
            return FAILURE;
        }

        if (!(flags & Flags::apply))
        {
            return 0;
        }

        Vehicles::Vehicle train(veh->getHead());

        for (auto& car : train.cars)
        {
            if (args.hasRepaintFlags(VehicleRepaintFlags::applyToEntireTrain))
            {
                paintEntireCar(car, args);
                continue;
            }
            for (auto& carComponent : car)
            {
                if (carComponent.front == veh || carComponent.back == veh || carComponent.body == veh)
                {
                    if (args.hasRepaintFlags(VehicleRepaintFlags::applyToEntireCar))
                    {
                        paintEntireCar(car, args);
                    }
                    else
                    {
                        paintComponent(carComponent, args);
                    }
                    return 0;
                }
            }
        }
        return 0;
    }

    void vehicleRepaint(registers& regs)
    {
        regs.ebx = vehicleRepaint(VehicleRepaintArgs(regs), regs.bl);
    }
}
