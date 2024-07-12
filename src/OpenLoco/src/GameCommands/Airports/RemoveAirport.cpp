#include "RemoveAirport.h"
#include "Economy/Economy.h"
#include "Localisation/StringIds.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Objects/AirportObject.h"
#include "Objects/ObjectManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include "ViewportManager.h"
#include "World/Industry.h"
#include "World/Station.h"
#include "World/StationManager.h"
#include "World/TownManager.h"

namespace OpenLoco::GameCommands
{
    constexpr std::array<World::Pos2, 4> kLargeTileOffsets = { {
        { 0, 0 },
        { 0, -32 },
        { -32, -32 },
        { -32, 0 },
    } };

    static World::StationElement* getStationEl(const World::Pos3& pos)
    {
        auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* stationEl = el.as<World::StationElement>();
            if (stationEl == nullptr)
            {
                continue;
            }

            if (stationEl->baseHeight() == pos.z)
            {
                return stationEl;
            }
        }

        return nullptr;
    }

    // 0x004938D9
    static bool removeAirportTileElement(const World::Pos3& pos, const AirportObject* airportObj, const uint8_t buildingIndex, const uint8_t flags)
    {
        for (auto& searchTile : getBuildingTileOffsets(airportObj->largeTiles & (1U << buildingIndex)))
        {
            const auto airportPos = World::Pos3(searchTile.pos + pos, pos.z);

            if ((flags & Flags::aiAllocated) != 0 && (flags & Flags::apply) != 0)
            {
                auto tile = World::TileManager::get(airportPos);
                auto* surfaceEl = tile.surface();
                if (surfaceEl != nullptr)
                {
                    surfaceEl->setAiAllocated(false);
                }
            }

            if ((flags & Flags::apply) == 0)
            {
                continue;
            }

            auto* stationEl = getStationEl(airportPos);
            if (stationEl == nullptr)
            {
                return false;
            }

            if ((flags & (Flags::aiAllocated)) == 0)
            {
                Ui::ViewportManager::invalidate(World::Pos2(airportPos), stationEl->baseHeight(), stationEl->clearHeight(), ZoomLevel::eighth);
            }

            World::TileManager::removeElement(*reinterpret_cast<World::TileElement*>(stationEl));
        }

        return true;
    }

    // 0x00493519
    static bool isAirportInUseByVehicle(const StationId stationId)
    {
        auto vehicleList = VehicleManager::VehicleList();
        for (auto* vehicle : vehicleList)
        {
            if (vehicle->vehicleType != VehicleType::aircraft)
                continue;

            if (vehicle->tileX == -1)
                continue;

            if (vehicle->stationId == stationId)
            {
                GameCommands::setErrorText(StringIds::currently_in_use_by_at_least_one_vehicle);
                return true;
            }
        }

        return false;
    }

    // 0x0049372F
    static currency32_t loc_49372F(const StationId stationId, const World::StationElement& stationEl, const World::Pos3 pos, const uint8_t flags)
    {
        const auto rotation = stationEl.rotation();
        const auto objectId = stationEl.objectId();

        if (stationId != StationId::null)
        {
            if (isAirportInUseByVehicle(stationId))
            {
                return FAILURE;
            }
        }

        const auto* airportObj = ObjectManager::get<AirportObject>(objectId);

        // Calculate base removal cost
        currency32_t totalCost = Economy::getInflationAdjustedCost(airportObj->sellCostFactor, airportObj->costIndex, 6);

        // Adjust number of airports for nearest town
        auto maybeTown = TownManager::getClosestTownAndDensity(pos);
        if (maybeTown && (flags & Flags::apply) != 0)
        {
            if ((flags & (Flags::aiAllocated | Flags::ghost)) == 0)
            {
                auto* town = TownManager::get(maybeTown->first);
                town->numberOfAirports--;
            }
        }

        for (auto& building : airportObj->getBuildingPositions())
        {
            // 0x004937FA
            auto offset = World::TilePos2(building.x, building.y);
            offset = Math::Vector::rotate(offset, rotation);

            auto worldPos = World::Pos3(World::toWorldSpace(offset), 0);

            if ((airportObj->largeTiles & (1 << building.index)) != 0)
            {
                worldPos.x += kLargeTileOffsets[rotation].x;
                worldPos.y += kLargeTileOffsets[rotation].y;
            }

            worldPos += pos;

            if (!removeAirportTileElement(worldPos, airportObj, building.index, flags))
            {
                return FAILURE;
            }
        }

        // 0x00493858
        // Should we update the station meta data?
        if ((flags & Flags::ghost) == 0 && (flags & Flags::apply) != 0)
        {
            auto* station = StationManager::get(stationId);

            removeTileFromStationAndRecalcCargo(stationId, pos, rotation);
            station->flags &= ~StationFlags::flag_6;
            station->invalidate();

            recalculateStationModes(stationId);
            recalculateStationCenter(stationId);
            station->updateLabel();
            station->invalidate();
        }

        return totalCost;
    }

    // 0x00493559
    static currency32_t removeAirport(const AirportRemovalArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));

        // Find station element on tile
        auto* stationEl = getStationEl(args.pos);
        if (stationEl == nullptr)
        {
            return FAILURE;
        }

        StationId stationId = StationId::null;
        if ((flags & Flags::ghost) != 0)
        {
            return loc_49372F(stationId, *stationEl, args.pos, flags);
        }

        // 0x004935DC
        stationId = stationEl->stationId();
        auto* station = StationManager::get(stationId);

        // Try to find airport station tile at the specified Z coordinate
        World::StationElement* foundStationEl = nullptr;
        World::Pos3 foundPos{};
        for (auto i = 0U; i < station->stationTileSize; i++)
        {
            auto tilePos = station->stationTiles[i];
            auto rotation = tilePos.z & 3;
            tilePos.z = World::heightFloor(tilePos.z);

            if (tilePos.z != args.pos.z)
            {
                continue;
            }

            // Find station element on tile
            stationEl = getStationEl(tilePos);
            if (stationEl == nullptr)
            {
                return FAILURE;
            }

            // 0x0049364C
            if (stationEl->stationType() != StationType::airport)
            {
                continue;
            }

            // 0x0049365A
            auto* airportObj = ObjectManager::get<AirportObject>(stationEl->objectId());

            auto minPos = World::toWorldSpace(World::TilePos2(airportObj->minX, airportObj->minY));
            auto maxPos = World::toWorldSpace(World::TilePos2(airportObj->maxX, airportObj->maxY));

            minPos = Math::Vector::rotate(minPos, rotation);
            maxPos = Math::Vector::rotate(maxPos, rotation);

            // 0x004936CD
            minPos += World::Pos2{ tilePos.x, tilePos.y };
            maxPos += World::Pos2{ tilePos.x, tilePos.y };

            if (minPos.x > maxPos.x)
                std::swap(minPos.x, maxPos.x);

            if (minPos.y > maxPos.y)
                std::swap(minPos.y, maxPos.y);

            // Ensure that current airport tile fits within these min/max bounds
            if (args.pos.x < minPos.x || args.pos.y < minPos.y || args.pos.x > maxPos.x || args.pos.y > maxPos.y)
            {
                // We must've targetted a neighbouring airport -- look further
                continue;
            }

            foundStationEl = stationEl;
            foundPos = tilePos;
            break;
        }

        if (foundStationEl == nullptr)
        {
            return FAILURE;
        }

        // 0x00493719
        return loc_49372F(stationId, *foundStationEl, foundPos, flags);
    }

    void removeAirport(registers& regs)
    {
        regs.ebx = removeAirport(AirportRemovalArgs(regs), regs.bl);
    }
}
