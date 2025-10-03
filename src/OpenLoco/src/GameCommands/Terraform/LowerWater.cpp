#include "LowerWater.h"
#include "Audio/Audio.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/StringIds.h"
#include "Map/SurfaceElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Objects/ObjectManager.h"
#include "ScenarioOptions.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    // 0x004C5126
    static uint32_t lowerWater(const LowerWaterArgs& args, const uint8_t flags)
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

        // Find out what the highest water height in the selected range is
        auto highestWaterHeight = 0;
        for (const auto& tilePos : tileLoop)
        {
            auto tile = World::TileManager::get(tilePos);
            auto* surface = tile.surface();
            if (surface->water())
            {
                auto waterHeight = surface->water() * kMicroToSmallZStep;
                highestWaterHeight = std::max<SmallZ>(waterHeight, highestWaterHeight);
            }
        }

        if (highestWaterHeight > 0)
        {
            // Now modify only the elements matching this highest water height
            for (const auto& tilePos : tileLoop)
            {
                auto tile = World::TileManager::get(tilePos);
                auto* surface = tile.surface();
                auto waterHeight = surface->water() * kMicroToSmallZStep;
                if (waterHeight < highestWaterHeight)
                {
                    continue;
                }

                waterHeight -= kSmallZStep;
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
        }

        if ((flags & Flags::apply) && totalCost > 0)
        {
            auto centre = (args.pointA + args.pointB) / 2;
            auto tileHeight = World::TileManager::getHeight(centre);
            GameCommands::setPosition(World::Pos3(centre.x + World::kTileSize / 2, centre.y + World::kTileSize / 2, tileHeight.waterHeight));
            Audio::playSound(Audio::SoundId::water, World::Pos3(centre.x, centre.y, tileHeight.waterHeight));
        }

        return totalCost;
    }

    void lowerWater(registers& regs)
    {
        const LowerWaterArgs args(regs);
        regs.ebx = lowerWater(args, regs.bl);
    }
}
