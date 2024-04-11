#include "VehiclePlaceAir.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "Localisation/StringIds.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Objects/AirportObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/VehicleObject.h"
#include "Random.h"
#include "Vehicles/Vehicle.h"
#include "ViewportManager.h"
#include "World/StationManager.h"

using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    // TODO: MOVE TO SOMEWHERE ELSE
    // 0x0048B11D
    void playPlacedownSound(const World::Pos3 pos)
    {
        const auto frequency = gPrng2().randNext(20003, 24095);
        Audio::playSound(Audio::SoundId::vehiclePlace, pos, -600, frequency);
    }
}
namespace OpenLoco::GameCommands
{
    // 0x004267BE
    static uint32_t vehiclePlaceAir(const VehicleAirPlacementArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::AircraftRunningCosts);

        auto* station = StationManager::get(args.stationId);
        if (station == nullptr)
        {
            return FAILURE;
        }

        const auto pos = station->airportStartPos;
        setPosition(pos);

        auto* head = EntityManager::get<Vehicles::VehicleHead>(args.head);
        if (head == nullptr)
        {
            return FAILURE;
        }

        if (!sub_431E6A(head->owner))
        {
            return FAILURE;
        }

        Vehicles::Vehicle train(head->id);

        if (!args.convertGhost)
        {
            if (head->tileX != -1)
            {
                setErrorText(StringIds::empty);
                return FAILURE;
            }
            if (train.cars.empty())
            {
                setErrorText(StringIds::empty);
                return FAILURE;
            }
        }

        if (args.convertGhost)
        {
            train.applyToComponents([](auto& component) {
                component.var_38 &= ~Vehicles::Flags38::isGhost;
                Ui::ViewportManager::invalidate(&component, ZoomLevel::eighth);
            });
        }
        else
        {
            auto* elStation = [pos]() -> World::StationElement* {
                const auto tile = World::TileManager::get(pos);
                for (auto& el : tile)
                {
                    auto* elStation = el.as<World::StationElement>();
                    if (elStation == nullptr)
                    {
                        continue;
                    }
                    if (elStation->baseHeight() != pos.z)
                    {
                        continue;
                    }
                    return elStation;
                }
                return nullptr;
            }();
            if (elStation == nullptr)
            {
                return FAILURE;
            }
            if (elStation->isGhost() || elStation->isAiAllocated())
            {
                return FAILURE;
            }

            auto* airportObj = ObjectManager::get<AirportObject>(elStation->objectId());
            auto previousEdgeId = 0;
            for (; previousEdgeId < airportObj->numMovementEdges; ++previousEdgeId)
            {
                if (airportObj->movementEdges[previousEdgeId].nextNode == args.airportNode)
                {
                    break;
                }
            }
            if (previousEdgeId == airportObj->numMovementEdges)
            {
                return FAILURE;
            }

            auto& previousMovEdge = airportObj->movementEdges[previousEdgeId];
            if (station->airportMovementOccupiedEdges & previousMovEdge.mustBeClearEdges)
            {
                setErrorText(StringIds::vehicle_approaching_or_in_the_way);
                return FAILURE;
            }

            if (station->airportMovementOccupiedEdges & airportObj->var_B6)
            {
                setErrorText(StringIds::vehicle_approaching_or_in_the_way);
                return FAILURE;
            }

            if (!sub_431E6A(station->owner))
            {
                return FAILURE;
            }

            if (!(airportObj->allowedPlaneTypes & train.cars.firstCar.front->getPlaneType()))
            {

                setErrorText(StringIds::airport_type_not_suitable_for_aircraft);
                return FAILURE;
            }

            if (!(flags & Flags::apply))
            {
                return 0;
            }

            const auto placePos = getAirportMovementNodeLoc(args.stationId, args.airportNode);
            const auto previousNodePos = getAirportMovementNodeLoc(args.stationId, previousMovEdge.curNode);
            if (!placePos.has_value() || !previousNodePos.has_value())
            {
                return FAILURE;
            }
            auto yaw = Vehicles::calculateYaw1FromVector(placePos->x - previousNodePos->x, placePos->y - previousNodePos->y);
            auto reverseYaw = yaw ^ (1U << 5);

            auto* vehicleObj = ObjectManager::get<VehicleObject>(train.cars.firstCar.front->objectId);
            const auto pitch = vehicleObj->hasFlags(VehicleObjectFlags::unk_08) ? Pitch::up12deg : Pitch::flat;

            head->movePlaneTo(*placePos, reverseYaw, pitch);
            head->status = Vehicles::Status::stopped;
            head->vehicleFlags |= VehicleFlags::commandStop;
            head->stationId = args.stationId;
            head->airportMovementEdge = previousEdgeId;

            station->airportMovementOccupiedEdges |= (1U << previousEdgeId);

            train.veh1->var_48 |= Vehicles::Flags48::flag2;

            train.veh2->currentSpeed = 0.0_mph;
            train.veh2->var_5A = 0;

            if (flags & Flags::ghost)
            {
                train.applyToComponents([](auto& component) {
                    component.var_38 |= Vehicles::Flags38::isGhost;
                });
            }
        }

        if ((flags & Flags::apply) && !(flags & Flags::ghost))
        {
            Vehicles::playPlacedownSound(pos);
        }
        return 0;
    }

    void vehiclePlaceAir(registers& regs)
    {
        regs.ebx = vehiclePlaceAir(VehicleAirPlacementArgs(regs), regs.bl);
    }
}
