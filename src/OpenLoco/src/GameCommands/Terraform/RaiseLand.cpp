#include "RaiseLand.h"
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
    // 0x00463702, 0x0046370B
    uint32_t raiseLand(const RaiseLandArgs& args, World::TileClearance::RemovedBuildings& removedBuildings, const uint8_t flags)
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

        const auto tileLoop = getClampedRange(args.pointA, args.pointB);

        // Find out what the lowest baseZ in the selected range is
        auto lowestBaseZ = 255;
        for (const auto& tilePos : tileLoop)
        {
            auto tile = World::TileManager::get(tilePos);
            auto* surface = tile.surface();
            lowestBaseZ = std::min<SmallZ>(lowestBaseZ, surface->baseZ());
        }

        // Now modify only the elements matching this lowest baseZ
        auto totalCost = 0;
        for (const auto& tilePos : tileLoop)
        {
            auto tile = World::TileManager::get(tilePos);
            auto* surface = tile.surface();
            if (surface->baseZ() > lowestBaseZ)
            {
                continue;
            }

            auto targetBaseZ = lowestBaseZ;
            auto slopeFlags = raiseSurfaceCornerFlags(args.corner, surface->slope());
            if (slopeFlags & SurfaceSlope::requiresHeightAdjustment)
            {
                targetBaseZ += kSmallZStep;
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

    void raiseLand(registers& regs)
    {
        // We keep track of removed buildings for each tile visited
        // this prevents accidentally double counting their removal
        // cost if they span across multiple tiles.
        World::TileClearance::RemovedBuildings removedBuildings{};

        const RaiseLandArgs args(regs);
        regs.ebx = raiseLand(args, removedBuildings, regs.bl);
    }
}
