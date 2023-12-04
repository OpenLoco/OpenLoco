#include "VehicleRearrange.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "Vehicles/Vehicle.h"

namespace OpenLoco::GameCommands
{
    currency32_t vehicleRearrange(const VehicleRearrangeArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Miscellaneous);

        auto* sourceVehicle = EntityManager::get<Vehicles::VehicleBase>(args.source);
        auto* destVehicle = EntityManager::get<Vehicles::VehicleBase>(args.dest);

        if (!(flags & Flags::apply))
        {
            if (!sub_431E6A(sourceVehicle->owner))
            {
                return FAILURE;
            }
            if (!sub_431E6A(destVehicle->owner))
            {
                return FAILURE;
            }

            auto* sourceHead = EntityManager::get<Vehicles::VehicleHead>(sourceVehicle->getHead());
            auto* destHead = EntityManager::get<Vehicles::VehicleHead>(destVehicle->getHead());
            if (!sourceHead->canBeModified())
            {
                return FAILURE;
            }
            if (!destHead->canBeModified())
            {
                return FAILURE;
            }

            if (sourceVehicle->getTrackType() != destVehicle->getTrackType())
            {
                setErrorText(StringIds::incompatible_vehicle);
                return FAILURE;
            }

            // Pretty sure this needs to be true but not 100% (openloco addition)
            auto* sourceBogie = sourceVehicle->asVehicleBogie();
            if (sourceBogie == nullptr)
            {
                return FAILURE;
            }

            if (!destHead->isVehicleTypeCompatible(sourceBogie->objectId))
            {
                return FAILURE;
            }

            setPosition(destVehicle->position);
            return 0;
        }
        else
        {
            // 0x004AF2A1
        }
        return FAILURE;
    }

    // 0x004AF1DF
    void vehicleRearrange(registers& regs)
    {
        regs.ebx = vehicleRearrange(VehicleRearrangeArgs(regs), regs.bl);
    }
}
