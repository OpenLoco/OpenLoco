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
    // 0x004A8826
    void update()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !isEditorMode())
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
    static void determineAvailableVehicleTypes(Company& company)
    {
        uint16_t availableTypes = 0;

        for (uint32_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::vehicle); ++i)
        {
            if (!company.unlockedVehicles[i])
            {
                continue;
            }

            auto* vehObj = ObjectManager::get<VehicleObject>(i);
            if (vehObj == nullptr)
            {
                continue;
            }

            availableTypes |= 1U << enumValue(vehObj->type);
        }
        company.availableVehicles = availableTypes;
    }

    // 0x004C3A0C
    void determineAvailableVehicles(Company& company)
    {
        std::fill(std::begin(company.unlockedVehicles), std::end(company.unlockedVehicles), 0);

        const auto curYear = getCurrentYear();

        for (uint32_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::vehicle); ++i)
        {
            auto* vehObj = ObjectManager::get<VehicleObject>(i);
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

            company.unlockedVehicles.set(i, true);
        }

        determineAvailableVehicleTypes(company);
    }

    // 0x004B05E4
    void placeDownVehicle(Vehicles::VehicleHead* const head, const coord_t x, const coord_t y, const uint8_t baseZ, const Vehicles::TrackAndDirection& unk1, const uint16_t unk2)
    {
        registers regs{};
        regs.esi = X86Pointer(head);
        regs.ax = x;
        regs.cx = y;
        regs.bx = unk2;
        regs.dl = baseZ;
        regs.ebp = unk1.track._data;
        call(0x004B05E4, regs);
    }

    // 0x004AEFB5
    void deleteCar(Vehicles::Car& car)
    {
        registers regs;
        regs.esi = X86Pointer(car.front);
        call(0x004AEFB5, regs);
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
        EntityManager::freeEntity(train.tail);
        EntityManager::freeEntity(train.veh2);
        EntityManager::freeEntity(train.veh1);
        EntityManager::freeEntity(train.head);
        CompanyManager::get(companyId)->recalculateTransportCounts();
    }
}
