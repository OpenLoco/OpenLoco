#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "GameCommands.h"
#include "Localisation/StringIds.h"
#include "Map/BuildingElement.h"
#include "Map/QuarterTile.h"
#include "Map/SurfaceElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/TreeObject.h"
#include "S5/S5.h"
#include "SceneManager.h"
#include "ViewportManager.h"
#include <set>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    static loco_global<uint32_t, 0x00F0013C> _F0013C;
    static loco_global<uint32_t, 0x00F00140> _F00140;
    static loco_global<uint16_t, 0x00F00144> _F00144;

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

    struct LessThanPos3
    {
        bool operator()(World::Pos3 const& lhs, World::Pos3 const& rhs) const
        {
            return std::tie(lhs.x, lhs.y, lhs.z) < std::tie(rhs.x, rhs.y, rhs.z);
        }
    };

    // 0x00469E07
    static TileManager::ClearFuncResult tileClearFunction(World::TileElement& el, const World::Pos2 pos, std::set<World::Pos3, LessThanPos3>& removedBuildings, const uint8_t flags, currency32_t& cost)
    {
        switch (el.type())
        {
            case ElementType::tree:
            {
                auto* elTree = el.as<TreeElement>();
                if (elTree == nullptr)
                {
                    return TileManager::ClearFuncResult::noCollision;
                }
                auto* treeObj = ObjectManager::get<TreeObject>(elTree->treeObjectId());
                cost += Economy::getInflationAdjustedCost(treeObj->clearCostFactor, treeObj->costIndex, 12);

                if (flags & GameCommands::Flags::flag_6 || !(flags & GameCommands::Flags::apply))
                {
                    return TileManager::ClearFuncResult::noCollision;
                }

                World::TileManager::setRemoveElementPointerChecker(el);
                World::TileManager::removeTree(*elTree, GameCommands::Flags::apply, pos);
                S5::getOptions().madeAnyChanges = 1;
                if (World::TileManager::wasRemoveOnLastElement())
                {
                    return TileManager::ClearFuncResult::allCollisionsRemoved;
                }
                return TileManager::ClearFuncResult::collisionRemoved;
            }
            case ElementType::building:
            {
                auto* elBuilding = el.as<BuildingElement>();
                if (elBuilding == nullptr)
                {
                    return TileManager::ClearFuncResult::noCollision;
                }
                auto* buildingObj = elBuilding->getObject();
                if (buildingObj->hasFlags(BuildingObjectFlags::isHeadquarters))
                {
                    return TileManager::ClearFuncResult::collision;
                }

                const auto buildingStart = World::Pos3{
                    pos - World::offsets[elBuilding->multiTileIndex()], elBuilding->baseHeight()
                };
                if (removedBuildings.count(buildingStart) != 0)
                {
                    return TileManager::ClearFuncResult::noCollision;
                }
                removedBuildings.insert(buildingStart);

                World::TileManager::setRemoveElementPointerChecker(el);
                uint8_t removeBuildingFlags = flags;
                if ((flags & GameCommands::Flags::apply) || removedBuildings.size() != 1)
                {
                    removeBuildingFlags |= GameCommands::Flags::flag_7;
                }
                if (flags & GameCommands::Flags::flag_6)
                {
                    removeBuildingFlags &= ~(GameCommands::Flags::flag_6 | GameCommands::Flags::apply);
                }
                GameCommands::BuildingRemovalArgs args{};
                args.pos = buildingStart;
                Interop::registers regs = static_cast<Interop::registers>(args);
                regs.bl = removeBuildingFlags;
                // We should probably call doCommand here but then it gets messy with the costs
                // look into changing this in the future.
                removeBuilding(regs);
                const auto buildingCost = static_cast<currency32_t>(regs.ebx);
                if (static_cast<uint32_t>(buildingCost) == GameCommands::FAILURE)
                {
                    return TileManager::ClearFuncResult::collisionErrorSet;
                }
                if (flags & GameCommands::Flags::apply)
                {
                    S5::getOptions().madeAnyChanges = 1;
                }
                cost += buildingCost;

                if (!(flags & GameCommands::Flags::apply) || flags & GameCommands::Flags::flag_6)
                {
                    return TileManager::ClearFuncResult::noCollision;
                }
                if (World::TileManager::wasRemoveOnLastElement())
                {
                    return TileManager::ClearFuncResult::allCollisionsRemoved;
                }
                return TileManager::ClearFuncResult::collisionRemoved;
            }
            default:
                return TileManager::ClearFuncResult::noCollision;
        }
    };

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
            return tileClearFunction(el, pos, removedBuildings, flags, cost);
        };

        auto tileHeight = World::TileManager::getHeight(pos);
        if (TileManager::sub_462908(pos, tileHeight.landHeight / 4, tileHeight.landHeight / 4, qt, clearFunc))
            return cost;
        else
            return GameCommands::FAILURE;
    }

    // 0x00469CCB
    static uint32_t clearLand(const ClearLandArgs& args, const uint8_t flags)
    {
        _F0013C = args.pointB.x << 16 | args.pointA.x;
        _F00140 = args.pointB.y << 16 | args.pointA.y;
        _F00144 = flags;

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
