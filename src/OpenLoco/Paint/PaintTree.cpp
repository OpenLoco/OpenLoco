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
    constexpr std::array<bool, 6> _500770 = { true, true, false, false, true, true };
    constexpr std::array<Map::Pos2, 4> kTreeQuadrantOffset = {
        Map::Pos2{ 7, 7 },
        Map::Pos2{ 7, 23 },
        Map::Pos2{ 23, 23 },
        Map::Pos2{ 23, 7 },
    };

    // 0x004BAEDA
    void paintTree(PaintSession& session, const Map::TreeElement& elTree)
    {
        // registers regs;
        // regs.esi = X86Pointer(&elTree);
        // regs.ecx = (session.getRotation() + elTree.data()[0]) & 0x3;
        // regs.dx = elTree.baseHeight();
        // call(0x004BAEDA, regs);
        session.setItemType(InteractionItem::tree);

        const auto* treeObj = ObjectManager::get<TreeObject>(elTree.treeObjectId());
        const uint8_t viewableRotation = (session.getRotation() + elTree.rotation()) & 0x3;
        const uint32_t treeFrameNum = (viewableRotation % treeObj->numRotations) + elTree.unk5l() * treeObj->numRotations;

        uint8_t season = elTree.season();

        const uint8_t altSeason = elTree.hasSnow() ? 1 : 0;
        bool hasImage2 = false;
        uint32_t imageIndex2 = 0;
        uint8_t noiseMask = 0;
        if (elTree.unk7l() != 7)
        {
            hasImage2 = true;

            season = _50076A[season];
            noiseMask = elTree.unk7l() + 1;
            auto image2Season = elTree.season();

            if (_500770[image2Season])
            {
                image2Season = season;
                season = elTree.season();
                noiseMask = 8 - noiseMask;
            }
            imageIndex2 = treeFrameNum + treeObj->sprites[altSeason][image2Season];
        }

        const auto seasonBaseImageIndex = treeObj->sprites[altSeason][season];

        std::optional<ImageId> shadowImageId = std::nullopt;
        if (treeObj->flags & TreeObjectFlags::hasShadow)
        {
            shadowImageId = ImageId{ treeObj->shadowImageOffset + treeFrameNum + seasonBaseImageIndex }.withTranslucency(ExtColour::unk32);
        }

        const uint8_t quadrant = (elTree.quadrant() + session.getRotation()) % 4;
        const auto imageOffset = Map::Pos3(kTreeQuadrantOffset[quadrant].x, kTreeQuadrantOffset[quadrant].y, elTree.baseHeight());

        const int16_t boundBoxSizeZ = std::min(elTree.clearZ() - elTree.baseZ(), 32) * Map::kSmallZStep - 3;

        const uint32_t imageIndex1 = treeFrameNum + seasonBaseImageIndex;
        ImageId imageId1{};
        ImageId imageId2{};

        if (elTree.isGhost())
        {
            session.setItemType(InteractionItem::noInteraction);
            imageId2 = Gfx::applyGhostToImage(imageIndex2);
            imageId1 = Gfx::applyGhostToImage(imageIndex1);
        }
        else if (treeObj->colours != 0)
        {
            // No vanilla object has this property set
            const auto colour = static_cast<Colour>(elTree.colour());
            imageId2 = ImageId{ imageIndex2, colour }.withNoiseMask(noiseMask);
            imageId1 = ImageId{ imageIndex1, colour };
        }
        else
        {
            imageId1 = ImageId{ imageIndex1 };
            imageId2 = ImageId{ imageIndex2 }.withNoiseMask(noiseMask);
        }

        if (shadowImageId)
        {
            if (session.getRenderTarget()->zoomLevel <= 1)
            {
                session.addToPlotListAsParent(*shadowImageId, imageOffset, imageOffset, { 18, 18, 1 });
            }
        }

        session.addToPlotListAsParent(imageId1, imageOffset, imageOffset + Map::Pos3(0, 0, 2), { 2, 2, boundBoxSizeZ });

        if (hasImage2)
        {
            session.addToPlotListAsChild(imageId2, imageOffset, imageOffset + Map::Pos3(0, 0, 2), { 2, 2, boundBoxSizeZ });
        }
    }
}
