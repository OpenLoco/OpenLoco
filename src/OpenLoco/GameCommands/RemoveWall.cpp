#include "../Economy/Expenditures.h"
#include "../Map/TileManager.h"
#include "../S5/S5.h"
#include "../ViewportManager.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x00461760 - this should be more generic function and not just related to remove wall function
    static void sub_461760(const TileElement& tileElement)
    {
        registers regs = {};
        regs.esi = X86Pointer(&tileElement);
        call(0x00461760, regs);
    }

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
        GameCommands::setExpenditureType(ExpenditureType::Construction); // 0x004C466C

        GameCommands::setPosition(Map::Pos3(args.pos.x + Map::tile_size / 2, args.pos.y + Map::tile_size / 2, args.pos.z)); // 0x004C4674-0x004C4698

        auto tile = Map::TileManager::get(args.pos); // 0x004C46A1-0x004C46B0
        for (auto& tileElement : tile)
        {
            auto wallElement = tileElement.asWall();
            if (wallElement == nullptr)
                continue;

            if (wallElement->baseZ() != args.pos.z / 4) // 0x004C46C0-0x004C46C3
                continue;

            if (wallElement->rotation() != args.rotation) // 0x004C46C5-0x004C46CC
                continue;

            if ((flags & Flags::flag_6) == 0 && wallElement->isGhost()) // 0x004C46D9-0x004C46E2
                continue;

            if ((flags & Flags::apply) == 0) // 0x004C46E4-004C46E7
            {
                return 0;
            }

            Ui::ViewportManager::invalidate(args.pos, wallElement->baseZ() * 4, wallElement->baseZ() * 4 + 48, ZoomLevel::half); // 0x004C46E9-0x004C4701

            sub_461760(tileElement); // 0x004C4702

            auto& options = S5::getOptions();
            options.madeAnyChanges = 1; // 0x004C4707

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
