#include "TreeElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TreeObject.h"
#include "Scenario.h"
#include "SceneManager.h"
#include "SurfaceElement.h"
#include "TileManager.h"
#include "ViewportManager.h"

namespace OpenLoco::World
{
    static constexpr std::array<uint8_t, 6> kSeasonToNextSeason = { 1, 4, 3, 0, 5, 0xFFU };

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
            if (treeObj->hasFlags(TreeObjectFlags::hasSnowVariation) && !elTree.hasSnow())
            {
                elTree.setSnow(true);
                Ui::ViewportManager::invalidate(loc, elTree.baseHeight(), elTree.clearHeight(), ZoomLevel::eighth, 56);
            }
        }

        const auto& elSurface = *TileManager::get(loc).surface();
        if (elSurface.isIndustrial())
        {
            // 0x004BD5F0
        }
        else
        {
            // 0x004BD61B
        }
    }
}
