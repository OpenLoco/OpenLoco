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
    static ImageId getWallImage(ImageIndex imageIndex, bool isGhost, const World::WallElement& elWall, const WallObject* wallObject)
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
    void paintWall(PaintSession& session, World::WallElement& elWall)
    {
        const auto* wallObject = ObjectManager::get<WallObject>(elWall.wallObjectId());
        assert(wallObject != nullptr);

        const auto isGhost = elWall.isGhost();
        session.setItemType(isGhost ? InteractionItem::noInteraction : InteractionItem::wall);

        uint32_t frameNo = 0;
        if (wallObject->var_09 & 0x10)
        {
            frameNo = 2 * (ScenarioManager::getScenarioTicks() & 7U);
        }

        uint8_t type = elWall.rawData()[0];
        int16_t height = 4 * wallObject->height - 2;
        uint32_t imageOffset = 0;
        uint32_t imageIndex = 0;
        coord_t baseHeight = elWall.baseHeight();
        coord_t baseHeightEnd = baseHeight + 1;

        const auto rotation = (session.getRotation() + elWall.rotation()) & 0x3;

        switch (rotation)
        {
            case 0:
                imageOffset = 3;
                if ((type & 0x80u) == 0)
                {
                    imageOffset = 5;
                    if ((type & 0x40) == 0)
                        imageOffset = 1;
                }
                imageIndex = frameNo + wallObject->sprite + imageOffset;
                if ((wallObject->flags & WallObjectFlags::unk1) != WallObjectFlags::none)
                {
                    const auto imageId = getWallImage(imageIndex, isGhost, elWall, wallObject);
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
                    const auto imageId = getWallImage(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 0, 0, baseHeight },
                        World::Pos3{ 1, 1, baseHeightEnd },
                        World::Pos3{ 1, 28, height });

                    // if (!v9)
                    //*(_DWORD*)(v8 + 4) = tertiaryColour;
                }
                break;
            case 1:
                imageOffset = 2;
                if ((type & 0x80u) == 0)
                {
                    imageOffset = 4;
                    if ((type & 0x40) == 0)
                        imageOffset = 0;
                }
                if ((wallObject->flags & WallObjectFlags::unk1) != WallObjectFlags::none)
                {
                    if ((wallObject->flags & WallObjectFlags::unk3) != WallObjectFlags::none)
                        imageOffset += 12;
                    imageIndex = wallObject->sprite + imageOffset;

                    const auto imageId = getWallImage(imageIndex, isGhost, elWall, wallObject);
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
                    imageIndex = frameNo + wallObject->sprite + imageOffset;
                    const auto imageId = getWallImage(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 1, 31, baseHeight },
                        World::Pos3{ 2, 30, baseHeightEnd },
                        World::Pos3{ 29, 1, height });
                    // if (!v9)
                    //*(_DWORD*)(v13 + 4) = tertiaryColour;
                }
                break;
            case 2:
                imageOffset = 5;
                if ((type & 0x80u) == 0)
                {
                    imageOffset = 3;
                    if ((type & 0x40) == 0)
                        imageOffset = 1;
                }
                if ((wallObject->flags & WallObjectFlags::unk1) != WallObjectFlags::none)
                {
                    if ((wallObject->flags & WallObjectFlags::unk3) != WallObjectFlags::none)
                        imageOffset += 6;
                    imageIndex = wallObject->sprite + imageOffset;
                    const auto imageId = getWallImage(imageIndex, isGhost, elWall, wallObject);
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
                    imageIndex = frameNo + wallObject->sprite + imageOffset;
                    const auto imageId = getWallImage(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 31, 0, baseHeight },
                        World::Pos3{ 30, 2, baseHeightEnd },
                        World::Pos3{ 1, 29, height });
                    // if (!v9)
                    //*(_DWORD*)(v18 + 4) = tertiaryColour;
                }
                break;
            case 3:
                imageOffset = 4;
                if ((type & 0x80u) == 0)
                {
                    imageOffset = 2;
                    if ((type & 0x40) == 0)
                        imageOffset = 0;
                }
                imageIndex = frameNo + wallObject->sprite + imageOffset;
                if ((wallObject->flags & WallObjectFlags::unk1) != WallObjectFlags::none)
                {
                    const auto imageId = getWallImage(imageIndex, isGhost, elWall, wallObject);
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
                    const auto imageId = getWallImage(imageIndex, isGhost, elWall, wallObject);
                    session.addToPlotListAsParent(
                        imageId,
                        World::Pos3{ 2, 1, baseHeight },
                        World::Pos3{ 1, 1, baseHeightEnd },
                        World::Pos3{ 28, 1, height });
                    // if (!v9)
                    //*(_DWORD*)(v23 + 4) = tertiaryColour;
                }
                break;
            default:
                assert(false);
                break;
        }
    }
}
