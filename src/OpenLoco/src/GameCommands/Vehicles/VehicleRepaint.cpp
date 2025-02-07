#include "VehicleRepaint.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Types.hpp"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{

    static void PaintComponent(Vehicles::CarComponent component, QuadraColour colours, uint8_t paintFlags)
    {
        if ((paintFlags & VehicleRepaintFlags::bodyColour) && component.body != nullptr)
        {
            component.body->colourScheme = colours[kBodyColour];
            component.body->invalidateSprite();
        }
        if ((paintFlags & VehicleRepaintFlags::frontBogieColour) && component.front != nullptr)
        {
            component.front->colourScheme = colours[kFrontBogieColour];
            component.front->invalidateSprite();
        }
        if ((paintFlags & VehicleRepaintFlags::backBogieColour) && component.back != nullptr)
        {
            component.back->colourScheme = colours[kBackBogieColour];
            component.back->invalidateSprite();
        }
    }

    static void PaintEntireCar(Vehicles::Car& car, QuadraColour colours, uint8_t paintFlags)
    {
        for (Vehicles::CarComponent component : car)
        {
            PaintComponent(component, colours, paintFlags);
        }
    }

    // 0x004B0B50
    static uint32_t vehicleRepaint(EntityId headId, const QuadraColour colours, const uint8_t paintFlags, const uint8_t flags)
    {

        try
        {
            auto entity = EntityManager::get<EntityBase>(headId);
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

            [&train = train, &targetEntity = *veh, &colours = colours, &paintFlags = paintFlags]() {
                for (auto& car : train.cars)
                {
                    if (paintFlags & VehicleRepaintFlags::applyToEntireTrain)
                    {
                        PaintEntireCar(car, colours, paintFlags);
                        continue;
                    }
                    for (auto& carComponent : car)
                    {
                        if (carComponent.front == &targetEntity || carComponent.back == &targetEntity || carComponent.body == &targetEntity)
                        {
                            if (paintFlags & VehicleRepaintFlags::applyToEntireCar)
                            {
                                PaintEntireCar(car, colours, paintFlags);
                            }
                            else
                            {
                                PaintComponent(carComponent, colours, paintFlags);
                            }
                            return;
                        }
                    }
                }
            }();

            return 0;
        }
        catch (std::runtime_error&)
        {
            return FAILURE;
        }
    }

    void vehicleRepaint(registers& regs)
    {
        regs.ebx = vehicleRepaint(EntityId(regs.ebp), { ColourScheme(regs.cx), ColourScheme(regs.ecx >> 16), ColourScheme(regs.dx), ColourScheme(regs.edx >> 16) }, regs.ax, regs.bl);
    }
}
