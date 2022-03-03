#include "../Economy/Economy.h"
#include "../Economy/Expenditures.h"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/TreeObject.h"
#include "../OpenLoco.h"
#include "../S5/S5.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x004BB432
    // TODO: Move to somewhere else multiple functions call this one
    static void removeTree(Map::TreeElement& element, const uint8_t flags, const Map::Pos2& pos)
    {
        registers regs;
        regs.bl = flags;
        regs.esi = X86Pointer(&element);
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004BB432, regs);
    }

    /**
     * 0x004BB392
     * Remove tree
     *
     * This is called when you activate the Plant Trees from the construction (first) menu and you move the cursor over the terrain.
     *
     * @param pos_x @<ax>
     * @param pos_y @<cx>
     * @param pos_z @<dl> * 4
     * @param type @<dh>
     * @param elementType @<bh>
     * @param flags @<bl>
     * @return @<ebx> - returns the remove cost if successful; otherwise GameCommands::FAILURE (in the assembly code we never get into failure path)
     */
    static uint32_t removeTree(const Map::Pos3& pos, const uint8_t type, const uint8_t elementType, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction);

        auto tileHeight = Map::TileManager::getHeight(pos);
        GameCommands::setPosition(Map::Pos3(pos.x + Map::tile_size / 2, pos.y + Map::tile_size / 2, tileHeight.landHeight));

        auto tile = Map::TileManager::get(pos);
        for (auto& element : tile)
        {
            // TODO: refactor! Figure out what info it actually needs.
            if (element.rawData()[0] != elementType)
                continue;

            if (element.baseZ() * 4 != pos.z)
                continue;

            auto* treeElement = element.as<Map::TreeElement>();
            if (treeElement == nullptr)
                continue;

            if (treeElement->treeObjectId() != type)
                continue;

            auto treeObj = ObjectManager::get<TreeObject>(treeElement->treeObjectId());
            currency32_t removalCost = Economy::getInflationAdjustedCost(treeObj->clear_cost_factor, treeObj->cost_index, 12);

            if (flags & Flags::apply)
                removeTree(*treeElement, flags, pos);

            auto& options = S5::getOptions();
            options.madeAnyChanges = 1;

            return removalCost;
        }

        return FAILURE;
    }

    void removeTree(registers& regs)
    {
        TreeRemovalArgs args(regs);
        regs.ebx = removeTree(args.pos, args.type, args.elementType, regs.bl);
    }
}
