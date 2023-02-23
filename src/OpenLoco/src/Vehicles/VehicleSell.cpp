#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Vehicle.h"
#include "VehicleManager.h"

namespace OpenLoco::Vehicles
{
    static currency32_t sell(EntityId id, uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::VehicleDisposals);

        currency32_t refundCost = 0;
        auto* vehBase = EntityManager::get<VehicleBase>(id);
        if (vehBase == nullptr)
        {
            return GameCommands::FAILURE;
        }

        auto* head = EntityManager::get<VehicleHead>(vehBase->getHead());
        if (head == nullptr)
        {
            return GameCommands::FAILURE;
        }
        Vehicle train(*head);
        if (head == vehBase)
        {
            for (const auto& car : train.cars)
            {
                refundCost += car.front->refundCost;
            }
        }
        else
        {
            auto* bogie = vehBase->asVehicleBogie();
            if (bogie == nullptr)
            {
                return GameCommands::FAILURE;
            }
            refundCost = bogie->refundCost;
        }

        if (flags & GameCommands::Flags::apply)
        {
            if (head == vehBase)
            {
                VehicleManager::deleteTrain(*head);
            }
            else
            {
                struct PlaceDown
                {
                    EntityId head;
                    World::Pos3 pos;
                    TrackAndDirection tad;
                    uint16_t subPosition;
                };
                std::optional<PlaceDown> placeArgs;
                if (head->tileX != -1)
                {
                    PlaceDown args{ head->id, head->getTrackLoc(), head->trackAndDirection, head->subPosition };
                    placeArgs = args;
                    switch (head->mode)
                    {
                        case TransportMode::road:
                        case TransportMode::rail:
                            head->liftUpVehicle();
                            break;
                        case TransportMode::air:
                            VehicleManager::vehiclePickupAir(head->id, GameCommands::Flags::apply);
                            break;
                        case TransportMode::water:
                            VehicleManager::vehiclePickupWater(head->id, GameCommands::Flags::apply);
                            break;
                    }
                }
                Vehicles::Car car(vehBase);
                VehicleManager::deleteCar(car);
                head->sub_4AF7A4();
                head->sub_4B7CC3();
                head->applyBreakdownToTrain();
                if (placeArgs.has_value())
                {
                    train = Vehicle(*head);
                    if (train.cars.empty())
                    {
                        train.tail->trainDanglingTimeout = 0;
                    }
                    else
                    {
                        VehicleManager::placeDownVehicle(head, placeArgs->pos.x, placeArgs->pos.y, placeArgs->pos.z / World::kSmallZStep, placeArgs->tad, placeArgs->subPosition);
                    }
                }
            }
        }
        else
        {
            if (!GameCommands::sub_431E6A(vehBase->owner))
            {
                return GameCommands::FAILURE;
            }
            if (!head->canBeModified())
            {
                // Error message set by canBeModified
                return GameCommands::FAILURE;
            }
            GameCommands::setPosition(head->position);
        }
        return -refundCost;
    }

    // 0x004AED34
    void sell(Interop::registers& regs)
    {
        regs.ebx = sell(EntityId(regs.dx), regs.bl);
    }
}
