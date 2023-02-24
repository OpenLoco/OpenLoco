#include "PaintIndustry.h"
#include "Map/IndustryElement.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScaffoldingObject.h"
#include "Paint.h"
#include "ScenarioManager.h"
#include "Ui.h"
#include "World/Industry.h"

namespace OpenLoco::Paint
{
    static World::Pos3 kImageOffsetBase1x1 = { 16, 16, 0 };
    static World::Pos3 kImageOffsetBase2x2 = { 0, 0, 0 };
    static World::Pos3 kBBOffsetBase1x1 = { 3, 3, 0 };
    static World::Pos3 kBBOffsetBase2x2 = { -8, -8, 0 };
    static World::Pos3 kBBSizeBase1x1 = { 26, 26, 0 };
    static World::Pos3 kBBSizeBase2x2 = { 38, 38, 0 };

    static void paintIndustryBuilding(PaintSession& session, const World::IndustryElement& elIndustry, const IndustryObject& indObj, const World::Pos3& imageOffset, const World::Pos3& bbOffset, const World::Pos3& bbSize, const ImageId& baseColour, const uint8_t rotation, const bool isMultiTile)
    {
        // 0xE0C3A0
        auto ticks = ScenarioManager::getScenarioTicks();
        uint8_t numSections = 0xF0; // 0xF0 represents all sections completed
        // Only used when under construction
        uint8_t sectionProgress = 0;
        if (!elIndustry.isConstructed())
        {
            ticks = 0;
            numSections = elIndustry.var_6_003F();
            sectionProgress = elIndustry.sectionProgress();
        }

        stdx::span<const std::uint8_t> animationSequence{};
        if ((elIndustry.var_6_003F() & (1 << 5)) && (elIndustry.var_6_003F() & (1 << 4)))
        {
            animationSequence = indObj.getAnimationSequence(elIndustry.var_6_003F() & 0x3);
        }

        uint32_t buildingType = elIndustry.buildingType();
        const auto buildingParts = indObj.getBuildingParts(buildingType);

        // 0x00525D4F
        uint8_t totalSectionHeight = 0;
        if (numSections != 0xF0)
        {
            int8_t sectionCount = numSections;
            for (const auto buildingPart : buildingParts)
            {
                totalSectionHeight += indObj.buildingPartHeight[buildingPart];
                sectionCount--;
                if (sectionCount == -1)
                {
                    totalSectionHeight = std::max<uint8_t>(1, totalSectionHeight);
                    break;
                }
            }
        }
        // 0x00525D30 (note this should be combined with a applyGhostToImage)
        const auto scaffoldingColour = indObj.scaffoldingColour;

        const auto scaffSegType = indObj.scaffoldingSegmentType;
        if (totalSectionHeight != 0 && scaffSegType != 0xFF)
        {
            const auto* scaffObj = ObjectManager::get<ScaffoldingObject>();
            const auto segmentHeight = scaffObj->segmentHeights[scaffSegType];
            const auto& scaffImages = isMultiTile ? getScaffoldingImages(scaffSegType).get2x2() : getScaffoldingImages(scaffSegType).get1x1();
            uint32_t baseScaffImageIdx = scaffObj->image;
            ImageId baseScaffImage{};
            if (elIndustry.isGhost())
            {
                baseScaffImage = Gfx::applyGhostToImage(baseScaffImageIdx);
            }
            else
            {
                baseScaffImage = ImageId(baseScaffImageIdx, scaffoldingColour);
            }
            ImageId scaffImage = baseScaffImage.withIndexOffset(scaffImages.back);
            auto segmentImageOffset = imageOffset;
            for (int8_t remainingHeight = totalSectionHeight; remainingHeight > 0; remainingHeight -= segmentHeight, segmentImageOffset.z += segmentHeight)
            {
                session.addToPlotListAsChild(scaffImage, segmentImageOffset, bbOffset, bbSize);
            }
        }

        int8_t sectionCount = numSections;
        auto sectionImageOffset = imageOffset;
        for (const auto buildingPart : buildingParts)
        {
            if (sectionCount == -1)
            {
                break;
            }
            auto& buildingAnimation = indObj.buildingPartAnimations[buildingPart];
            auto adjustedBuildingPart = buildingPart;
            if (buildingAnimation.numFrames)
            {
                auto frameMask = buildingAnimation.numFrames - 1;
                auto cl = buildingAnimation.animationSpeed & 0x7F;
                auto tickThing = ticks >> cl;
                if (buildingAnimation.animationSpeed & (1 << 7))
                {
                    auto pos = World::TilePos2(session.getUnkPosition());
                    tickThing += pos.x * 5;
                    tickThing += pos.y * 3;
                }
                adjustedBuildingPart += frameMask & tickThing;
            }
            else
            {
                if (!animationSequence.empty())
                {
                    auto tickThing = (ticks >> buildingAnimation.animationSpeed) & (animationSequence.size() - 1);
                    adjustedBuildingPart += animationSequence[tickThing];
                }
            }
            const auto sectionHeight = indObj.buildingPartHeight[adjustedBuildingPart];
            const uint32_t imageIdx = adjustedBuildingPart * 4 + indObj.var_12 + rotation;
            ImageId image = baseColour.withIndex(imageIdx);
            if (sectionCount == 0 && !baseColour.isBlended())
            {
                image = image.withNoiseMask((sectionProgress + 1) & 0x7);
            }
            session.addToPlotListAsChild(image, sectionImageOffset, bbOffset, bbSize);
            sectionImageOffset.z += sectionHeight;
            sectionCount--;
        }
        if (totalSectionHeight != 0 && scaffSegType != 0xFF)
        {
            const auto* scaffObj = ObjectManager::get<ScaffoldingObject>();
            const auto segmentHeight = scaffObj->segmentHeights[scaffSegType];
            const auto& scaffImages = isMultiTile ? getScaffoldingImages(scaffSegType).get2x2() : getScaffoldingImages(scaffSegType).get1x1();
            uint32_t baseScaffImageIdx = scaffObj->image;
            ImageId baseScaffImage{};
            if (elIndustry.isGhost())
            {
                baseScaffImage = Gfx::applyGhostToImage(baseScaffImageIdx);
            }
            else
            {
                baseScaffImage = ImageId(baseScaffImageIdx, scaffoldingColour);
            }
            auto scaffImage = baseScaffImage.withIndexOffset(scaffImages.front);
            auto segmentImageOffset = imageOffset;
            for (int8_t remainingHeight = totalSectionHeight; remainingHeight > 0; remainingHeight -= segmentHeight, segmentImageOffset.z += segmentHeight)
            {
                session.addToPlotListAsChild(scaffImage, segmentImageOffset, bbOffset, bbSize);
            }
            scaffImage = baseScaffImage.withIndexOffset(scaffImages.getRoof(rotation));
            session.addToPlotListAsChild(scaffImage, segmentImageOffset, bbOffset, bbSize);
        }
    }

