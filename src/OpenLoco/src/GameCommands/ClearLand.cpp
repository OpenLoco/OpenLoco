#include "Economy/Expenditures.h"
#include "GameCommands.h"
#include "Localisation/StringIds.h"
#include "Map/QuarterTile.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "SceneManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    static loco_global<uint32_t, 0x00F0013C> _F0013C;
    static loco_global<uint32_t, 0x00F00140> _F00140;
    static loco_global<uint16_t, 0x00F00144> _F00144;
    static loco_global<uint8_t*, 0x00F0016C> _F0016C; // stack offset to -eventually- flags
    static loco_global<uint16_t, 0x00F00170> _F00170;
    static loco_global<uint32_t, 0x00F0013C> _F003CE; // tile x?
    static loco_global<uint32_t, 0x00F00140> _F003D0; // tile y?
    static loco_global<uint32_t, 0x00F25308> _sub469E07Cost; // cost?

    // 0x004690FC
    static void setTerrainStyleAsCleared(World::Pos2 pos)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004690FC, regs);
    }

    // 0x00469D76
    static uint32_t clearTile(World::Pos2 pos, const uint8_t flags)
    {
        // This shoudn't happen due to using TilePosRangeView
        if (!World::validCoords(pos))
        {
            GameCommands::setErrorText(StringIds::off_edge_of_map);
            return GameCommands::FAILURE;
        }

        _sub469E07Cost = 0;
        if (flags & GameCommands::Flags::apply)
        {
            if (!isEditorMode())
            {
                setTerrainStyleAsCleared(pos);
            }

            auto tileHeight = World::TileManager::getHeight(pos);
            TileManager::removeAllWallsOnTile(pos, tileHeight.landHeight / 4);
        }

        World::QuarterTile qt(0xF, 0);

        // TODO: for 0x00469E07 -- remove this hack when no longer needed
        uint8_t flagStackHack[10] = { 0 };
        flagStackHack[8] = flags;

        // These are part of the parameters for 0x00469E07
        _F003CE = pos.x;
        _F003D0 = pos.y;
        _F0016C = flagStackHack;

        // TODO: implement 0x00469E07 as a real function after canConstructAt is implemented
        auto tileHeight = World::TileManager::getHeight(pos);
        if (TileManager::sub_462908(pos, tileHeight.landHeight / 4, tileHeight.landHeight / 4, qt, (void*)0x00469E07))
            return _sub469E07Cost;
        else
            return GameCommands::FAILURE;
    }

    // 0x00469CCB
    static uint32_t clearLand(const ClearLandArgs& args, const uint8_t flags)
    {
        _F00170 = 0xFFFF;
        _F0013C = args.pointB.x << 16 | args.pointA.x;
        _F00140 = args.pointB.y << 16 | args.pointA.y;
        _F00144 = flags;

        World::TilePosRangeView tileLoop{ { args.pointA }, { args.pointB } };
        uint32_t totalCost = 0;
        for (const auto& tilePos : tileLoop)
        {
            uint32_t tileRes = clearTile(tilePos, flags);
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
