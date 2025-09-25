#include "CreateTree.h"
#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/StringIds.h"
#include "Map/QuarterTile.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/TreeObject.h"
#include "OpenLoco.h"
#include "Scenario.h"
#include "ScenarioOptions.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "World/TownManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    constexpr std::array<World::Pos2, 4> _quadrantCentreOffsets = {
        World::Pos2{ 7, 7 },
        World::Pos2{ 7, 23 },
        World::Pos2{ 23, 23 },
        World::Pos2{ 23, 7 },
    };

    /**
     * 0x004BB138
     * Create tree
     *
     * This is called when you activate the Plant Trees from the construction menu and you move the cursor over the terrain.
     *
     */
    static uint32_t createTree(const TreePlacementArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);

        auto tileHeight = World::TileManager::getHeight(args.pos);
        setPosition(World::Pos3(args.pos.x + World::kTileSize / 2, args.pos.y + World::kTileSize / 2, tileHeight.landHeight));

        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            // Error message set in checkFreeElementsAndReorganise
            return FAILURE;
        }

        if (!World::TileManager::validCoords(args.pos))
        {
            return FAILURE;
        }

        const auto* treeObj = ObjectManager::get<TreeObject>(args.type);
        const auto quadrantHeight = World::TileManager::getHeight(args.pos + _quadrantCentreOffsets[args.quadrant] - World::Pos2{ 1, 1 });

        auto* elSurface = World::TileManager::get(args.pos).surface();
        if (elSurface == nullptr)
        {
            return FAILURE;
        }

        if (elSurface->water())
        {
            if (elSurface->waterHeight() - 1 > quadrantHeight.landHeight)
            {
                setErrorText(StringIds::cant_build_this_underwater);
                return FAILURE;
            }
        }

        const auto* landObj = ObjectManager::get<LandObject>(elSurface->terrain());
        if (landObj->hasFlags(LandObjectFlags::noTrees))
        {
            setErrorText(StringIds::land_type_not_suitable);
            return FAILURE;
        }

        const auto baseZ = quadrantHeight.landHeight / World::kSmallZStep;
        auto clearanceZ = baseZ + treeObj->initialHeight / World::kSmallZStep;
        if (args.requiresFullClearance)
        {
            clearanceZ = std::numeric_limits<uint8_t>::max();
        }

        World::QuarterTile qt(1 << (args.quadrant ^ (1 << 1)), 0xF);
        if (!World::TileClearance::canConstructAt(args.pos, baseZ, clearanceZ, qt))
        {
            // Error message set in canConstructAt
            return FAILURE;
        }

        if (flags & Flags::apply)
        {
            auto* elTree = World::TileManager::insertElement<World::TreeElement>(args.pos, baseZ, qt.getBaseQuarterOccupied());
            if (elTree == nullptr)
            {
                return FAILURE;
            }
            Ui::Windows::Terraform::setLastPlacedTree(elTree);
            elTree->setRotation(args.rotation);
            elTree->setQuadrant(args.quadrant);
            elTree->setTreeObjectId(args.type);
            elTree->setGrowth(0);
            elTree->setUnk5h(0);
            elTree->setColour(args.colour);
            elTree->setIsDying(false);
            elTree->setSnow(false);
            elTree->setSeason(treeObj->var_3E);
            elTree->setUnk7l(7);
            elTree->setClearZ(treeObj->initialHeight / World::kSmallZStep + elTree->baseZ());
            Scenario::getOptions().madeAnyChanges = 1;
            if (args.buildImmediately)
            {
                elTree->setGrowth(treeObj->growth - 1);
                elTree->setClearZ(treeObj->height / World::kSmallZStep + elTree->baseZ());
                if (elTree->baseZ() - 4 > Scenario::getCurrentSnowLine() && treeObj->hasFlags(TreeObjectFlags::hasSnowVariation))
                {
                    elTree->setSnow(true);
                }
            }
            if (flags & Flags::ghost)
            {
                elTree->setGhost(true);
            }
            else
            {
                TownManager::updateTownInfo(args.pos, 0, 0, treeObj->rating, 0);
            }
            Ui::ViewportManager::invalidate(args.pos, elTree->baseHeight(), elTree->clearHeight(), ZoomLevel::eighth, 56);
        }

        return Economy::getInflationAdjustedCost(treeObj->buildCostFactor, treeObj->costIndex, 12);
    }

    void createTree(registers& regs)
    {
        TreePlacementArgs args(regs);
        regs.ebx = createTree(args, regs.bl);
    }
}
