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
            if (elSurface->water() * 8 - 1 > quadrantHeight.landHeight)
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

        const auto baseZ = quadrantHeight.landHeight / 4;
        auto clearanceZ = baseZ + treeObj->var_02;
        if (args.requiresFullClearance)
        {
            clearanceZ = 255;
        }

        Map::QuarterTile qt{ 1 << (args.quadrant ^ (1 << 1)), 0xF };
        if (!Map::TileManager::canConstructAt(args.pos, baseZ, clearanceZ, qt))
        {
            // Error message set in canConstructAt
            return FAILURE;
        }

        if (flags & Flags::apply)
        {
            auto* elTree = Map::TileManager::insertElement<Map::TreeElement>(args.pos, baseZ, qt.getBaseQuarterOccupied());
            Ui::Windows::Terraform::setLastPlacedTree(elTree);
            elTree->setRotation(args.rotation);
            elTree->setQuadrant(args.quadrant);
            elTree->setTreeObjectId(args.type);
            // 0x004BB2B7
        }

        return Economy::getInflationAdjustedCost(treeObj->build_cost_factor, treeObj->cost_index, 12);
    }

    void createTree(registers& regs)
    {
        TreePlacementArgs args(regs);
        regs.ebx = createTree(args, regs.bl);
    }
}
