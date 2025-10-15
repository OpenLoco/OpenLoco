#include "RaiseWater.h"
#include "Audio/Audio.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/StringIds.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Objects/ObjectManager.h"
#include "ScenarioOptions.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    // 0x004C4F19
    static uint32_t raiseWater(const RaiseWaterArgs& args, const uint8_t flags)
    {
        // We keep track of removed buildings for each tile visited
        // this prevents accidentally double counting their removal
        // cost if they span across multiple tiles.
        World::TileClearance::RemovedBuildings removedBuildings{};
        auto totalCost = 0;

        if (flags & Flags::apply)
        {
            Scenario::getOptions().madeAnyChanges = 1;
        }

        const auto tileLoop = getClampedRange(args.pointA, args.pointB);

        // Find out what the lowest baseZ in the selected range is
        auto lowestBaseZ = 255;
        for (const auto& tilePos : tileLoop)
        {
            auto tile = World::TileManager::get(tilePos);
            auto* surface = tile.surface();

            auto baseZ = surface->baseZ();
            if (surface->water())
            {
                baseZ = surface->waterHeight();
            }

            lowestBaseZ = std::min<SmallZ>(lowestBaseZ, baseZ);
        }

        // Now modify only the elements matching this lowest baseZ
        for (const auto& tilePos : tileLoop)
        {
            auto tile = World::TileManager::get(tilePos);
            auto* surface = tile.surface();
            if (surface->baseZ() > lowestBaseZ)
            {
                continue;
            }

            auto waterHeight = surface->water() * kMicroToSmallZStep;
            if (waterHeight == 0)
            {
                waterHeight = surface->baseZ() + kSmallZStep;
            }
            else
            {
                if (waterHeight > lowestBaseZ)
                {
                    continue;
                }
                else
                {
                    waterHeight += kSmallZStep;
                }
            }

            auto cost = TileManager::adjustWaterHeight(World::toWorldSpace(tilePos), waterHeight, removedBuildings, flags);
            if (cost == FAILURE)
            {
                return FAILURE;
            }
            else
            {
                totalCost += cost;
            }
        }

        if ((flags & Flags::apply) && totalCost > 0)
        {
            auto centre = (args.pointA + args.pointB) / 2;
            auto tileHeight = World::TileManager::getHeight(centre);
            Audio::playSound(Audio::SoundId::water, World::Pos3(centre.x, centre.y, tileHeight.waterHeight));
        }

        return totalCost;
    }

    void raiseWater(registers& regs)
    {
        const RaiseWaterArgs args(regs);
        regs.ebx = raiseWater(args, regs.bl);
    }
}
