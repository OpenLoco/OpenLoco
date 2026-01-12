#include "VehiclePlaceWater.h"
#include "Audio/Audio.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "Localisation/StringIds.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Objects/DockObject.h"
#include "Objects/ObjectManager.h"
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

namespace OpenLoco::GameCommands
{
    // 0x0048B199
    static void playWaterPlacedownSound(const World::Pos3 pos)
    {
        const auto frequency = gPrng2().randNext(20003, 24095);
        Audio::playSound(Audio::SoundId::constructShip, pos, -600, frequency);
    }

    // 0x004267BE
    static uint32_t vehiclePlaceWater(const VehicleWaterPlacementArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::ShipRunningCosts);
        setPosition(args.pos + World::Pos3{ 32, 32, 0 }); // Odd why 32,32

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
            auto* elStation = [pos = args.pos]() -> World::StationElement* {
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

            auto* station = StationManager::get(elStation->stationId());

            if (!sub_431E6A(station->owner))
            {
                return FAILURE;
            }

            if (elStation->isFlag6())
            {
                setErrorText(StringIds::vehicle_approaching_or_in_the_way);
                return FAILURE;
            }

            auto* dockObj = ObjectManager::get<DockObject>(elStation->objectId());

            const auto boatPos = World::Pos3(Math::Vector::rotate(dockObj->boatPosition, elStation->rotation()), 0) + args.pos + World::Pos3(32, 32, 0);

            const auto waterHeight = World::TileManager::getHeight(boatPos).waterHeight;
            if (waterHeight == 0)
            {
                setErrorText(StringIds::noWater);
                return FAILURE;
            }

            if (!(flags & Flags::apply))
            {
                return 0;
            }

            auto yaw = ((elStation->rotation() + 1) & 0x3) * 16;

            head->moveBoatTo(boatPos, yaw, Pitch::flat);
            head->moveTo(boatPos + World::Pos3(0, 0, 32));

            head->status = Vehicles::Status::stopped;
            head->vehicleFlags |= Vehicles::VehicleFlags::commandStop;
            head->stationId = elStation->stationId();
            head->tileX = args.pos.x;
            head->tileY = args.pos.y;
            head->tileBaseZ = args.pos.z / World::kSmallZStep;

            elStation->setFlag6(true);

            train.veh1->var_48 |= Vehicles::Flags48::flag2;

            if (flags & Flags::ghost)
            {
                train.applyToComponents([](auto& component) {
                    component.var_38 |= Vehicles::Flags38::isGhost;
                });
            }
        }

        if ((flags & Flags::apply) && !(flags & Flags::ghost))
        {
            playWaterPlacedownSound(getPosition());
        }
        return 0;
    }

    void vehiclePlaceWater(registers& regs)
    {
        regs.ebx = vehiclePlaceWater(VehicleWaterPlacementArgs(regs), regs.bl);
    }
}
