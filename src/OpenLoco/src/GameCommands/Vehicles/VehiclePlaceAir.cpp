#include "GameCommands/Vehicles/VehiclePlaceAir.h"
#include "Audio/Audio.h"
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
#include "Vehicles/Vehicle1.h"
#include "Vehicles/Vehicle2.h"
#include "Vehicles/VehicleBody.h"
#include "Vehicles/VehicleBogie.h"
#include "Vehicles/VehicleHead.h"
#include "Vehicles/VehicleTail.h"
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
        Audio::playSound(Audio::SoundId::vehiclePlace, Audio::ChannelId::ui, pos, -600, frequency);
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
            return kFailure;
        }

        const auto pos = station->airportStartPos;
        setPosition(pos);

        auto* head = EntityManager::get<Vehicles::VehicleHead>(args.head);
        if (head == nullptr)
        {
            return kFailure;
        }

        if (!checkCompanyCompatibility(head->owner))
        {
            return kFailure;
        }

        Vehicles::Vehicle train(head->id);

        if (!args.convertGhost)
        {
            if (head->tileX != -1)
            {
                setErrorText(StringIds::empty);
                return kFailure;
            }
            if (train.cars.empty())
            {
                setErrorText(StringIds::empty);
                return kFailure;
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
                return kFailure;
            }
            if (elStation->isGhost() || elStation->isAiAllocated())
            {
                return kFailure;
            }

            auto* airportObj = ObjectManager::get<AirportObject>(elStation->objectId());
            const auto movementEdges = airportObj->getMovementEdges();
            auto previousEdgeId = 0;
            for (; previousEdgeId < airportObj->numMovementEdges; ++previousEdgeId)
            {
                if (movementEdges[previousEdgeId].nextNode == args.airportNode)
                {
                    break;
                }
            }
            if (previousEdgeId == airportObj->numMovementEdges)
            {
                return kFailure;
            }

            auto& previousMovEdge = movementEdges[previousEdgeId];
            if (station->airportMovementOccupiedEdges & previousMovEdge.mustBeClearEdges)
            {
                setErrorText(StringIds::vehicle_approaching_or_in_the_way);
                return kFailure;
            }

            if (station->airportMovementOccupiedEdges & airportObj->var_B6)
            {
                setErrorText(StringIds::vehicle_approaching_or_in_the_way);
                return kFailure;
            }

            if (!checkCompanyCompatibility(station->owner))
            {
                return kFailure;
            }

            if ((airportObj->flags & train.cars.firstCar.front->getCompatibleAirportType()) == AirportObjectFlags::none)
            {

                setErrorText(StringIds::airport_type_not_suitable_for_aircraft);
                return kFailure;
            }

            if (!(flags & Flags::apply))
            {
                return 0;
            }

            const auto placePos = getAirportMovementNodeLoc(args.stationId, args.airportNode);
            const auto previousNodePos = getAirportMovementNodeLoc(args.stationId, previousMovEdge.curNode);
            if (!placePos.has_value() || !previousNodePos.has_value())
            {
                return kFailure;
            }
            auto yaw = Vehicles::calculateYaw1FromVector(placePos->x - previousNodePos->x, placePos->y - previousNodePos->y);
            auto reverseYaw = yaw ^ (1U << 5);

            auto* vehicleObj = ObjectManager::get<VehicleObject>(train.cars.firstCar.front->objectId);
            const auto pitch = vehicleObj->hasFlags(VehicleObjectFlags::aircraftIsTaildragger) ? Pitch::up12deg : Pitch::flat;

            head->movePlaneTo(*placePos, reverseYaw, pitch);
            head->status = Vehicles::Status::stopped;
            head->vehicleFlags |= Vehicles::VehicleFlags::commandStop;
            head->stationId = args.stationId;
            head->airportMovementEdge = previousEdgeId;

            station->airportMovementOccupiedEdges |= (1U << previousEdgeId);

            train.veh1->var_48 |= Vehicles::Flags48::flag2;

            train.veh2->currentSpeed = 0.0_mph;
            train.veh2->motorState = Vehicles::MotorState::stopped;

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
