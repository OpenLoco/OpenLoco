#include "PaintBuilding.h"
#include "Graphics/RenderTarget.h"
#include "Map/BuildingElement.h"
#include "Map/TileManager.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScaffoldingObject.h"
#include "Paint.h"
#include "ScenarioManager.h"
#include "Ui/ViewportInteraction.h"

namespace OpenLoco::Paint
{
    // TODO: Docks, Airport, Building, Industry all have very similar code...

    constexpr World::Pos3 kImageOffsetBase1x1 = { 16, 16, 0 };
    constexpr World::Pos3 kImageOffsetBase2x2 = { 0, 0, 0 };
    constexpr World::Pos3 kBBOffsetBase1x1 = { 3, 3, 0 };
    constexpr World::Pos3 kBBOffsetBase2x2 = { -8, -8, 0 };
    constexpr World::Pos3 kBBSizeBase1x1 = { 26, 26, 0 };
    constexpr World::Pos3 kBBSizeBase2x2 = { 38, 38, 0 };

    static void paintBuildingBuilding(PaintSession& session, const World::BuildingElement& elBuilding, const BuildingObject& buildingObj, const World::Pos3& imageOffset, const World::Pos3& bbOffset, const World::Pos3& bbSize, const ImageId& baseColour, const uint8_t rotation, const bool isMultiTile)
    {
        // 0xE0C3A0
        auto ticks = ScenarioManager::getScenarioTicks();
        uint8_t numSections = 0xF0; // 0xF0 represents all sections completed
        // Only used when under construction
        uint8_t sectionProgress = 0;
        if (!elBuilding.isConstructed())
        {
            ticks = 0;
            numSections = elBuilding.age();
            sectionProgress = elBuilding.unk5u();
        }

        uint32_t variation = elBuilding.variation();
        const auto parts = buildingObj.getBuildingParts(variation);

        // 0x00525D4F
        uint8_t totalSectionHeight = 0;
        if (numSections != 0xF0)
        {
            int8_t sectionCount = numSections;
            const auto partHeights = buildingObj.getBuildingPartHeights();
            for (const auto part : parts)
            {
                totalSectionHeight += partHeights[part];
                sectionCount--;
                if (sectionCount == -1)
                {
                    totalSectionHeight = std::max<uint8_t>(1, totalSectionHeight);
                    break;
                }
            }
        }
        // 0x00525D30 (note this should be combined with a applyGhostToImage)
        const auto scaffoldingColour = buildingObj.scaffoldingColour;

        const auto scaffSegType = buildingObj.scaffoldingSegmentType;
        if (totalSectionHeight != 0 && scaffSegType != 0xFF)
        {
            const auto* scaffObj = ObjectManager::get<ScaffoldingObject>();
            const auto segmentHeight = scaffObj->segmentHeights[scaffSegType];
            const auto& scaffImages = isMultiTile ? getScaffoldingImages(scaffSegType).get2x2() : getScaffoldingImages(scaffSegType).get1x1();
            uint32_t baseScaffImageIdx = scaffObj->image;
            ImageId baseScaffImage{};
            if (elBuilding.isGhost())
            {
                baseScaffImage = Gfx::applyGhostToImage(baseScaffImageIdx);
            }
            else
            {
                baseScaffImage = ImageId(baseScaffImageIdx, scaffoldingColour);
            }
            ImageId scaffImage = baseScaffImage.withIndexOffset(scaffImages.back);
            auto segmentImageOffset = imageOffset;
            for (coord_t remainingHeight = totalSectionHeight; remainingHeight > 0; remainingHeight -= segmentHeight, segmentImageOffset.z += segmentHeight)
            {
                session.addToPlotListAsChild(scaffImage, segmentImageOffset, bbOffset, bbSize);
            }
        }

        int8_t sectionCount = numSections;
        auto sectionImageOffset = imageOffset;
        const auto partHeights = buildingObj.getBuildingPartHeights();
        const auto partAnimations = buildingObj.getBuildingPartAnimations();
        for (const auto part : parts)
        {
            if (sectionCount == -1)
            {
                break;
            }
            const auto partAnimation = partAnimations[part];
            auto adjustedPart = part;

            auto frameMask = partAnimation.numFrames - 1;
            auto cl = partAnimation.animationSpeed & 0x7F;
            auto tickThing = ticks >> cl;
            if (partAnimation.animationSpeed & (1 << 7))
            {
                auto pos = World::toTileSpace(session.getUnkPosition());
                tickThing += pos.x * 5;
                tickThing += pos.y * 3;
            }
            adjustedPart += frameMask & tickThing;

            const auto sectionHeight = partHeights[adjustedPart];
            const uint32_t imageIdx = adjustedPart * 4 + buildingObj.image + rotation;
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
            if (elBuilding.isGhost())
            {
                baseScaffImage = Gfx::applyGhostToImage(baseScaffImageIdx);
            }
            else
            {
                baseScaffImage = ImageId(baseScaffImageIdx, scaffoldingColour);
            }
            auto scaffImage = baseScaffImage.withIndexOffset(scaffImages.front);
            auto segmentImageOffset = imageOffset;
            for (coord_t remainingHeight = totalSectionHeight; remainingHeight > 0; remainingHeight -= segmentHeight, segmentImageOffset.z += segmentHeight)
            {
                session.addToPlotListAsChild(scaffImage, segmentImageOffset, bbOffset, bbSize);
            }
            scaffImage = baseScaffImage.withIndexOffset(scaffImages.getRoof(rotation));
            session.addToPlotListAsChild(scaffImage, segmentImageOffset, bbOffset, bbSize);
        }

        if (totalSectionHeight == 0)
        {
            for (auto animIdx = 0; animIdx < buildingObj.numElevatorSequences; ++animIdx)
            {
                auto sequence = buildingObj.getElevatorHeightSequence(animIdx);
                auto tickThing = ScenarioManager::getScenarioTicks() / 2;
                auto pos = World::toTileSpace(session.getUnkPosition());
                tickThing += pos.x * 8;
                tickThing += pos.y * 8;
                // Sequence is always a power of 2 so (& -1) is like modulo
                const auto seqIdx = tickThing & (sequence.size() - 1);
                const auto elevatorHeight = sequence[seqIdx];

                const auto image = baseColour.withIndex(buildingObj.image + buildingObj.numParts * 4 + animIdx * 4 + rotation);

                const auto offset = imageOffset + World::Pos3(0, 0, elevatorHeight);
                session.addToPlotListAsChild(image, offset, bbOffset, bbSize);
            }
        }
    }

