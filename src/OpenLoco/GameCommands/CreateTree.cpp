#include "../Economy/Economy.h"
#include "../Economy/Expenditures.h"
#include "../Localisation/StringIds.h"
#include "../Map/QuarterTile.h"
#include "../Map/TileManager.h"
#include "../Objects/LandObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/TreeObject.h"
#include "../OpenLoco.h"
#include "../S5/S5.h"
#include "../Scenario/Scenario.h"
#include "../TownManager.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    constexpr std::array<Map::Pos2, 4> _quadrantCentreOffsets = {
        Map::Pos2{ 7, 7 },
        Map::Pos2{ 7, 23 },
        Map::Pos2{ 23, 23 },
        Map::Pos2{ 23, 7 },
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

        auto tileHeight = Map::TileManager::getHeight(args.pos);
        setPosition(Map::Pos3(args.pos.x + Map::tile_size / 2, args.pos.y + Map::tile_size / 2, tileHeight.landHeight));

        if (!Map::TileManager::checkFreeElementsAndReorganise())
        {
            // Error message set in checkFreeElementsAndReorganise
            return FAILURE;
        }

        if (!Map::validCoords(args.pos))
        {
            return FAILURE;
        }

        const auto* treeObj = ObjectManager::get<TreeObject>(args.type);
        const auto quadrantHeight = Map::TileManager::getHeight(args.pos + _quadrantCentreOffsets[args.quadrant] - Map::Pos2{ 1, 1 });

        auto* elSurface = Map::TileManager::get(args.pos).surface();
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
        if (landObj->flags & LandObjectFlags::noTrees)
        {
            setErrorText(StringIds::land_type_not_suitable);
            return FAILURE;
        }

        const auto baseZ = quadrantHeight.landHeight / Map::kSmallZStep;
        auto clearanceZ = baseZ + treeObj->var_02;
        if (args.requiresFullClearance)
        {
            clearanceZ = std::numeric_limits<uint8_t>::max();
        }

        Map::QuarterTile qt(1 << (args.quadrant ^ (1 << 1)), 0xF);
        if (!Map::TileManager::canConstructAt(args.pos, baseZ, clearanceZ, qt))
        {
            // Error message set in canConstructAt
            return FAILURE;
        }

        if (flags & Flags::apply)
        {
            auto* elTree = Map::TileManager::insertElement<Map::TreeElement>(args.pos, baseZ, qt.getBaseQuarterOccupied());
            if (elTree == nullptr)
            {
                return FAILURE;
            }
            Ui::Windows::Terraform::setLastPlacedTree(elTree);
            elTree->setRotation(args.rotation);
            elTree->setQuadrant(args.quadrant);
            elTree->setTreeObjectId(args.type);
            elTree->setUnk5l(0);
            elTree->setUnk5h(0);
            elTree->setColour(args.colour);
            elTree->setUnk6_80(false);
            elTree->setSnow(false);
            elTree->setSeason(treeObj->var_3E);
            elTree->setUnk7l(7);
            elTree->setClearZ(treeObj->var_02 / Map::kSmallZStep + elTree->baseZ());
            S5::getOptions().madeAnyChanges = 1;
            if (args.buildImmediately)
            {
                elTree->setUnk5l(treeObj->growth - 1);
                elTree->setClearZ(treeObj->height / Map::kSmallZStep + elTree->baseZ());
                if (elTree->baseZ() - 4 > Scenario::getCurrentSnowLine() && (treeObj->flags & TreeObjectFlags::hasSnowVariation))
                {
                    elTree->setSnow(true);
                }
            }
            if (flags & Flags::flag_6)
            {
                elTree->setGhost(true);
            }
            else
            {
                TownManager::sub_497DC1(args.pos, 0, 0, treeObj->rating, 0);
            }
            Ui::ViewportManager::invalidate(args.pos, elTree->baseHeight(), elTree->clearHeight(), ZoomLevel::eighth, 56);
        }

        return Economy::getInflationAdjustedCost(treeObj->build_cost_factor, treeObj->cost_index, 12);
    }

    void createTree(registers& regs)
    {
        TreePlacementArgs args(regs);
        regs.ebx = createTree(args, regs.bl);
    }
}
