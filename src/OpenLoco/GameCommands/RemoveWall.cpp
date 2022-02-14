#include "../Economy/Expenditures.h"
#include "../Map/TileManager.h"
#include "../S5/S5.h"
#include "../ViewportManager.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco::GameCommands
{
    /**
     * 0x004C466C
     * Remove wall
     *
     * This is called when you activate the Build Walls from the construction (first) menu and you move the cursor over the terrain.
     * Removing the ghost walls (during the drawing when you move the cursor) are handled by this function.
     *
     * @param args.pos.x @<ax>
     * @param args.pos.y @<cx>
     * @param args.pos.z @<dh> * 4 (multiplied by 4 in WallRemovalArgs initialiser)
     * @param args.rotation @<bh>
     * @param flags @<bl>
     * @return @<ebx> - returns 0 (always successful)
     */
    static uint32_t removeWall(const WallRemovalArgs& args, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction);

        GameCommands::setPosition(Map::Pos3(args.pos.x + Map::tile_size / 2, args.pos.y + Map::tile_size / 2, args.pos.z));

        auto tile = Map::TileManager::get(args.pos);
        for (auto& tileElement : tile)
        {
            auto* wallElement = tileElement.as<WallElement>();
            if (wallElement == nullptr)
                continue;

            if (wallElement->baseZ() != args.pos.z / 4)
                continue;

            if (wallElement->rotation() != args.rotation)
                continue;

            if ((flags & Flags::flag_6) == 0 && wallElement->isGhost())
                continue;

            if ((flags & Flags::apply) == 0)
            {
                return 0;
            }

            Ui::ViewportManager::invalidate(args.pos, wallElement->baseZ() * 4, wallElement->baseZ() * 4 + 48, ZoomLevel::half);

            TileManager::removeElement(tileElement);

            auto& options = S5::getOptions();
            options.madeAnyChanges = 1;

            return 0;
        }

        return 0;
    }

    void removeWall(registers& regs)
    {
        const WallRemovalArgs args(regs);
        regs.ebx = removeWall(args, regs.bl);
    }
}
