#include "CreatePort.h"
#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "Localisation/StringIds.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/DockObject.h"
#include "Objects/ObjectManager.h"
#include "World/CompanyManager.h"
#include "World/Industry.h"
#include "World/StationManager.h"

namespace OpenLoco::GameCommands
{
    // Used by company ai
    static loco_global<StationId, 0x0112C748> _lastPlacedDockStationId;
    static loco_global<uint32_t, 0x00112C734> _lastConstructedAdjoiningStationId;           // Can be 0xFFFF'FFFFU for no adjoining station
    static loco_global<World::Pos2, 0x00112C792> _lastConstructedAdjoiningStationCentrePos; // Can be x = -1 for no adjoining station

    struct NearbyStation
    {
        StationId id;
        bool isPhysicallyAttached;
    };

    // 0x0049060C
    static NearbyStation sub_49060C(World::Pos3 pos, uint8_t dockObjectId, uint8_t rotation)
    {
        registers regs;
        regs.eax = (pos.x & 0xFFFFU); // eax as we need to empty upper portion of eax
        regs.cx = pos.y;
        regs.dx = pos.z;
        regs.bl = dockObjectId;
        regs.bh = rotation;
        call(0x0049060C, regs);
        NearbyStation result{};
        result.id = static_cast<StationId>(regs.bx);
        result.isPhysicallyAttached = regs.eax & (1U << 31);
        return result;
    }

    // 0x004906AC
    static NearbyStation sub_4906AC(World::Pos3 pos, uint8_t dockObjectId, uint8_t rotation)
    {
        // This one is for ai preview allocated docks
        registers regs;
        regs.eax = (pos.x & 0xFFFFU); // eax as we need to empty upper portion of eax
        regs.cx = pos.y;
        regs.dx = pos.z;
        regs.bl = dockObjectId;
        regs.bh = rotation;
        call(0x004906AC, regs);
        NearbyStation result{};
        result.id = static_cast<StationId>(regs.bx);
        result.isPhysicallyAttached = regs.eax & (1U << 31);
        return result;
    }

    enum class NearbyStationValidation
    {
        okay,
        requiresNewStation,
        failure,
    };

    // 0x0048BDCE & 0x0048BD40
    static std::pair<NearbyStationValidation, StationId> validateNearbyStation(const World::Pos3 pos, const uint8_t docksObjectId, const uint8_t rotation, const uint8_t flags)
    {
        auto func = (flags & Flags::aiAllocated) ? &sub_4906AC : &sub_49060C;
        auto nearbyStation = func(pos, docksObjectId, rotation);
        if (nearbyStation.id == StationId::null)
        {
            return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
        }

        // _lastPlacedTrackStationId = nearbyStation.id; set in callers
        auto* station = StationManager::get(nearbyStation.id);
        if (station->stationTileSize >= std::size(station->stationTiles))
        {
            if (nearbyStation.isPhysicallyAttached)
            {
                setErrorText(StringIds::station_too_large);
                return std::make_pair(NearbyStationValidation::failure, StationId::null);
            }
            return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
        }
        else
        {
            if (!(flags & Flags::aiAllocated))
            {
                if (StationManager::exceedsStationSize(*station, pos))
                {
                    if (nearbyStation.isPhysicallyAttached)
                    {
                        setErrorText(StringIds::station_too_spread_out);
                        return std::make_pair(NearbyStationValidation::failure, StationId::null);
                    }
                    return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
                }
            }
        }
        return std::make_pair(NearbyStationValidation::okay, nearbyStation.id);
    }

    // 0x004FEB80 & 0x004FEB90
    static constexpr std::array<std::array<World::Pos2, 2>, 4> kRotationToBuildingFront = {
        std::array<World::Pos2, 2>{
            World::Pos2{ 64, 0 },
            World::Pos2{ 64, 32 },
        },
        std::array<World::Pos2, 2>{
            World::Pos2{ 0, -32 },
            World::Pos2{ 32, -32 },
        },
        std::array<World::Pos2, 2>{
            World::Pos2{ -32, 32 },
            World::Pos2{ -32, 0 },
        },
        std::array<World::Pos2, 2>{
            World::Pos2{ 32, 64 },
            World::Pos2{ 0, 64 },
        },
    };

    static bool tileHasWater(World::Pos2 pos)
    {
        if (!World::validCoords(pos))
        {
            return false;
        }
        auto* elSurface = World::TileManager::get(pos).surface();
        if (elSurface->water() == 0)
        {
            return false;
        }
        return true;
    }

