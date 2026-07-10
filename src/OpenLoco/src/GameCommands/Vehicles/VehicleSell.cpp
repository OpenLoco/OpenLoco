#include "GameCommands/Vehicles/VehicleSell.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Vehicles/VehiclePickupAir.h"
#include "GameCommands/Vehicles/VehiclePickupWater.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleBody.h"
#include "Vehicles/VehicleBogie.h"
#include "Vehicles/VehicleHead.h"
#include "Vehicles/VehicleManager.h"
#include "Vehicles/VehicleTail.h"

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
            return kFailure;
        }

        auto* head = EntityManager::get<VehicleHead>(vehBase->getHead());
        if (head == nullptr)
        {
            return kFailure;
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
                return kFailure;
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
                head->updateTrainProperties();
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
            if (!checkCompanyCompatibility(vehBase->owner))
            {
                return kFailure;
            }
            if (!head->canBeModified())
            {
                // Error message set by canBeModified
                return kFailure;
            }
            setPosition(head->position);
        }
        return -refundCost;
    }

    // 0x004AED34
    void sellVehicle(registers& regs, const uint8_t flags)
    {
        regs.ebx = sellVehicle(EntityId(regs.dx), flags);
    }
}