    // 0x00453C52
    void paintIndustry(PaintSession& session, const World::IndustryElement& elIndustry)
    {
        session.setItemType(Ui::ViewportInteraction::InteractionItem::industry);
        auto* industry = elIndustry.industry();
        auto* indObj = industry->getObject();

        ImageId baseColour(0, elIndustry.var_6_F800());
        if (elIndustry.isGhost())
        {
            baseColour = Gfx::applyGhostToImage(0);
        }
        // Combine this with any imageId
        const uint8_t rotation = (session.getRotation() + elIndustry.rotation()) & 0x3;

        // 0x00525D4E
        const int16_t bbLengthZ = std::min(elIndustry.clearHeight() - elIndustry.baseHeight(), 128) - 2;

        // 0x00E0C3A4
        uint32_t buildingType = elIndustry.buildingType();
        const auto baseHeight = elIndustry.baseHeight();

        bool isMultiTile = indObj->buildingSizeFlags & (1 << buildingType);

        // Note: Image offsets will change as you move up the building but bboffset/size does not
        const World::Pos3 imageOffset = (isMultiTile ? kImageOffsetBase2x2 : kImageOffsetBase1x1) + World::Pos3{ 0, 0, baseHeight };
        const World::Pos3 bbOffset = (isMultiTile ? kBBOffsetBase2x2 : kBBOffsetBase1x1) + World::Pos3{ 0, 0, baseHeight };
        const World::Pos3 bbSize = (isMultiTile ? kBBSizeBase2x2 : kBBSizeBase1x1) + World::Pos3{ 0, 0, bbLengthZ };

        session.resetLastPS(); // Odd...
        if (indObj->hasFlags(IndustryObjectFlags::hasShadows))
        {
            if (session.getRenderTarget()->zoomLevel <= 1)
            {
                const auto shadowImageOffset = buildingType * 4 + indObj->var_0E + rotation;
                const ImageId shadowImage = baseColour.withIndex(shadowImageOffset).withTranslucency(Colours::getShadow(elIndustry.var_6_F800()));
                if (isMultiTile)
                {
                    session.addToPlotListAsChild(shadowImage, imageOffset, bbOffset, bbSize);
                }
                else
                {
                    session.addToPlotListAsChild(shadowImage, imageOffset, bbOffset, bbSize);
                }
            }
        }

        if (isMultiTile)
        {
            uint8_t sequenceIndex = elIndustry.sequenceIndex();
            // Only the front of the 2x2 area will draw. Images are sized to overlap into the other tiles.
            if ((sequenceIndex ^ (1 << 1)) == ((-session.getRotation()) & 0x3))
            {
                paintIndustryBuilding(session, elIndustry, *indObj, imageOffset, bbOffset, bbSize, baseColour, rotation, isMultiTile);
            }
            session.setSegmentSupportHeight(SegmentFlags::all, 0xFFFF, 0);
            session.setGeneralSupportHeight(0xFFFF, 0); // TODO: Check if this works previously would not set slope to zero
        }
        else
        {
            paintIndustryBuilding(session, elIndustry, *indObj, imageOffset, bbOffset, bbSize, baseColour, rotation, isMultiTile);
        }
    }
}
