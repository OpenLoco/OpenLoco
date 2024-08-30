#include "VehicleRearrange.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "VehiclePickupAir.h"
#include "VehiclePickupWater.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"

namespace OpenLoco::GameCommands
{
    struct PlacementBackup
    {
        int16_t tileX;
        int16_t tileY;
        World::SmallZ tileBaseZ;
        Vehicles::TrackAndDirection trackAndDirection;
        uint16_t subPosition;
        EntityId head;
    };

    // 0x004AF1DF
    static currency32_t vehicleRearrange(const VehicleRearrangeArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::TrainRunningCosts);

        auto* sourceVehicle = EntityManager::get<Vehicles::VehicleBase>(args.source);
        auto* destVehicle = EntityManager::get<Vehicles::VehicleBase>(args.dest);

        auto* sourceHead = EntityManager::get<Vehicles::VehicleHead>(sourceVehicle->getHead());
        auto* destHead = EntityManager::get<Vehicles::VehicleHead>(destVehicle->getHead());

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
            auto* sourceBogie = sourceVehicle->asVehicleBogie();
            if (sourceBogie == nullptr)
            {
                return FAILURE;
            }

            Vehicles::Vehicle sourceTrain(*sourceHead);
            Vehicles::Vehicle destTrain(*destHead);

            if (sourceHead != destHead)
            {
                [&train = sourceTrain, &targetBogie = *sourceBogie]() {
                    for (auto& car : train.cars)
                    {
                        for (auto& carComponet : car)
                        {
                            if (carComponet.front == &targetBogie)
                            {
                                Vehicles::removeAllCargo(carComponet);
                                return;
                            }
                        }
                    }
                }();
            }

            auto tryPickupTrain = [](Vehicles::Vehicle& train) -> std::optional<PlacementBackup> {
                if (train.head->tileX == -1)
                {
                    return std::nullopt;
                }
                const auto res = PlacementBackup{ train.head->tileX, train.head->tileY, train.head->tileBaseZ, train.head->trackAndDirection, train.head->subPosition, train.head->head };
                switch (train.head->mode)
                {
                    case TransportMode::rail:
                    case TransportMode::road:
                        train.head->liftUpVehicle();
                        break;
                    case TransportMode::air:
                    {
                        // Calling this GC directly as we need the result immediately
                        // perhaps in the future this could be changed.
                        GameCommands::VehiclePickupAirArgs airArgs{};
                        airArgs.head = train.head->id;
                        registers regs = static_cast<registers>(airArgs);
                        regs.bl = GameCommands::Flags::apply;
                        GameCommands::vehiclePickupAir(regs);
                        break;
                    }
                    case TransportMode::water:
                    {
                        // Calling this GC directly as we need the result immediately
                        // perhaps in the future this could be changed.
                        GameCommands::VehiclePickupWaterArgs waterArgs{};
                        waterArgs.head = train.head->id;
                        registers regs = static_cast<registers>(waterArgs);
                        regs.bl = GameCommands::Flags::apply;
                        GameCommands::vehiclePickupWater(regs);
                    }
                }
                setExpenditureType(ExpenditureType::TrainRunningCosts);
                return res;
            };

            std::optional<PlacementBackup> sourcePlacement = tryPickupTrain(sourceTrain);
            std::optional<PlacementBackup> destPlacement = tryPickupTrain(destTrain);

            Vehicles::sub_4AF4D6(*sourceBogie, *destVehicle);

            // Vehicle has been invalidate so get it again
            sourceTrain = Vehicles::Vehicle(*sourceHead);

            destHead->sub_4AF7A4();
            destHead->sub_4B7CC3();
            if (sourceHead != destHead)
            {
                sourceHead->sub_4AF7A4();
                sourceHead->sub_4B7CC3();
            }

            if (sourcePlacement.has_value() && !sourceTrain.cars.empty())
            {
                VehicleManager::placeDownVehicle(sourceHead, sourcePlacement->tileX, sourcePlacement->tileY, sourcePlacement->tileBaseZ, sourcePlacement->trackAndDirection, sourcePlacement->subPosition);
            }
            if (destPlacement.has_value())
            {
                VehicleManager::placeDownVehicle(destHead, destPlacement->tileX, destPlacement->tileY, destPlacement->tileBaseZ, destPlacement->trackAndDirection, destPlacement->subPosition);
            }
            return 0;
        }
    }

    void vehicleRearrange(registers& regs)
    {
        regs.ebx = vehicleRearrange(VehicleRearrangeArgs(regs), regs.bl);
    }
}
