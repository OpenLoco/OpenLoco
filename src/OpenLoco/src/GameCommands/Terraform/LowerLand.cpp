#include "LowerLand.h"
#include "Audio/Audio.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/StringIds.h"
#include "Map/RoadElement.h"
#include "Map/SurfaceData.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "ScenarioOptions.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::World::SurfaceSlope;

namespace OpenLoco::GameCommands
{
    // 0x004638C6, 0x004638CF
    uint32_t lowerLand(const LowerLandArgs& args, World::TileClearance::RemovedBuildings& removedBuildings, const uint8_t flags)
    {
        if (flags & Flags::apply)
        {
            Scenario::getOptions().madeAnyChanges = 1;

            if (getCommandNestLevel() == 1 && getUpdatingCompanyId() != CompanyId::neutral)
            {
                const auto height = TileManager::getHeight(args.centre).landHeight;
                Audio::playSound(Audio::SoundId::construct, World::Pos3(args.centre.x, args.centre.y, height));
            }
        }

        const auto tileLoop = World::getClampedRange(args.pointA, args.pointB);

        // Find out what the highest baseZ in the selected range is
        auto highestBaseZ = 0;
        for (const auto& tilePos : tileLoop)
        {
            auto tile = TileManager::get(tilePos);
            auto* surface = tile.surface();

            auto baseZ = TileManager::getSurfaceCornerHeight(*surface);
            highestBaseZ = std::max<SmallZ>(highestBaseZ, baseZ);
        }

        // Now modify only the elements matching this highest baseZ
        auto totalCost = 0;
        for (const auto& tilePos : tileLoop)
        {
            auto tile = TileManager::get(tilePos);
            auto* surface = tile.surface();

            auto baseZ = TileManager::getSurfaceCornerHeight(*surface);
            if (baseZ < highestBaseZ)
            {
                continue;
            }

            auto targetBaseZ = surface->baseZ();
            auto slopeFlags = lowerSurfaceCornerFlags(args.corner, surface->slope());
            if (slopeFlags & SurfaceSlope::requiresHeightAdjustment)
            {
                targetBaseZ -= kSmallZStep;
                slopeFlags &= ~SurfaceSlope::requiresHeightAdjustment;
            }

            auto cost = TileManager::adjustSurfaceHeight(World::toWorldSpace(tilePos), targetBaseZ, slopeFlags, removedBuildings, flags);
            if (cost == FAILURE)
            {
                return FAILURE;
            }
            else
            {
                totalCost += cost;
            }
        }

        GameCommands::setExpenditureType(ExpenditureType::Construction);

        auto tileHeight = World::TileManager::getHeight(args.centre);
        GameCommands::setPosition(World::Pos3(args.centre.x, args.centre.y, tileHeight.landHeight));

        return totalCost;
    }

    void lowerLand(registers& regs)
    {
        // We keep track of removed buildings for each tile visited
        // this prevents accidentally double counting their removal
        // cost if they span across multiple tiles.
        World::TileClearance::RemovedBuildings removedBuildings{};

        const LowerLandArgs args(regs);
        regs.ebx = lowerLand(args, removedBuildings, regs.bl);
    }
}