    static currency32_t createPort(const PortPlacementArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));

        _lastPlacedDockStationId = StationId::null;

        if ((flags & Flags::apply) && !(flags & Flags::aiAllocated))
        {
            companySetObservation(getUpdatingCompanyId(), ObservationStatus::buildingDock, World::Pos2(args.pos) + World::Pos2{ 16, 16 }, EntityId::null, args.type);
        }

        _lastConstructedAdjoiningStationCentrePos = World::Pos2(-1, -1);
        _lastConstructedAdjoiningStationId = 0xFFFFFFFFU;

        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        for (auto& frontTile : kRotationToBuildingFront[args.rotation])
        {
            const auto frontPos = frontTile + args.pos;
            if (!tileHasWater(frontPos))
            {
                setErrorText(StringIds::requires_water_in_front_of_dock);
                return FAILURE;
            }
        }

        if ((flags & Flags::ghost) && (flags & Flags::apply))
        {
            _lastConstructedAdjoiningStationCentrePos = args.pos;
            auto nearbyStation = flags & Flags::aiAllocated ? sub_4906AC(args.pos, args.type, args.rotation) : sub_49060C(args.pos, args.type, args.rotation);
            _lastConstructedAdjoiningStationId = static_cast<int16_t>(nearbyStation.id);
        }

        if (!(flags & Flags::ghost))
        {
            if (flags & Flags::apply)
            {
                auto [result, nearbyStationId] = validateNearbyStation(args.pos, args.type, args.rotation, flags);
                switch (result)
                {
                    case NearbyStationValidation::failure:
                        // Odd???
                        break;
                    case NearbyStationValidation::requiresNewStation:
                    {
                        const auto newStationId = StationManager::allocateNewStation(args.pos, getUpdatingCompanyId(), 0);
                        if (newStationId != StationId::null)
                        {
                            _lastPlacedDockStationId = newStationId;
                            auto* station = StationManager::get(newStationId);
                            station->updateLabel();
                        }
                    }
                    break;
                    case NearbyStationValidation::okay:
                        _lastPlacedDockStationId = nearbyStationId;
                        break;
                }
            }
            else
            {
                // Same as the other branch but deallocate after allocating and return failure on failure
                auto [result, nearbyStationId] = validateNearbyStation(args.pos, args.type, args.rotation, flags);
                switch (result)
                {
                    case NearbyStationValidation::failure:
                        return FAILURE;
                    case NearbyStationValidation::requiresNewStation:
                    {
                        const auto newStationId = StationManager::allocateNewStation(args.pos, getUpdatingCompanyId(), 0);
                        if (newStationId == StationId::null)
                        {
                            return FAILURE;
                        }
                        StationManager::deallocateStation(newStationId);
                        // _lastPlacedDockStationId not set but that's fine since this is the no apply side
                    }
                    break;
                    case NearbyStationValidation::okay:
                        _lastPlacedDockStationId = nearbyStationId;
                        break;
                }
            }
        }

        auto* dockObj = ObjectManager::get<DockObject>(args.type);
        currency32_t totalCost = Economy::getInflationAdjustedCost(dockObj->buildCostFactor, dockObj->costIndex, 7);

        std::set<World::Pos3, World::LessThanPos3> removedBuildings{};
        const auto buildingCost = createBuilding(args, flags, removedBuildings);
        if (buildingCost == FAILURE)
        {
            return FAILURE;
        }
        totalCost += buildingCost;

        if (!(flags & Flags::ghost) && (flags & Flags::apply))
        {
            addTileToStation(_lastPlacedDockStationId, args.pos, args.rotation);

            auto* station = StationManager::get(_lastPlacedDockStationId);
            station->invalidate();
            recalculateStationModes(_lastPlacedDockStationId);
            recalculateStationCenter(_lastPlacedDockStationId);
            station->updateLabel();
            station->invalidate();
            sub_48D794(*station);
        }
        if (!(flags & Flags::ghost | Flags::aiAllocated) && (flags & Flags::apply))
        {
            playPlacementSound(args.pos);
        }

        // Vanilla did this check wrong
        if ((flags & Flags::apply) && CompanyManager::isPlayerCompany(getUpdatingCompanyId()))
        {
            companyEmotionEvent(getUpdatingCompanyId(), Emotion::thinking);
        }
        return totalCost;
    }

    // 0x00493AA7
    void createPort(registers& regs)
    {
        regs.ebx = createPort(PortPlacementArgs(regs), regs.bl);
    }
}
