#include "VehicleManager.h"
#include "Date.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Vehicles/VehiclePickupAir.h"
#include "GameCommands/Vehicles/VehiclePickupWater.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "MessageManager.h"
#include "OrderManager.h"
#include "Orders.h"
#include "RoutingManager.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "Vehicle.h"
#include "World/Company.h"
#include "World/CompanyManager.h"

#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::VehicleManager
{
    void honkAllTrains()
    {
        for (auto* v : VehicleList())
        {
            v->updateVehicle(true);
        }
    }

    // 0x004A8826
    void update()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !isEditorMode())
        {
            for (auto* v : VehicleList())
            {
                v->updateVehicle(false);
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
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !isEditorMode())
        {
            for (auto* v : VehicleList())
            {
                v->updateDaily();
            }
            Ui::WindowManager::invalidate(Ui::WindowType::vehicleList);
        }
    }

    // 0x004C39D4
    static uint16_t determineAvailableVehicleTypes(const Company& company)
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

    // 0x004B05E4
    PlaceDownResult placeDownVehicle(Vehicles::VehicleHead* const head, const coord_t x, const coord_t y, const uint8_t baseZ, const Vehicles::TrackAndDirection& unk1, const uint16_t unk2)
    {
        registers regs{};
        regs.esi = X86Pointer(head);
        regs.ax = x;
        regs.cx = y;
        regs.bx = unk2;
        regs.dl = baseZ;
        regs.ebp = unk1.track._data;
        bool hasFailed = call(0x004B05E4, regs) & X86_FLAG_CARRY;
        if (hasFailed)
        {
            return regs.al == 0 ? PlaceDownResult::Unk0 : PlaceDownResult::Unk1;
        }
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
                waterArgs.head = head.id;
                registers regs = static_cast<registers>(waterArgs);
                regs.bl = GameCommands::Flags::apply;
                GameCommands::vehiclePickupWater(regs);
            }
        }

        Vehicles::Vehicle train(head);
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
