#include "ClearLand.h"
#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/StringIds.h"
#include "Map/BuildingElement.h"
#include "Map/QuarterTile.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/TreeObject.h"
#include "SceneManager.h"
#include "ViewportManager.h"

using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    // 0x00469D76
    static uint32_t clearTile(World::Pos2 pos, World::TileClearance::RemovedBuildings& removedBuildings, const uint8_t flags)
    {
        // This shouldn't happen due to using TilePosRangeView
        if (!World::TileManager::validCoords(pos))
        {
            GameCommands::setErrorText(StringIds::off_edge_of_map);
            return GameCommands::FAILURE;
        }

        if (flags & GameCommands::Flags::apply)
        {
            if (!SceneManager::isEditorMode())
            {
                TileManager::setTerrainStyleAsCleared(pos);
            }

            auto tileHeight = World::TileManager::getHeight(pos);
            TileManager::removeAllWallsOnTileAbove(World::toTileSpace(pos), tileHeight.landHeight / 4);
        }

        World::QuarterTile qt(0xF, 0);

        currency32_t cost{};
        // Bind our local vars to the tile clear function
        auto clearFunc = [pos, &removedBuildings, flags, &cost](World::TileElement& el) {
            return TileClearance::clearWithoutDefaultCollision(el, pos, removedBuildings, flags, cost);
        };

        auto tileHeight = World::TileManager::getHeight(pos);
        if (TileClearance::applyClearAtAllHeights(pos, tileHeight.landHeight / 4, tileHeight.landHeight / 4, qt, clearFunc))
        {
            return cost;
        }
        else
        {
            return GameCommands::FAILURE;
        }
    }

    // 0x00469CCB
    static uint32_t clearLand(const ClearLandArgs& args, const uint8_t flags)
    {
        const auto tileLoop = World::getClampedRange(args.pointA, args.pointB);
        uint32_t totalCost = 0;

        // We keep track of removed buildings for each tile visited
        // this prevents accidentally double counting their removal
        // cost if they span across multiple tiles.
        World::TileClearance::RemovedBuildings removedBuildings{};

        for (const auto& tilePos : tileLoop)
        {
            uint32_t tileRes = clearTile(World::toWorldSpace(tilePos), removedBuildings, flags);
            if (tileRes == GameCommands::FAILURE)
            {
                return GameCommands::FAILURE;
            }
            else
            {
                totalCost += tileRes;
            }
        }

        GameCommands::setExpenditureType(ExpenditureType::Construction);

        auto tileHeight = World::TileManager::getHeight(args.centre);
        GameCommands::setPosition(World::Pos3(args.centre.x, args.centre.y, tileHeight.landHeight));

        return totalCost;
    }

    void clearLand(registers& regs)
    {
        const ClearLandArgs args(regs);
        regs.ebx = clearLand(args, regs.bl);
    }
}
