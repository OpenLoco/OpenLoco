#include "PaintWall.h"
#include "Graphics/Colour.h"
#include "Map/WallElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/WallObject.h"
#include "Paint.h"
#include "ScenarioManager.h"
#include "Ui.h"
#include <cassert>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;

namespace OpenLoco::Paint
{
    static constexpr World::Pos3 kOffsets[4] = {
        { 0, 0, 0 },
        { 1, 31, 0 },
        { 31, 0, 0 },
        { 2, 1, 0 },
    };

    static constexpr World::Pos3 kBBoxOffsets[4] = {
        { 1, 1, 0 },
        { 2, 30, 0 },
        { 30, 2, 0 },
        { 1, 1, 0 },
    };

    static constexpr World::Pos3 kBBoxLengths[4] = {
        { 1, 28, 0 },
        { 29, 1, 0 },
        { 1, 29, 0 },
        { 28, 1, 0 },
    };

    static constexpr uint8_t kImageOffsets[4][3] = {
        { 3, 5, 1 },
        { 2, 4, 0 },
        { 5, 3, 1 },
        { 4, 2, 0 },
    };

    static uint32_t getWallImageIndexOffset(const World::WallElement& elWall, int32_t rotation)
    {
        // TODO: Add the appropriate getters to WallElement
        uint8_t type = elWall.rawData()[0];

        // TODO: Make this branch-less.
        uint32_t imageOffset = kImageOffsets[rotation][0];
        if ((type & 0x80u) == 0)
        {
            imageOffset = kImageOffsets[rotation][1];
            if ((type & 0x40) == 0)
                imageOffset = kImageOffsets[rotation][2];
        }

        return imageOffset;
    }

    static ImageId getWallImageId(ImageIndex imageIndex, bool isGhost, const World::WallElement& elWall, const WallObject* wallObject)
    {
        if (isGhost)
        {
            return Gfx::applyGhostToImage(imageIndex);
        }

        ImageId imageId{ imageIndex };

        if ((wallObject->flags & WallObjectFlags::hasPrimaryColour) != WallObjectFlags::none)
        {
            imageId = imageId.withPrimary(elWall.getPrimaryColour());
        }

        if ((wallObject->flags & WallObjectFlags::hasSecondaryColour) != WallObjectFlags::none)
        {
            imageId = imageId.withSecondary(elWall.getSecondaryColour());
        }

        return imageId;
    }

    // 0x004C3D7C
    void paintWall(PaintSession& session, const World::WallElement& elWall)
    {
        const auto* wallObject = ObjectManager::get<WallObject>(elWall.wallObjectId());
        assert(wallObject != nullptr);

        const auto isGhost = elWall.isGhost();
        session.setItemType(isGhost ? InteractionItem::noInteraction : InteractionItem::wall);

        const coord_t height = 4 * wallObject->height - 2;
        const coord_t baseHeight = elWall.baseHeight();
        const coord_t baseHeightEnd = baseHeight + 1;
        const int32_t rotation = (session.getRotation() + elWall.rotation()) & 0x3;

        const auto offset = kOffsets[rotation] + World::Pos3{ 0, 0, baseHeight };
        const auto bboxOffset = kBBoxOffsets[rotation] + World::Pos3{ 0, 0, baseHeightEnd };
        const auto bboxLength = kBBoxLengths[rotation] + World::Pos3{ 0, 0, height };

        const auto imageOffset = getWallImageIndexOffset(elWall, rotation);
        const auto imageIndex = wallObject->sprite + imageOffset;

        if ((wallObject->flags & WallObjectFlags::unk1) != WallObjectFlags::none)
        {
            const auto imageId = getWallImageId(imageIndex, isGhost, elWall, wallObject);
            session.addToPlotListAsParent(
                imageId,
                offset,
                bboxOffset,
                bboxLength);

            if (!isGhost)
            {
                session.addToPlotListAsChild(
                    imageId.withRemap(ExtColour::unk74).withIndexOffset(6),
                    offset,
                    bboxOffset,
                    bboxLength);
            }
        }
        else
        {
            const auto imageId = getWallImageId(imageIndex, isGhost, elWall, wallObject);
            session.addToPlotListAsParent(
                imageId,
                offset,
                bboxOffset,
                bboxLength);
        }
    }
}
