#include "GameCommands.h"
#include "Map/MapGenerator.h"
#include "Map/SurfaceElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "S5/S5.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x00468EDD
    static uint32_t changeLandMaterial(const World::Pos2& pointA, const World::Pos2& pointB, const uint8_t landType, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction);
        const auto midPoint = (pointA + pointB) / 2 + World::Pos2{ 16, 16 };
        auto height = World::TileManager::getHeight(midPoint);
        GameCommands::setPosition({ midPoint.x, midPoint.y, height.landHeight });

        if ((flags & Flags::apply) == 0)
        {
            return 0;
        }

        const auto tileLoop = World::getClampedRange(pointA, pointB);
        for (const auto& tilePos : tileLoop)
        {
            auto surface = World::TileManager::get(tilePos).surface();
            if (surface == nullptr)
                continue;

            surface->setTerrain(landType);
            if (!surface->isIndustrial())
            {
                surface->setVar6SLR5(0);
            }
            const auto variation = World::MapGenerator::getRandomTerrainVariation(*surface);
            if (variation.has_value())
            {
                if (!surface->isIndustrial())
                {
                    surface->setVariation(*variation);
                }
            }
            auto* landObj = ObjectManager::get<LandObject>(landType);
            if (landObj != nullptr)
            {
                if (!surface->isIndustrial())
                {
                    surface->setVar6SLR5(landObj->var_03 - 1);
                }
            }
            World::TileManager::mapInvalidateTileFull(World::toWorldSpace(tilePos));
            auto& options = S5::getOptions();
            options.madeAnyChanges = 1;
        }
        return 0;
    }

    void changeLandMaterial(registers& regs)
    {
        ChangeLandMaterialArgs args(regs);
        regs.ebx = changeLandMaterial(args.pointA, args.pointB, args.landType, regs.bl);
    }
}
