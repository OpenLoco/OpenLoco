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
    static uint32_t getWallImageIndexOffset(const World::WallElement& elWall, int32_t rotation)
    {
        static constexpr uint8_t kImageOffsets[4][3] = {
            { 3, 5, 1 },
            { 2, 4, 0 },
            { 5, 3, 1 },
            { 4, 2, 0 },
        };

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

        uint32_t imageOffset = 0;
        uint32_t imageIndex = 0;

        switch (rotation)
        {
            case 0:
                imageOffset = getWallImageIndexOffset(elWall, rotation);
                imageIndex = wallObject->sprite + imageOffset;
                if ((wallObject->flags & WallObjectFlags::unk1) != WallObjectFlags::none)
                {
                    const auto imageId = getWallImageId(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 0, 0, baseHeight },
                        World::Pos3{ 1, 1, baseHeightEnd },
                        World::Pos3{ 1, 28, height });

                    if (!isGhost)
                    {
                        session.addToPlotListAsChild(
                            imageId.withRemap(ExtColour::unk74).withIndexOffset(6),
                            World::Pos3{ 0, 0, baseHeight },
                            World::Pos3{ 1, 1, baseHeightEnd },
                            World::Pos3{ 1, 28, height });
                    }
                }
                else
                {
                    const auto imageId = getWallImageId(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 0, 0, baseHeight },
                        World::Pos3{ 1, 1, baseHeightEnd },
                        World::Pos3{ 1, 28, height });
                }
                break;
            case 1:
                imageOffset = getWallImageIndexOffset(elWall, rotation);
                if ((wallObject->flags & WallObjectFlags::unk1) != WallObjectFlags::none)
                {
                    if ((wallObject->flags & WallObjectFlags::unk3) != WallObjectFlags::none)
                        imageOffset += 12;
                    imageIndex = wallObject->sprite + imageOffset;

                    const auto imageId = getWallImageId(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 1, 31, baseHeight },
                        World::Pos3{ 2, 30, baseHeightEnd },
                        World::Pos3{ 29, 1, height });

                    if (!isGhost)
                    {
                        session.addToPlotListAsChild(
                            imageId.withRemap(ExtColour::unk74).withIndexOffset(6),
                            World::Pos3{ 1, 31, baseHeight },
                            World::Pos3{ 2, 30, baseHeightEnd },
                            World::Pos3{ 29, 1, height });
                    }
                }
                else
                {
                    if ((wallObject->flags & WallObjectFlags::unk3) != WallObjectFlags::none)
                        imageOffset += 6;
                    imageIndex = wallObject->sprite + imageOffset;
                    const auto imageId = getWallImageId(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 1, 31, baseHeight },
                        World::Pos3{ 2, 30, baseHeightEnd },
                        World::Pos3{ 29, 1, height });
                }
                break;
            case 2:
                imageOffset = getWallImageIndexOffset(elWall, rotation);
                if ((wallObject->flags & WallObjectFlags::unk1) != WallObjectFlags::none)
                {
                    if ((wallObject->flags & WallObjectFlags::unk3) != WallObjectFlags::none)
                        imageOffset += 6;
                    imageIndex = wallObject->sprite + imageOffset;
                    const auto imageId = getWallImageId(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 31, 0, baseHeight },
                        World::Pos3{ 30, 2, baseHeightEnd },
                        World::Pos3{ 1, 29, height });

                    if (!isGhost)
                    {
                        session.addToPlotListAsChild(
                            imageId.withRemap(ExtColour::unk74).withIndexOffset(6),
                            World::Pos3{ 31, 0, baseHeight },
                            World::Pos3{ 30, 2, baseHeightEnd },
                            World::Pos3{ 1, 29, height });
                    }
                }
                else
                {
                    if ((wallObject->flags & WallObjectFlags::unk3) != WallObjectFlags::none)
                        imageOffset += 6;
                    imageIndex = wallObject->sprite + imageOffset;
                    const auto imageId = getWallImageId(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 31, 0, baseHeight },
                        World::Pos3{ 30, 2, baseHeightEnd },
                        World::Pos3{ 1, 29, height });
                }
                break;
            case 3:
                imageOffset = getWallImageIndexOffset(elWall, rotation);
                imageIndex = wallObject->sprite + imageOffset;
                if ((wallObject->flags & WallObjectFlags::unk1) != WallObjectFlags::none)
                {
                    const auto imageId = getWallImageId(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 2, 1, baseHeight },
                        World::Pos3{ 1, 1, baseHeightEnd },
                        World::Pos3{ 28, 1, height });
                    if (!isGhost)
                    {
                        session.addToPlotListAsChild(
                            imageId.withRemap(ExtColour::unk74).withIndexOffset(6),
                            World::Pos3{ 2, 1, baseHeight },
                            World::Pos3{ 1, 1, baseHeightEnd },
                            World::Pos3{ 28, 1, height });
                    }
                }
                else
                {
                    const auto imageId = getWallImageId(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 2, 1, baseHeight },
                        World::Pos3{ 1, 1, baseHeightEnd },
                        World::Pos3{ 28, 1, height });
                }
                break;
            default:
                assert(false);
                break;
        }
    }
}
