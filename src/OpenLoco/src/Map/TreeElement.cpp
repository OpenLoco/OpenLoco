#include "TreeElement.h"
#include "GameCommands/Terraform/CreateTree.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/TreeObject.h"
#include "Random.h"
#include "RoadElement.h"
#include "Scenario.h"
#include "SceneManager.h"
#include "SurfaceElement.h"
#include "TileClearance.h"
#include "TileManager.h"
#include "TrackElement.h"
#include "Tree.h"
#include "ViewportManager.h"
#include "World/IndustryManager.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco::World
{
    static constexpr std::array<uint8_t, 6> kSeasonDeathSequence = { 1, 4, 3, 0, 5, 0xFFU };

    // TODO: Deduplicate copied from PaintTree
    constexpr std::array<World::Pos2, 4> kTreeQuadrantOffset = {
        World::Pos2{ 7, 7 },
        World::Pos2{ 7, 23 },
        World::Pos2{ 23, 23 },
        World::Pos2{ 23, 7 },
    };
    // 0x004BDA17
    static TileClearance::ClearFuncResult clearFunction(World::TileElement& el, bool& noTrees)
    {
        if (el.isAiAllocated() || el.isGhost())
        {
            return TileClearance::ClearFuncResult::noCollision;
        }
        switch (el.type())
        {
            case ElementType::tree:
            case ElementType::building:
            case ElementType::industry:
                break;

            case ElementType::road:
            {
                auto* elRoad = el.as<RoadElement>();
                if (elRoad != nullptr)
                {
                    if (elRoad->hasBridge())
                    {
                        noTrees = true;
                    }
                }
                break;
            }
            case ElementType::track:
            {
                auto* elTrack = el.as<TrackElement>();
                if (elTrack != nullptr)
                {
                    if (elTrack->hasBridge())
                    {
                        noTrees = true;
                    }
                }
                break;
            }

            case ElementType::signal:
            case ElementType::station:
            case ElementType::surface:
            case ElementType::wall:
                noTrees = true;
                break;
        }

        return TileClearance::ClearFuncResult::noCollision;
    }

    // 0x004BD64A
    static bool hasObstructionsTooNear(World::Pos2 loc, uint8_t quadrant, uint8_t baseZ, uint8_t clearZ)
    {
        const auto topLeft = loc + World::kOffsets[quadrant] / 2 - World::Pos2{ 16, 16 };
        auto pos = topLeft;
        bool noTrees = false;
        for (auto i = 0; i < 3; ++i)
        {
            for (auto j = 0; j < 3; ++j)
            {
                auto checkPos = pos;
                QuarterTile qt(0, 0);
                if (pos.x & 0x1F)
                {
                    checkPos.x &= ~0x1F;
                    qt = World::QuarterTile(1U << 1, 0b1111);
                    if (pos.y & 0x1F)
                    {
                        checkPos.y &= ~0x1F;
                        qt = World::QuarterTile(1U << 0, 0b1111);
                    }
                }
                else
                {
                    qt = World::QuarterTile(1U << 2, 0b1111);
                    if (pos.y & 0x1F)
                    {
                        checkPos.y &= ~0x1F;
                        qt = World::QuarterTile(1U << 3, 0b1111);
                    }
                }

                auto clearFunc = [&noTrees](World::TileElement& el) -> TileClearance::ClearFuncResult {
                    return clearFunction(el, noTrees);
                };

                TileClearance::applyClearAtStandardHeight(checkPos, baseZ, clearZ, qt, clearFunc);
                pos.x += 16;
            }

            pos.x = topLeft.x;
            pos.y += 16;
        }
        return noTrees;
    }

    static void killTree(TreeElement& elTree)
    {
        if (!SceneManager::isEditorMode())
        {
            elTree.setIsDying(true);
        }
    }

    static void invalidateTree(TreeElement& elTree, const World::Pos2 loc)
    {
        Ui::ViewportManager::invalidate(loc, elTree.baseHeight(), elTree.clearHeight(), ZoomLevel::eighth, 56);
    }

    // 0x004BD52B
    bool updateTreeElement(TreeElement& elTree, const World::Pos2 loc)
    {
        if (elTree.unk7l() != 7)
        {
            elTree.setUnk7l(elTree.unk7l() + 1);
            invalidateTree(elTree, loc);
            return true;
        }

        auto* treeObj = ObjectManager::get<TreeObject>(elTree.treeObjectId());
        if (elTree.isDying())
        {
            const auto unk = kSeasonDeathSequence[elTree.season()];
            if (unk == 0xFFU)
            {
                invalidateTree(elTree, loc);
                TileManager::removeElement(reinterpret_cast<TileElement&>(elTree));
                return false;
            }
            else
            {
                elTree.setSeason(unk);
                elTree.setUnk7l(0);
                invalidateTree(elTree, loc);
                return true;
            }
        }

        if (!SceneManager::isEditorMode())
        {
            elTree.setUnk5h(elTree.unk5h() + 1);
            if (elTree.unk5h() != 0)
            {
                return true;
            }
        }

        const auto isBelowSnowLine = elTree.baseZ() - 4 <= Scenario::getCurrentSnowLine();
        if (isBelowSnowLine)
        {
            if (elTree.hasSnow())
            {
                elTree.setSnow(false);
                invalidateTree(elTree, loc);
            }
        }
        else
        {
            if (treeObj->hasFlags(TreeObjectFlags::hasSnowVariation))
            {
                if (!elTree.hasSnow())
                {
                    elTree.setSnow(true);
                    invalidateTree(elTree, loc);
                }
            }
            else
            {
                killTree(elTree);
                return true;
            }
        }

        const auto& elSurface = *TileManager::get(loc).surface();
        if (elSurface.isIndustrial())
        {
            auto* industry = IndustryManager::get(elSurface.industryId());
            auto* industryObj = ObjectManager::get<IndustryObject>(industry->objectId);
            if (industryObj->hasFlags(IndustryObjectFlags::killsTrees))
            {
                killTree(elTree);
                return true;
            }
        }

        if (elTree.season() != enumValue(Scenario::getCurrentSeason()))
        {
            elTree.setSeason((elTree.season() + 1) & 0x3);
            elTree.setUnk7l(0);
            invalidateTree(elTree, loc);
            return true;
        }

        const auto newGrowth = elTree.growth() + 1;

        if (newGrowth < treeObj->growth)
        {
            if (SceneManager::isEditorMode())
            {
                return true;
            }
            const auto oldClearZ = elTree.clearZ();
            elTree.setClearZ(elTree.baseZ());

            auto length = (treeObj->height - treeObj->initialHeight) * (newGrowth + 1);
            const auto divisor = treeObj->growth - 1;
            if (divisor != 0)
            {
                length /= divisor;
            }
            const World::SmallZ newHeight = (treeObj->initialHeight + length) / kSmallZStep;
            const auto newClearZ = elTree.baseZ() + newHeight;
            const auto occupiedQuad = 1U << ((elTree.quadrant() + 2) & 0x3);
            const auto qt = World::QuarterTile(occupiedQuad, 0b1111);

            const auto canConstruct = TileClearance::canConstructAt(loc, elTree.baseZ(), newClearZ, qt);
            if (!canConstruct)
            {
                elTree.setClearZ(oldClearZ);
                killTree(elTree);
                return true;
            }
            elTree.setClearZ(newClearZ);
            elTree.setGrowth(newGrowth);
            elTree.setUnk5h(0);
            invalidateTree(elTree, loc);
            return true;
        }

        if (treeObj->var_05 > 34)
        {
            bool hasObstruction = hasObstructionsTooNear(loc, elTree.quadrant(), elTree.baseZ(), elTree.clearZ());
            if (hasObstruction)
            {
                killTree(elTree);
                return true;
            }
        }
        if (SceneManager::isEditorMode())
        {
            return true;
        }

        auto& prng = gPrng1();
        const auto rand = prng.randNext();
        const auto randDecisions = rand & 0x3F;
        if (randDecisions < 52)
        {
            return true;
        }
        else if (randDecisions < 58)
        {
            World::Pos2 randOffset{};
            const auto rand2 = prng.randNext();
            const auto offsetDistance = randDecisions == 57 ? 31 : 7;
            randOffset.x = ((rand2 >> 16) & (offsetDistance * kTileSize)) - (offsetDistance / 2 * kTileSize);
            randOffset.y = ((rand2 & 0xFFFFU) & (offsetDistance * kTileSize)) - (offsetDistance / 2 * kTileSize);

            if (Scenario::getCurrentSeason() == Scenario::Season::winter)
            {
                return true;
            }

            const auto newTreePos = loc + randOffset;
            if (!TileManager::validCoords(newTreePos))
            {
                return true;
            }

            auto newTreeObjId = elTree.treeObjectId();
            if (!(rand & 0x0F00'0000))
            {
                const auto randTreeObjId = getRandomTreeTypeFromSurface(World::toTileSpace(newTreePos), true);
                if (!randTreeObjId.has_value())
                {
                    return true;
                }
                newTreeObjId = randTreeObjId.value();
            }
            auto* newTreeObj = ObjectManager::get<TreeObject>(newTreeObjId);
            const auto newQuadrant = (rand >> 5) & 0x3;

            if (newTreeObj->var_05 > 34)
            {
                const auto newTreeQuadPos = newTreePos + kTreeQuadrantOffset[newQuadrant] - World::Pos2{ 1, 1 };
                const auto heights = TileManager::getHeight(newTreeQuadPos);
                const auto baseZ = heights.landHeight / World::kSmallZStep;
                const auto clearZ = newTreeObj->height / World::kSmallZStep + baseZ;
                bool hasObstruction = hasObstructionsTooNear(newTreePos, newQuadrant, baseZ, clearZ);
                if (hasObstruction)
                {
                    return true;
                }
            }
            GameCommands::TreePlacementArgs args;
            args.pos = newTreePos;
            args.quadrant = newQuadrant;
            args.rotation = (rand >> 16) & 0x3;
            const auto randNumRotations = (rand >> 19) & 0x1F;
            auto randColoursRot = std::rotr(newTreeObj->colours, randNumRotations);
            const auto randColour = Numerics::bitScanForward(randColoursRot);
            args.colour = randColour == -1 ? Colour::black : static_cast<Colour>((randColour + randNumRotations) & 0x1F);
            args.buildImmediately = false;
            args.requiresFullClearance = true;
            args.type = newTreeObjId;

            GameCommands::doCommand(args, GameCommands::Flags::apply);
            return false;
        }
        else
        {
            if (Scenario::getCurrentSeason() == Scenario::Season::autumn || Scenario::getCurrentSeason() == Scenario::Season::winter)
            {
                killTree(elTree);
                return true;
            }
            return true;
        }
    }
}
