#include "VehicleManager.h"
#include "Audio/Audio.h"
#include "Date.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Vehicles/VehiclePickupAir.h"
#include "GameCommands/Vehicles/VehiclePickupWater.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Map/Track/SubpositionData.h"
#include "Map/Track/TrackData.h"
#include "MessageManager.h"
#include "Objects/ObjectManager.h"
#include "OrderManager.h"
#include "Orders.h"
#include "RoutingManager.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "Vehicle.h"
#include "Vehicle1.h"
#include "Vehicle2.h"
#include "VehicleBody.h"
#include "VehicleBogie.h"
#include "VehicleHead.h"
#include "VehicleTail.h"
#include "World/Company.h"
#include "World/CompanyManager.h"

namespace OpenLoco::VehicleManager
{
    // 0x004A8826
    void update()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !SceneManager::isEditorMode())
        {
            for (auto* v : VehicleList())
            {
                v->updateVehicle();
            }
        }
    }

    // 0x004C3C54
    void updateMonthly()
    {
        for (auto v : VehicleList())
        {
            v->updateMonthly();
        }
    }

    // 0x004B94CF
    void updateDaily()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !SceneManager::isEditorMode())
        {
            for (auto* v : VehicleList())
            {
                v->updateDaily();
            }
            Ui::WindowManager::invalidate(Ui::WindowType::vehicleList);
        }
    }

    // 0x004C39D4
    uint16_t determineAvailableVehicleTypes(const Company& company)
    {
        uint16_t availableTypes = 0;

        for (uint32_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::vehicle); ++i)
        {
            if (!company.unlockedVehicles[i])
            {
                continue;
            }

            const auto* vehObj = ObjectManager::get<VehicleObject>(i);
            if (vehObj == nullptr)
            {
                continue;
            }

            availableTypes |= 1U << enumValue(vehObj->type);
        }
        return availableTypes;
    }

    static BitSet<224> determineUnlockedVehicles(const Company& company)
    {
        const auto curYear = getCurrentYear();
        BitSet<224> unlockedVehicles{};
        for (uint32_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::vehicle); ++i)
        {
            const auto* vehObj = ObjectManager::get<VehicleObject>(i);
            if (vehObj == nullptr)
            {
                continue;
            }

            if (curYear < vehObj->designed)
            {
                continue;
            }
            if (curYear >= vehObj->obsolete)
            {
                continue;
            }

            const auto forbiddenVehicles = CompanyManager::isPlayerCompany(company.id()) ? getGameState().forbiddenVehiclesPlayers : getGameState().forbiddenVehiclesCompetitors;
            if (forbiddenVehicles & (1U << enumValue(vehObj->type)))
            {
                continue;
            }

            unlockedVehicles.set(i, true);
        }
        return unlockedVehicles;
    }

    // 0x004C3A0C
    void determineAvailableVehicles(Company& company)
    {
        company.unlockedVehicles = determineUnlockedVehicles(company);
        company.availableVehicles = determineAvailableVehicleTypes(company);
    }

    template<typename T>
    static void moveComponentToSubPosition(T& base)
    {
        base.invalidateSprite();
        if (base.getTransportMode() == TransportMode::road)
        {
            auto& moveInfo = World::TrackData::getRoadSubPositon(base.getTrackAndDirection().road._data)[base.subPosition];
            const auto newPos = World::Pos3{ base.tileX, base.tileY, base.tileBaseZ * World::kSmallZStep } + moveInfo.loc;
            base.spriteYaw = moveInfo.yaw;
            base.spritePitch = moveInfo.pitch;
            base.moveTo(newPos);
        }
        else
        {
            auto& moveInfo = World::TrackData::getTrackSubPositon(base.getTrackAndDirection().track._data)[base.subPosition];
            const auto newPos = World::Pos3{ base.tileX, base.tileY, base.tileBaseZ * World::kSmallZStep } + moveInfo.loc;
            base.spriteYaw = moveInfo.yaw;
            base.spritePitch = moveInfo.pitch;
            base.moveTo(newPos);
        }
        base.invalidateSprite();
    }

    // 0x004B05E4
    PlaceDownResult placeDownVehicle(Vehicles::VehicleHead* const head, const coord_t x, const coord_t y, const uint8_t baseZ, const Vehicles::TrackAndDirection& trackAndDirection, const uint16_t initialSubPosition)
    {
        const auto pos = World::Pos3{ x, y, baseZ * World::kSmallZStep };
        if (head->tileX != -1)
        {
            return PlaceDownResult::Okay;
        }

        auto subPosition = 0;
        World::Pos3 reversePos = pos;
        Vehicles::TrackAndDirection reverseTad = trackAndDirection;
        if (head->mode != TransportMode::road)
        {
            if (Vehicles::findNearbySignalState(pos, trackAndDirection.track, head->owner, head->trackType) & (1U << 0))
            {
                return PlaceDownResult::VehicleApproachingOrInWay;
            }

            if (Vehicles::isBlockOccupied(pos, trackAndDirection.track, head->owner, head->trackType))
            {
                return PlaceDownResult::VehicleApproachingOrInWay;
            }

            const auto subPositionLength = static_cast<uint32_t>(World::TrackData::getTrackSubPositon(trackAndDirection.track._data).size());
            subPosition = subPositionLength - 1 - initialSubPosition;

            const auto& trackSize = World::TrackData::getUnkTrack(trackAndDirection.track._data);
            reversePos += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                reversePos -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
            reverseTad.track.setReversed(!trackAndDirection.track.isReversed());
        }
        else
        {

            const auto subPositionLength = static_cast<uint32_t>(World::TrackData::getRoadSubPositon(trackAndDirection.road._data).size());
            subPosition = subPositionLength - 1 - initialSubPosition;

            const auto& roadSize = World::TrackData::getUnkRoad(trackAndDirection.road.basicRad());
            reversePos += roadSize.pos;
            if (roadSize.rotationEnd < 12)
            {
                reversePos -= World::Pos3{ World::kRotationOffset[roadSize.rotationEnd], 0 };
            }
            reverseTad.road.setReversed(!trackAndDirection.road.isReversed());
            reverseTad.road._data ^= (1U << 7);
            if (reverseTad.road.isChangingLane())
            {
                reverseTad.road._data ^= (1U << 7);
                if (!reverseTad.road.isOvertaking())
                {
                    reverseTad.road._data ^= (1U << 8);
                }
            }
        }

        Vehicles::RoutingManager::resetRoutings(head->routingHandle);
        Vehicles::RoutingManager::setRouting(head->routingHandle, reverseTad.track._data);

        if (head->mode == TransportMode::road)
        {
            if ((Vehicles::getRoadOccupation(reversePos, reverseTad.road) & (Vehicles::RoadOccupationFlags::isLaneOccupied | Vehicles::RoadOccupationFlags::isLevelCrossingClosed)) != Vehicles::RoadOccupationFlags::none)
            {
                return PlaceDownResult::VehicleApproachingOrInWay;
            }
        }
        else
        {
            if (Vehicles::findNearbySignalState(reversePos, reverseTad.track, head->owner, head->trackType) & (1U << 0))
            {
                return PlaceDownResult::VehicleApproachingOrInWay;
            }

            if (Vehicles::isBlockOccupied(reversePos, reverseTad.track, head->owner, head->trackType))
            {
                return PlaceDownResult::VehicleApproachingOrInWay;
            }
        }

        // 0x004B077E
        auto train = Vehicles::Vehicle(*head);

        train.applyToComponents([&reversePos, reverseTad, subPosition, head](auto& component) {
            component.routingHandle = head->routingHandle;
            component.tileX = reversePos.x;
            component.tileY = reversePos.y;
            component.tileBaseZ = reversePos.z / World::kSmallZStep;
            component.subPosition = subPosition;
            component.trackAndDirection = reverseTad;
            component.remainingDistance = 0;
            moveComponentToSubPosition(component);
        });
        train.head->var_3C = 0;
        train.veh1->var_3C = 0;

        Vehicles::applyVehicleObjectLength(train);
        auto oldVar52 = head->var_52;
        head->var_52 = 1;
        const bool failure = Vehicles::positionVehicleOnTrack(*head, true);
        head->var_52 = oldVar52;

        if (failure)
        {
            head->liftUpVehicle();
            return PlaceDownResult::NotEnoughSpaceOrVehicleInWay;
        }

        head->var_52 = 1;
        head->sub_4ADB47(true);
        head->var_52 = oldVar52;
        head->status = Vehicles::Status::stopped;
        train.veh1->var_48 |= Vehicles::Flags48::flag2;

        return PlaceDownResult::Okay;
    }

    // 0x004B93DC
    void resetIfHeadingForStation(const StationId stationId)
    {
        for (auto* v : VehicleList())
        {
            Vehicles::Vehicle train(*v);
            // Stop train from heading to the station
            if (train.head->stationId == stationId)
            {
                train.head->stationId = StationId::null;
            }

            // Remove any station references from cargo
            // NOTE: Deletes the cargo!
            for (auto& car : train.cars)
            {
                for (auto& carComponent : car)
                {
                    if (carComponent.front->secondaryCargo.townFrom == stationId)
                    {
                        carComponent.front->secondaryCargo.qty = 0;
                    }
                    if (carComponent.back->secondaryCargo.townFrom == stationId)
                    {
                        carComponent.back->secondaryCargo.qty = 0;
                    }
                    if (carComponent.body->primaryCargo.townFrom == stationId)
                    {
                        carComponent.body->primaryCargo.qty = 0;
                    }
                }
            }
        }
    }

    // 0x004AEFB5
    void deleteCar(Vehicles::Car& car)
    {
        Ui::WindowManager::invalidate(Ui::WindowType::vehicle, enumValue(car.front->head));
        Ui::WindowManager::invalidate(Ui::WindowType::vehicleList);

        // Component before this car
        Vehicles::VehicleBase* previous = [&car]() {
            // Points to one behind the iterator
            Vehicles::VehicleBase* previousIter = nullptr;
            for (auto* iter = EntityManager::get<Vehicles::VehicleBase>(car.front->head);
                 iter != car.front;
                 iter = iter->nextVehicleComponent())
            {
                previousIter = iter;
            }
            return previousIter;
        }();

        // Component after this car
        Vehicles::VehicleBase* next = [&car]() {
            auto carIter = car.begin();
            // Points to one behind the iterator
            auto previousCarIter = carIter;
            while (carIter != car.end())
            {
                previousCarIter = carIter;
                carIter++;
            }
            // Last component of a car component is a body
            return (*previousCarIter).body->nextVehicleComponent();
        }();

        // Remove the whole car from the linked list
        previous->setNextCar(next->id);

        for (auto* toDeleteComponent = static_cast<Vehicles::VehicleBase*>(car.front);
             toDeleteComponent != next;)
        {
            // Must fetch the next before deleting the current!
            auto* nextDeletion = toDeleteComponent->nextVehicleComponent();

            EntityManager::freeEntity(toDeleteComponent);
            toDeleteComponent = nextDeletion;
        }
    }

    // 0x004AF06E
    void deleteTrain(Vehicles::VehicleHead& head)
    {
        Vehicles::Vehicle train(head);
        EntityId viewportFollowEntity = train.veh2->id;
        auto main = Ui::WindowManager::getMainWindow();
        if (Ui::Windows::Main::viewportIsFocusedOnEntity(*main, viewportFollowEntity))
        {
            Ui::Windows::Main::viewportUnfocusFromEntity(*main);
        }

        Ui::WindowManager::close(Ui::WindowType::vehicle, enumValue(head.id));
        auto* vehListWnd = Ui::WindowManager::find(Ui::WindowType::vehicleList, enumValue(head.owner));
        if (vehListWnd != nullptr)
        {
            vehListWnd->invalidate();
            Ui::Windows::VehicleList::removeTrainFromList(*vehListWnd, head.id);
        }
        // Change to vanilla, update the build window to a valid train
        auto* vehBuildWnd = Ui::WindowManager::find(Ui::WindowType::buildVehicle, enumValue(head.owner));
        if (vehBuildWnd != nullptr)
        {
            vehBuildWnd->invalidate();
            Ui::Windows::BuildVehicle::sub_4B92A5(vehBuildWnd);
        }

        // 0x004AF0A3
        switch (head.mode)
        {
            case TransportMode::road:
            case TransportMode::rail:
                head.liftUpVehicle();
                break;
            case TransportMode::air:
            {
                // Calling this GC directly as we need the result immediately
                // perhaps in the future this could be changed.
                GameCommands::VehiclePickupAirArgs airArgs{};
                airArgs.head = head.id;
                GameCommands::registers regs = static_cast<GameCommands::registers>(airArgs);
                regs.bl = GameCommands::Flags::apply;
                GameCommands::vehiclePickupAir(regs);
                break;
            }
            case TransportMode::water:
            {
                // Calling this GC directly as we need the result immediately
                // perhaps in the future this could be changed.
                GameCommands::VehiclePickupWaterArgs waterArgs{};
                waterArgs.head = head.id;
                GameCommands::registers regs = static_cast<GameCommands::registers>(waterArgs);
                regs.bl = GameCommands::Flags::apply;
                GameCommands::vehiclePickupWater(regs);
            }
        }

        auto* nextVeh = train.veh2->nextVehicleComponent();
        while (nextVeh != nullptr && !nextVeh->isVehicleTail())
        {
            Vehicles::Car car(nextVeh);
            deleteCar(car);
            nextVeh = train.veh2->nextVehicleComponent();
        }

        Audio::stopVehicleNoise(head.id);
        Vehicles::RoutingManager::freeRoutingHandle(head.routingHandle);
        Vehicles::OrderManager::freeOrders(&head);
        MessageManager::removeAllSubjectRefs(enumValue(head.id), MessageItemArgumentType::vehicle);
        const auto companyId = head.owner;
        CompanyManager::get(companyId)->clearOwnerStatusForDeletedVehicle(head.id);
        EntityManager::freeEntity(train.tail);
        EntityManager::freeEntity(train.veh2);
        EntityManager::freeEntity(train.veh1);
        EntityManager::freeEntity(train.head);
        CompanyManager::get(companyId)->recalculateTransportCounts();
    }
}
