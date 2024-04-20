#include "RemovePort.h"
#include "Economy/Economy.h"
#include "Localisation/StringIds.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Objects/DockObject.h"
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
    static bool removePortTileElements(const World::Pos3& pos, const uint8_t flags)
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

            auto* stationEl = getStationEl(portPos);
            if (stationEl == nullptr)
            {
                return false;
            }

            if ((flags & Flags::apply) == 0)
            {
                continue;
            }

            if ((flags & (Flags::aiAllocated)) == 0)
            {
                Ui::ViewportManager::invalidate(World::Pos2(portPos), stationEl->baseHeight(), stationEl->clearHeight(), ZoomLevel::eighth);
            }

            World::TileManager::removeElement(*reinterpret_cast<World::TileElement*>(stationEl));
        }

        return true;
    }

    // 0x0048D2AC
    static currency32_t removePort(const PortRemovalArgs& args, const uint8_t flags)
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
        const auto rotation = stationEl->rotation();

        // Do we need to remove the port from a station?
        if ((flags & Flags::ghost) == 0)
        {
            stationId = stationEl->stationId();
        }

        // Station in use by a vehicle?
        if (stationEl->isFlag6())
        {
            GameCommands::setErrorText(StringIds::currently_in_use_by_at_least_one_vehicle);
            return FAILURE;
        }

        // Calculate base removal cost
        auto* dockObj = ObjectManager::get<DockObject>(stationEl->objectId());
        currency32_t totalCost = Economy::getInflationAdjustedCost(dockObj->sellCostFactor, dockObj->costIndex, 7);

        // Remove the actual tile elements associated with the port
        if (!removePortTileElements(args.pos, flags))
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

    void removePort(registers& regs)
    {
        regs.ebx = removePort(PortRemovalArgs(regs), regs.bl);
    }
}
