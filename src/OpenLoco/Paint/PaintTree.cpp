#include "PaintTree.h"
#include "../Config.h"
#include "../Graphics/Colour.h"
#include "../Map/Tile.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/TreeObject.h"
#include "../Ui.h"
#include "Paint.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    constexpr std::array<uint8_t, 6> _50076A = { 3, 0, 1, 2, 1, 4 };
    constexpr std::array<bool, 5> _500770 = { true, true, false, false, true };
    constexpr std::array<Map::Pos2, 4> kTreeQuadrantOffset = {
        Map::Pos2{ 7, 7 },
        Map::Pos2{ 7, 23 },
        Map::Pos2{ 23, 23 },
        Map::Pos2{ 23, 7 },
    };

    // 0x004BAEDA
    void paintTree(PaintSession& session, const Map::TreeElement& elTree)
    {
        session.setItemType(InteractionItem::tree);

        const auto* treeObj = ObjectManager::get<TreeObject>(elTree.treeObjectId());
        const uint8_t viewableRotation = (session.getRotation() + elTree.rotation()) & 0x3;
        const uint32_t treeFrameNum = (viewableRotation % treeObj->num_rotations) + elTree.unk5l() * treeObj->num_rotations;

        uint8_t season = elTree.season();

        const uint8_t altSeason = elTree.hasSnow() ? 1 : 0;
        bool hasImage2 = false;
        uint32_t imageId2 = 0;
        if (elTree.unk7l() != 7)
        {
            hasImage2 = true;

            uint32_t edx = (elTree.unk7l()) + 1;
            season = _50076A[season];

            auto image2Season = elTree.season();

            if (!_500770[season])
            {
                image2Season = season;
                season = elTree.season();
                edx = (~edx) & 0b111;
            }
            // Unlikely to do anything as no remap flag set
            edx = edx << 26;
            imageId2 = edx | (treeFrameNum + treeObj->sprites[altSeason][image2Season]);
        }

        const auto seasonBaseImageId = treeObj->sprites[altSeason][season];

        std::optional<uint32_t> shadowImageId = std::nullopt;
        if (treeObj->flags & TreeObjectFlags::hasShadow)
        {
            shadowImageId = Gfx::recolourTranslucent(treeObj->shadowImageOffset + treeFrameNum + seasonBaseImageId, ExtColour::unk32);
        }

        const uint8_t quadrant = (elTree.quadrant() + session.getRotation()) % 4;
        const auto imageOffset = Map::Pos3(kTreeQuadrantOffset[quadrant].x, kTreeQuadrantOffset[quadrant].y, elTree.baseZ() * 4);

        const int16_t boundBoxSizeZ = std::min(elTree.clearZ() - elTree.baseZ(), 32) * 4 - 3;

        uint32_t imageId1 = treeFrameNum + seasonBaseImageId;

        if (treeObj->colours != 0)
        {
            // No vanilla object has this property set
            const auto colour = static_cast<Colour>(elTree.colour());
            imageId2 = Gfx::recolour(imageId2, colour);
            imageId1 = Gfx::recolour(imageId1, colour);
        }

        if (elTree.isGhost())
        {
            session.setItemType(InteractionItem::noInteraction);
            imageId2 = Gfx::applyGhostToImage(imageId2);
            imageId1 = Gfx::applyGhostToImage(imageId1);
        }

        if (shadowImageId)
        {
            if (session.getContext()->zoom_level <= 1)
            {
                session.addToPlotListAsParent(*shadowImageId, imageOffset, { 18, 18, 1 }, imageOffset);
            }
        }

        session.addToPlotListAsParent(imageId1, imageOffset, imageOffset + Map::Pos3(0, 0, 2), { 2, 2, boundBoxSizeZ });

        if (hasImage2)
        {
            session.addToPlotList4FD1E0(imageId2, imageOffset, imageOffset + Map::Pos3(0, 0, 2), { 2, 2, boundBoxSizeZ });
        }
    }
}
