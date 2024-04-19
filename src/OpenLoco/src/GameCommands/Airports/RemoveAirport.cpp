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
#include "ViewportManager.h"
#include "World/Station.h"
#include "World/StationManager.h"

namespace OpenLoco::GameCommands
{
    // TODO: Copied from Industry.h
    struct Unk4F9274
    {
        World::Pos2 pos;
        uint8_t index;
    };

    // TODO: Copied from Industry.cpp
    static const std::array<Unk4F9274, 4> word_4F927C = {
        Unk4F9274{ { 0, 0 }, 0 },
        Unk4F9274{ { 0, 32 }, 1 },
        Unk4F9274{ { 32, 32 }, 2 },
        Unk4F9274{ { 32, 0 }, 3 },
    };

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

    // 0x00494706
    static bool removeAirportTileElements(const World::Pos3& pos, const uint8_t flags)
    {
        for (auto& searchTile : word_4F927C)
        {
            const auto portPos = World::Pos3(searchTile.pos + pos, pos.z);

            if ((flags & (Flags::aiAllocated | Flags::apply)) != 0)
            {
                auto tile = World::TileManager::get(portPos);
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

            auto* stationEl = getStationEl(portPos);
            if (stationEl == nullptr)
            {
                return false;
            }

            if ((flags & (Flags::aiAllocated)) == 0)
            {
                Ui::ViewportManager::invalidate(World::Pos2(portPos), stationEl->baseHeight(), stationEl->clearHeight(), ZoomLevel::eighth);
            }

            World::TileManager::removeElement(*reinterpret_cast<World::TileElement*>(stationEl));
        }

        return true;
    }

    // 0x0049372F
    static currency32_t loc_49372F(const World::StationElement& stationEl, const AirportRemovalArgs& args, const uint8_t flags)
    {

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
            return loc_49372F(*stationEl, args, flags);
        }

        // 0x004935DC
        stationId = stationEl->stationId();
        auto* station = StationManager::get(stationId);

        // Try to find airport station tile at the specified Z coordinate
        World::StationElement* foundStationEl = nullptr;
        World::Pos3* foundPos = nullptr;
        for (auto i = 0U; i < station->stationTileSize; i++)
        {
            auto& tilePos = station->stationTiles[i];
            if (World::heightFloor(tilePos.z) != args.pos.z)
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
            if (stationEl->unk5SHR5() == 2) // airport type?
            {
                foundStationEl = stationEl;
                foundPos = &tilePos;
                break;
            }
        }

        if (foundStationEl == nullptr || foundPos == nullptr)
        {
            return FAILURE;
        }

        // 0x0049365A
        auto rotation = foundPos->z & 3; // dx
        auto* airportObj = ObjectManager::get<AirportObject>(stationEl->objectId());

        auto minPos = World::toWorldSpace(World::TilePos2(airportObj->minX, airportObj->minY));
        auto maxPos = World::toWorldSpace(World::TilePos2(airportObj->maxX, airportObj->maxY));

        minPos = Math::Vector::rotate(minPos, rotation);
        maxPos = Math::Vector::rotate(maxPos, rotation);

        // 0x004936CD
        minPos += World::Pos2{foundPos->x, foundPos->y};
        maxPos += World::Pos2{foundPos->x, foundPos->y};

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

        // !!!

        // Calculate base removal cost
        currency32_t totalCost = Economy::getInflationAdjustedCost(airportObj->sellCostFactor, airportObj->costIndex, 7);

        // Remove the actual tile elements associated with the port
        if (!removeAirportTileElements(args.pos, flags))
        {
            return FAILURE;
        }

        // Should we update the station meta data?
        if ((flags & Flags::ghost) == 0 && (flags & Flags::apply) != 0)
        {
            auto* station = StationManager::get(stationId);

            removeTileFromStationAndRecalcCargo(stationId, args.pos, rotation);
            station->invalidate();

            recalculateStationModes(stationId);
            recalculateStationCenter(stationId);
            station->updateLabel();
            station->invalidate();
        }

        return totalCost;
    }

    void removeAirport(registers& regs)
    {
        regs.ebx = removeAirport(AirportRemovalArgs(regs), regs.bl);
    }
}
