#include "TreeElement.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/TreeObject.h"
#include "Scenario.h"
#include "SceneManager.h"
#include "SurfaceElement.h"
#include "TileClearance.h"
#include "TileManager.h"
#include "ViewportManager.h"
#include "World/IndustryManager.h"

namespace OpenLoco::World
{
    static constexpr std::array<uint8_t, 6> kSeasonToNextSeason = { 1, 4, 3, 0, 5, 0xFFU };

    // 0x004BDA17
    static TileClearance::ClearFuncResult clearFunc(World::TileElement& el)
    {
        if (el.isAiAllocated() || el.isGhost())
        {
            return TileClearance::ClearFuncResult::noCollision;
        }
        //...
    }

    // 0x004BD52B
    bool updateTreeElement(TreeElement& elTree, const World::Pos2 loc)
    {
        if (elTree.unk7l() != 7)
        {
            elTree.setUnk7l(elTree.unk7l() + 1);
            Ui::ViewportManager::invalidate(loc, elTree.baseHeight(), elTree.clearHeight(), ZoomLevel::eighth, 56);
            return true;
        }

        auto* treeObj = ObjectManager::get<TreeObject>(elTree.treeObjectId());
        if (elTree.unk6_80())
        {
            const auto unk = kSeasonToNextSeason[elTree.season()];
            if (unk == 0xFFU)
            {
                Ui::ViewportManager::invalidate(loc, elTree.baseHeight(), elTree.clearHeight(), ZoomLevel::eighth, 56);
                TileManager::removeElement(reinterpret_cast<TileElement&>(elTree));
                return false;
            }
            else
            {
                elTree.setSeason(unk);
                Ui::ViewportManager::invalidate(loc, elTree.baseHeight(), elTree.clearHeight(), ZoomLevel::eighth, 56);
                return true;
            }
        }

        if (!SceneManager::isEditorMode())
        {
            elTree.setUnk5h(elTree.unk5h() + 1);
            if (elTree.unk5h() != 0)
            {
                return;
            }
        }

        const auto isBelowSnowLine = elTree.baseZ() - 4 < Scenario::getCurrentSnowLine();
        if (isBelowSnowLine)
        {
            if (elTree.hasSnow())
            {
                elTree.setSnow(false);
                Ui::ViewportManager::invalidate(loc, elTree.baseHeight(), elTree.clearHeight(), ZoomLevel::eighth, 56);
            }
        }
        else
        {
            if (treeObj->hasFlags(TreeObjectFlags::hasSnowVariation))
            {
                if (!elTree.hasSnow())
                {
                    elTree.setSnow(true);
                    Ui::ViewportManager::invalidate(loc, elTree.baseHeight(), elTree.clearHeight(), ZoomLevel::eighth, 56);
                }
            }
            else
            {
                // kill tree
                if (!SceneManager::isEditorMode())
                {
                    elTree.setUnk6_80(true);
                }
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
                // kill tree
                if (!SceneManager::isEditorMode())
                {
                    elTree.setUnk6_80(true);
                }
                return true;
            }
        }

        if (elTree.season() != enumValue(Scenario::getCurrentSeason()))
        {
            elTree.setSeason((elTree.season() + 1) & 0x3);
            Ui::ViewportManager::invalidate(loc, elTree.baseHeight(), elTree.clearHeight(), ZoomLevel::eighth, 56);
            return true;
        }

        const auto newGrowth = elTree.unk5l() + 1;

        if (newGrowth < treeObj->growth)
        {
            if (SceneManager::isEditorMode())
            {
                return;
            }
            const auto oldClearZ = elTree.clearZ();
            elTree.setClearZ(elTree.baseZ());

            auto length = (treeObj->height - treeObj->initialHeight) * newGrowth;
            const auto divisor = treeObj->growth - 1;
            if (divisor != 0)
            {
                length /= divisor;
            }
            const World::SmallZ newHeight = treeObj->initialHeight + length;
            const auto newClearZ = elTree.baseZ() + newHeight;
            const auto occupiedQuad = 1U << ((elTree.quadrant() + 2) & 0x3);
            const auto qt = World::QuarterTile(occupiedQuad, 0b1111);

            const auto canConstruct = TileClearance::canConstructAt(loc, elTree.baseZ(), newClearZ, qt);
            if (!canConstruct)
            {
                elTree.setClearZ(oldClearZ);
                // kill tree
                if (!SceneManager::isEditorMode())
                {
                    elTree.setUnk6_80(true);
                }
                return true;
            }
            elTree.setClearZ(newClearZ);
            elTree.setUnk5l(newGrowth);
            Ui::ViewportManager::invalidate(loc, elTree.baseHeight(), elTree.clearHeight(), ZoomLevel::eighth, 56);
            return true;
        }

        if (treeObj->var_05 > 34)
        {
            const auto quad = elTree.quadrant();
            const auto topLeft = loc + World::kOffsets[quad] / 2 - World::Pos2{ 16, 16 };
            auto pos = topLeft;
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

                    TileClearance::applyClearAtStandardHeight(checkPos, elTree.baseZ(), elTree.clearZ(), qt, clearFunc);
                    pos.x += 16;
                }

                pos.x = topLeft.x;
                pos.y += 16;
            }
        }
    }
