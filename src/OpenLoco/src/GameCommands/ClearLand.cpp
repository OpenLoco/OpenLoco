#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "GameCommands.h"
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
#include <set>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

using OpenLoco::World::TileClearance::LessThanPos3;

namespace OpenLoco::GameCommands
{
    // 0x004690FC
    static void setTerrainStyleAsCleared(World::Pos2 pos)
    {
        auto* surface = World::TileManager::get(pos).surface();
        if (surface == nullptr)
        {
            return;
        }
        if (surface->isIndustrial())
        {
            return;
        }
        if (surface->var_6_SLR5() > 0)
        {
            surface->setVar6SLR5(0);
            surface->setVar4SLR5(0);

            Ui::ViewportManager::invalidate(pos, surface->baseHeight(), surface->baseHeight() + 32, ZoomLevel::eighth);
        }
        if (surface->var_4_E0() > 0)
        {
            surface->setVar4SLR5(0);

            Ui::ViewportManager::invalidate(pos, surface->baseHeight(), surface->baseHeight() + 32, ZoomLevel::eighth);
        }
    }

    // 0x00469D76
    static uint32_t clearTile(World::Pos2 pos, std::set<World::Pos3, LessThanPos3>& removedBuildings, const uint8_t flags)
    {
        // This shoudn't happen due to using TilePosRangeView
        if (!World::validCoords(pos))
        {
            GameCommands::setErrorText(StringIds::off_edge_of_map);
            return GameCommands::FAILURE;
        }

        if (flags & GameCommands::Flags::apply)
        {
            if (!isEditorMode())
            {
                setTerrainStyleAsCleared(pos);
            }

            auto tileHeight = World::TileManager::getHeight(pos);
            TileManager::removeAllWallsOnTile(World::toTileSpace(pos), tileHeight.landHeight / 4);
        }

        World::QuarterTile qt(0xF, 0);

        currency32_t cost{};
        // Bind our local vars to the tile clear function
        auto clearFunc = [pos, &removedBuildings, flags, &cost](World::TileElement& el) {
            return TileClearance::tileClearFunction(el, pos, removedBuildings, flags, cost);
        };

        auto tileHeight = World::TileManager::getHeight(pos);
        if (TileClearance::sub_462908(pos, tileHeight.landHeight / 4, tileHeight.landHeight / 4, qt, clearFunc))
            return cost;
        else
            return GameCommands::FAILURE;
    }

    // 0x00469CCB
    static uint32_t clearLand(const ClearLandArgs& args, const uint8_t flags)
    {
        World::TilePosRangeView tileLoop{ World::toTileSpace(args.pointA), World::toTileSpace(args.pointB) };
        uint32_t totalCost = 0;

        // We keep track of removed buildings for each tile visited
        // this prevents accidentally double counting their removal
        // cost if they span across multiple tiles.
        std::set<World::Pos3, LessThanPos3> removedBuildings{};

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