    // 0x0042C6C4
    void paintBuilding(PaintSession& session, const World::BuildingElement& elBuilding)
    {
        session.setItemType(Ui::ViewportInteraction::InteractionItem::building);
        auto* buildingObj = elBuilding.getObject();
        if (buildingObj->hasFlags(BuildingObjectFlags::isHeadquarters))
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::headquarterBuilding);
        }

        ImageId baseColour(0, elBuilding.colour());
        if (elBuilding.isGhost())
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            baseColour = Gfx::applyGhostToImage(0);
        }
        // Combine this with any imageId
        const uint8_t rotation = (session.getRotation() + elBuilding.rotation()) & 0x3;

        // 0x00525D4E
        const int16_t bbLengthZ = std::min(elBuilding.clearHeight() - elBuilding.baseHeight(), 128) - 2;

        // 0x00525D18
        const auto variation = elBuilding.variation();

        const auto baseHeight = elBuilding.baseHeight();

        bool isMultiTile = buildingObj->hasFlags(BuildingObjectFlags::largeTile);

        // Note: Image offsets will change as you move up the building but bboffset/size does not
        const World::Pos3 imageOffset = (isMultiTile ? kImageOffsetBase2x2 : kImageOffsetBase1x1) + World::Pos3{ 0, 0, baseHeight };
        const World::Pos3 bbOffset = (isMultiTile ? kBBOffsetBase2x2 : kBBOffsetBase1x1) + World::Pos3{ 0, 0, baseHeight };
        const World::Pos3 bbSize = (isMultiTile ? kBBSizeBase2x2 : kBBSizeBase1x1) + World::Pos3{ 0, 0, bbLengthZ };

        session.resetLastPS(); // Odd...
        if (buildingObj->hasFlags(BuildingObjectFlags::hasShadows))
        {
            if (session.getRenderTarget()->zoomLevel <= 1)
            {
                const auto shadowImageOffset = (buildingObj->numParts + buildingObj->numElevatorSequences + variation) * 4 + buildingObj->image + rotation;
                const ImageId shadowImage = ImageId(shadowImageOffset).withTranslucency(Colours::getShadow(Colour::orange));
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
            const auto sequenceIndex = elBuilding.sequenceIndex();
            // Only the front of the 2x2 area will draw. Images are sized to overlap into the other tiles.
            if ((sequenceIndex ^ (1 << 1)) == ((-session.getRotation()) & 0x3))
            {
                paintBuildingBuilding(session, elBuilding, *buildingObj, imageOffset, bbOffset, bbSize, baseColour, rotation, isMultiTile);
            }
            session.setSegmentsSupportHeight(SegmentFlags::all, 0xFFFF, 0);
            session.setGeneralSupportHeight(0xFFFF, 0); // TODO: Check if this works previously would not set slope to zero
        }
        else
        {
            paintBuildingBuilding(session, elBuilding, *buildingObj, imageOffset, bbOffset, bbSize, baseColour, rotation, isMultiTile);
        }
    }
}
