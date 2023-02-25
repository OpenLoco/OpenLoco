#include "Economy/Expenditures.h"
#include "GameCommands.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    static loco_global<uint32_t, 0x00F0013C> _F0013C;
    static loco_global<uint32_t, 0x00F00140> _F00140;
    static loco_global<uint16_t, 0x00F00144> _F00144;
    static loco_global<uint16_t, 0x00F00170> _F00170;

    // 0x00469D76
    static uint32_t sub_469D76(World::Pos2 pos, const uint8_t flags)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.bl = flags;
        call(0x00469D76, regs);
        return regs.ebx;
    }

    // 0x00469CCB
    static uint32_t clearLand(const ClearLandArgs& args, const uint8_t flags)
    {
        _F00170 = 0xFFFF;
        _F0013C = args.pointB.x << 16 | args.pointA.x;
        _F00140 = args.pointB.y << 16 | args.pointA.y;
        _F00144 = flags;

        World::TilePosRangeView tileLoop{ { args.pointA }, { args.pointB } };
        uint32_t selectionRes = 0;
        for (const auto& tilePos : tileLoop)
        {
            uint32_t tileRes = sub_469D76(tilePos, flags);
            if (tileRes == GameCommands::FAILURE)
                return GameCommands::FAILURE;
            else
                selectionRes += tileRes;
        }

        GameCommands::setExpenditureType(ExpenditureType::Construction);

        auto tileHeight = World::TileManager::getHeight(args.centre);
        GameCommands::setPosition(World::Pos3(args.centre.x, args.centre.y, tileHeight.landHeight));

        return selectionRes;
    }

    void clearLand(registers& regs)
    {
        const ClearLandArgs args(regs);
        regs.ebx = clearLand(args, regs.bl);
    }
}
