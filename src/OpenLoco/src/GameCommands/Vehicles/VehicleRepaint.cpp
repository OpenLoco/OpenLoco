#include "VehicleRepaint.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Types.hpp"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static void paintComponent(Vehicles::CarComponent component, QuadraColour colours, VehicleRepaintFlags paintFlags)
    {
        if (paintFlags && VehicleRepaintFlags::bodyColour && component.body != nullptr)
        {
            component.body->colourScheme = colours[kBodyColour];
            component.body->invalidateSprite();
        }
        if (paintFlags && VehicleRepaintFlags::frontBogieColour && component.front != nullptr)
        {
            component.front->colourScheme = colours[kFrontBogieColour];
            component.front->invalidateSprite();
        }
        if (paintFlags && VehicleRepaintFlags::backBogieColour && component.back != nullptr)
        {
            component.back->colourScheme = colours[kBackBogieColour];
            component.back->invalidateSprite();
        }
    }

    static void paintEntireCar(Vehicles::Car& car, QuadraColour colours, VehicleRepaintFlags paintFlags)
    {
        for (Vehicles::CarComponent& component : car)
        {
            paintComponent(component, colours, paintFlags);
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
            if ((args.paintFlags & VehicleRepaintFlags::applyToEntireTrain) != VehicleRepaintFlags::none)
            {
                paintEntireCar(car, args.colours, args.paintFlags);
                continue;
            }
            for (auto& carComponent : car)
            {
                if (carComponent.front == veh || carComponent.back == veh || carComponent.body == veh)
                {
                    if ((args.paintFlags & VehicleRepaintFlags::applyToEntireCar) != VehicleRepaintFlags::none)
                    {
                        paintEntireCar(car, args.colours, args.paintFlags);
                    }
                    else
                    {
                        paintComponent(carComponent, args.colours, args.paintFlags);
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
