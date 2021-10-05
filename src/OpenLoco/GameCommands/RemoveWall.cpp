#include "../Economy/Expenditures.h"
#include "../Map/TileManager.h"
#include "../S5/S5.h"
#include "../ViewportManager.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{

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
     * This is called when you activate the Build Walls from the construction (first) menu and you move the cursor over the terrain
     *
     * @param pos_x @<ax>
     * @param pos_y @<cx>
     * @param pos_z @<dh>
     * @param rotation @<bh>
     * @param flags @<bl>
     * @return @<ebx> - returns 0 (always successful)
     */
    static uint32_t removeWall(const Map::Pos2& pos, const uint8_t pos_z, const uint8_t rotation, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction); // 0x004C466C

        GameCommands::setPosition(Map::Pos3(pos.x + Map::tile_size / 2, pos.y + Map::tile_size / 2, pos_z * 4)); // 0x004C4674-0x004C4698

        auto tile = Map::TileManager::get(pos); // 0x004C46A1-0x004C46B0
        for (auto& element : tile)
        {
            if ((element.rawData()[0] & 0x3C) != 0x18) // is elementType wall? 0x004C46B6-0x004C46BE
                continue;

            if (element.baseZ() != pos_z) // 0x004C46C0-0x004C46C3
                continue;

            if ((element.rawData()[0] & 0x03) != rotation) // 0x004C46C5-0x004C46CC
                continue;

            if ((flags & Flags::flag_6) == 0 && element.isGhost()) // 0x004C46D9-0x004C46E2
                continue;

            auto wallElement = element.asWall();
            if (wallElement == nullptr)
                continue;

            if ((flags & Flags::apply) == 0) // 0x004C46E4-004C46E7
            {
                return 0;
            }

            Ui::ViewportManager::invalidate(pos, wallElement->baseZ() * 4, wallElement->baseZ() * 4 + 48, ZoomLevel::half); // 0x004C46E9-0x004C4701

            sub_461760(element); // 0x004C4702

            auto& options = S5::getOptions();
            options.madeAnyChanges = 1; // 0x004C4707

            return 0;
        }

        return 0;
    }

    void removeWall(registers& regs)
    {
        const Map::Pos2 pos2 = { regs.ax, regs.cx };
        regs.ebx = removeWall(pos2, regs.dh, regs.dl, regs.bl);
    }
}
