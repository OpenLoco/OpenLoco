#include "PaintWall.h"
#include "Graphics/Colour.h"
#include "Map/WallElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/WallObject.h"
#include "Paint.h"
#include "ScenarioManager.h"
#include "Ui/ViewportInteraction.h"
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
        { 1, 1, 1 },
        { 2, 30, 1 },
        { 30, 2, 1 },
        { 1, 1, 1 },
    };

    static constexpr World::Pos3 kBBoxLengths[4] = {
        { 1, 28, 0 },
        { 29, 1, 0 },
        { 1, 29, 0 },
        { 28, 1, 0 },
    };

    using namespace WallObj::ImageIds;

    static constexpr std::array<std::array<std::array<uint8_t, 3>, 4>, 2> kImageOffsets = {
        std::array<std::array<uint8_t, 3>, 4>{
            std::array<uint8_t, 3>{ kSlopedNE, kSlopedSW, kFlatNE },
            std::array<uint8_t, 3>{ kSlopedSE, kSlopedNW, kFlatSE },
            std::array<uint8_t, 3>{ kSlopedSW, kSlopedNE, kFlatNE },
            std::array<uint8_t, 3>{ kSlopedNW, kSlopedSE, kFlatSE },
        },
        std::array<std::array<uint8_t, 3>, 4>{
            std::array<uint8_t, 3>{ kSlopedNE, kSlopedSW, kFlatNE },
            std::array<uint8_t, 3>{ kGlassSlopedSE, kGlassSlopedNW, kGlassFlatSE },
            std::array<uint8_t, 3>{ kGlassSlopedSW, kGlassSlopedNE, kGlassFlatNE },
            std::array<uint8_t, 3>{ kSlopedNW, kSlopedSE, kFlatSE },
        }
    };
    static constexpr std::array<std::array<uint8_t, 3>, 4> kImageOffsetsGlass = {
        std::array<uint8_t, 3>{ kGlassSlopedNE, kGlassSlopedSW, kGlassFlatNE },
        std::array<uint8_t, 3>{ kGlassSlopedSE, kGlassSlopedNW, kGlassFlatSE },
        std::array<uint8_t, 3>{ kGlassSlopedSW, kGlassSlopedNE, kGlassFlatNE },
        std::array<uint8_t, 3>{ kGlassSlopedNW, kGlassSlopedSE, kGlassFlatSE },
    };

    static constexpr uint8_t slopeFlagsToIndex(EdgeSlope flags)
    {
        // Slope flags to index are 0 = downwards, 1 == upwards, 2 == no slope
        if ((flags & EdgeSlope::downwards) != EdgeSlope::none)
        {
            return 0;
        }
        if ((flags & EdgeSlope::upwards) != EdgeSlope::none)
        {
            return 1;
        }
        return 2;
    }

    static uint32_t getWallImageIndexOffset(const World::WallElement& elWall, int32_t rotation, bool isTwoSided)
    {
        const auto index = slopeFlagsToIndex(elWall.getSlopeFlags());

        return kImageOffsets[isTwoSided][rotation][index];
    }
    static uint32_t getWallImageIndexOffsetGlass(const World::WallElement& elWall, int32_t rotation)
    {
        const auto index = slopeFlagsToIndex(elWall.getSlopeFlags());

        return kImageOffsetsGlass[rotation][index];
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

        const coord_t objectHeight = kSmallZStep * wallObject->height - 2;
        const int32_t rotation = (session.getRotation() + elWall.rotation()) & 0x3;

        const auto heightOffset = World::Pos3{ 0, 0, elWall.baseHeight() };

        const auto offset = kOffsets[rotation] + heightOffset;
        const auto bboxOffset = kBBoxOffsets[rotation] + heightOffset;
        const auto bboxLength = kBBoxLengths[rotation] + World::Pos3{ 0, 0, objectHeight };

        const auto isTwoSided = (wallObject->flags & WallObjectFlags::twoSided) != WallObjectFlags::none;
        const auto imageOffset = getWallImageIndexOffset(elWall, rotation, isTwoSided);
        const auto imageIndex = wallObject->sprite + imageOffset;

        if ((wallObject->flags & WallObjectFlags::hasGlass) != WallObjectFlags::none)
        {
            const auto imageId = getWallImageId(imageIndex, isGhost, elWall, wallObject);
            session.addToPlotListAsParent(
                imageId,
                offset,
                bboxOffset,
                bboxLength);

            if (!isGhost)
            {
                const auto blendColour = Colours::getGlass(imageId.getPrimary());
                const auto glassImageIndex = wallObject->sprite + getWallImageIndexOffsetGlass(elWall, rotation);
                session.addToPlotListAsChild(
                    ImageId(glassImageIndex).withTranslucency(blendColour),
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
