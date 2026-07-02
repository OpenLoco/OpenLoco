#include "GameCommands/Terraform/RemoveTree.h"
#include "Audio/Audio.h"
#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "Map/TileElement.h"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TreeObject.h"
#include "Random.h"
#include "Scenario/ScenarioOptions.h"
#include "ViewportManager.h"
#include "World/TownManager.h"

namespace OpenLoco::GameCommands
{
    /**
     * 0x004BB392
     * Remove tree
     *
     * This is called when you activate the Plant Trees from the construction (first) menu and you move the cursor over the terrain.
     *
     * @param pos_x @<ax>
     * @param pos_y @<cx>
     * @param pos_z @<dl> * World::kSmallZStep
     * @param type @<dh>
     * @param elementType @<bh>
     * @param flags @<bl>
     * @return @<ebx> - returns the remove cost if successful; otherwise GameCommands::kFailure (in the assembly code we never get into failure path)
     */
    static uint32_t removeTree(const TreeRemovalArgs& args, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction);

        auto tileHeight = World::TileManager::getHeight(args.pos);
        GameCommands::setPosition(World::Pos3(args.pos.x + World::kTileSize / 2, args.pos.y + World::kTileSize / 2, tileHeight.landHeight));

        auto tile = World::TileManager::get(args.pos);
        for (auto& element : tile)
        {
            auto* treeElement = element.as<World::TreeElement>();
            if (treeElement == nullptr)
            {
                continue;
            }

            if (treeElement->baseHeight() != args.pos.z)
            {
                continue;
            }

            if (treeElement->quadrant() != args.quadrant)
            {
                continue;
            }

            if (treeElement->rotation() != args.rotation)
            {
                continue;
            }

            if (treeElement->treeObjectId() != args.type)
            {
                continue;
            }

            auto treeObj = ObjectManager::get<TreeObject>(treeElement->treeObjectId());
            currency32_t removalCost = Economy::getInflationAdjustedCost(treeObj->clearCostFactor, treeObj->costIndex, 12);

            if (flags & Flags::apply)
            {
                World::TileManager::removeTree(element, flags, args.pos);
            }

            auto& options = Scenario::getOptions();
            options.madeAnyChanges = 1;

            return removalCost;
        }

        return kFailure;
    }

    void removeTree(registers& regs, const uint8_t flags)
    {
        TreeRemovalArgs args(regs);
        regs.ebx = removeTree(args, flags);
    }
}
