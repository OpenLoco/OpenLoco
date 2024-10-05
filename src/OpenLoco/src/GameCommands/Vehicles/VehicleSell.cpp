#include "VehicleSell.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "VehiclePickupAir.h"
#include "VehiclePickupWater.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"

using namespace OpenLoco::Vehicles;

namespace OpenLoco::GameCommands
{
    static currency32_t sellVehicle(EntityId id, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::VehicleDisposals);

        currency32_t refundCost = 0;
        auto* vehBase = EntityManager::get<VehicleBase>(id);
        if (vehBase == nullptr)
        {
            return FAILURE;
        }

        auto* head = EntityManager::get<VehicleHead>(vehBase->getHead());
        if (head == nullptr)
        {
            return FAILURE;
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
                return FAILURE;
            }
            refundCost = bogie->refundCost;
        }

        if (flags & Flags::apply)
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
                        {
                            GameCommands::VehiclePickupAirArgs airArgs{};
                            airArgs.head = head->id;
                            GameCommands::doCommand(airArgs, Flags::apply);
                            break;
                        }
                        case TransportMode::water:
                        {
                            GameCommands::VehiclePickupWaterArgs waterArgs{};
                            waterArgs.head = head->id;
                            GameCommands::doCommand(waterArgs, Flags::apply);
                        }
                    }
                }
                Vehicles::Car car(vehBase);
                VehicleManager::deleteCar(car);
                head->autoLayoutTrain();
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
            if (!sub_431E6A(vehBase->owner))
            {
                return FAILURE;
            }
            if (!head->canBeModified())
            {
                // Error message set by canBeModified
                return FAILURE;
            }
            setPosition(head->position);
        }
        return -refundCost;
    }

    // 0x004AED34
    void sellVehicle(Interop::registers& regs)
    {
        regs.ebx = sellVehicle(EntityId(regs.dx), regs.bl);
    }
}
