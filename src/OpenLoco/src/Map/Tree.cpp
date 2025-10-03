#include "Tree.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Terraform/CreateTree.h"
#include "Graphics/Colour.h"
#include "Map/MapSelection.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/TreeObject.h"
#include "Random.h"
#include "Scenario.h"
#include "SurfaceElement.h"
#include "TileManager.h"
#include "Ui/ViewportInteraction.h"
#include <OpenLoco/Math/Trigonometry.hpp>

namespace OpenLoco::World
{
    // 0x004BDF19
    std::optional<uint8_t> getRandomTreeTypeFromSurface(const World::TilePos2& loc, bool unk)
    {
        if (!World::TileManager::validCoords(loc))
        {
            return {};
        }

        auto* surface = World::TileManager::get(loc).surface();
        if (surface == nullptr)
        {
            return {};
        }

        TreeObjectFlags mustNotTreeFlags = TreeObjectFlags::none;
        if (!unk)
        {
            mustNotTreeFlags |= TreeObjectFlags::unk1;
        }

        TreeObjectFlags mustTreeFlags = TreeObjectFlags::none;
        if (surface->baseZ() - 4 > Scenario::getCurrentSnowLine())
        {
            mustTreeFlags |= TreeObjectFlags::hasSnowVariation;
        }
        if (surface->baseZ() >= 68)
        {
            mustTreeFlags |= TreeObjectFlags::highAltitude;
        }
        if (surface->baseZ() <= 48)
        {
            mustTreeFlags |= TreeObjectFlags::lowAltitude;
        }

        auto* landObj = ObjectManager::get<LandObject>(surface->terrain());
        mustNotTreeFlags |= TreeObjectFlags::droughtResistant;
        if (landObj->hasFlags(LandObjectFlags::isDesert))
        {
            mustTreeFlags |= TreeObjectFlags::droughtResistant;
            mustNotTreeFlags &= ~TreeObjectFlags::droughtResistant;
        }

        if (landObj->hasFlags(LandObjectFlags::noTrees))
        {
            return {};
        }
        mustNotTreeFlags |= TreeObjectFlags::requiresWater;
        const uint16_t numSameTypeSurfaces = TileManager::countSurroundingWaterTiles(World::toWorldSpace(loc));
        if (numSameTypeSurfaces >= 8)
        {
            mustNotTreeFlags &= ~TreeObjectFlags::requiresWater;
        }

        std::vector<uint8_t> selectableTrees;
        for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::tree); ++i)
        {
            auto* treeObj = ObjectManager::get<TreeObject>(i);
            if (treeObj == nullptr)
            {
                continue;
            }
            if (treeObj->hasFlags(mustNotTreeFlags))
            {
                continue;
            }

            if ((treeObj->flags & mustTreeFlags) != mustTreeFlags)
            {
                continue;
            }
            selectableTrees.push_back(i);
        }

        if (selectableTrees.empty())
        {
            return {};
        }

        auto& rng = gPrng1();
        const auto randVal = rng.randNext();
        const auto selected = ((randVal & 0xFFFF) * selectableTrees.size()) / 65536;
        return { selectableTrees[selected] };
    }

    bool placeRandomTree(const World::Pos2& pos, std::optional<uint8_t> treeType)
    {
        GameCommands::TreePlacementArgs args;
        args.quadrant = World::getQuadrantFromPos(pos);
        args.pos = World::Pos2(pos.x & 0xFFE0, pos.y & 0xFFE0);
        // Note: this is not the same as the randomDirection above as it is the trees rotation
        args.rotation = gPrng1().randNext(3);
        args.colour = Colour::black;

        // If not set by the caller then a random tree type is selected based on the surface type
        std::optional<uint8_t> randTreeType = treeType;
        if (!randTreeType.has_value())
        {
            randTreeType = getRandomTreeTypeFromSurface(World::toTileSpace(args.pos), false);
            // It is possible that there are no valid tree types for the surface
            if (!randTreeType.has_value())
            {
                return false;
            }
        }
        args.type = *randTreeType;
        args.buildImmediately = true;
        args.requiresFullClearance = true;

        // First query if we can place a tree at this location; skip if we can't.
        auto queryRes = doCommand(args, 0);
        if (queryRes == GameCommands::FAILURE)
        {
            return false;
        }

        // Actually place the tree
        doCommand(args, GameCommands::Flags::apply);
        return true;
    }

    // 0x004BDC67 (when treeType is nullopt) & 0x004BDDC6 (when treeType is set)
    bool placeTreeCluster(const World::TilePos2& centreLoc, const uint16_t range, const uint16_t density, const std::optional<uint8_t> treeType)
    {
        const auto numPlacements = (range * range * density) / 8192;
        uint16_t numErrors = 0;
        for (auto i = 0; i < numPlacements; ++i)
        {
            // Choose a random offset in a circle
            auto& rng = gPrng1();
            auto randomMagnitude = rng.randNext(std::numeric_limits<uint16_t>::max()) * range / 65536;
            auto randomDirection = rng.randNext(Math::Trigonometry::kDirectionPrecisionHigh - 1);
            World::Pos2 randomOffset(
                Math::Trigonometry::integerSinePrecisionHigh(randomDirection, randomMagnitude),
                Math::Trigonometry::integerCosinePrecisionHigh(randomDirection, randomMagnitude));

            if (!placeRandomTree(randomOffset + World::toWorldSpace(centreLoc), treeType))
            {
                numErrors++;
            }
        }

        // Have we placed any trees?
        return (numErrors < numPlacements);
    }
}
