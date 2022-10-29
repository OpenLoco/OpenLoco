#include "PaintIndustry.h"
#include "../Industry.h"
#include "../Map/Tile.h"
#include "../Objects/IndustryObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/ScaffoldingObject.h"
#include "../ScenarioManager.h"
#include "../Ui.h"
#include "Paint.h"

namespace OpenLoco::Paint
{
    // 0x00453C52
    void paintIndustry(PaintSession& session, const Map::IndustryElement& elIndustry)
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
        // 0xE0C3A0
        auto ticks = ScenarioManager::getScenarioTicks();
        uint8_t bl = 0xF0;
        uint8_t bh = 0;
        if (!elIndustry.hasHighTypeFlag())
        {
            ticks = 0;
            bl = elIndustry.var_6_003F();
            bh = elIndustry.var_5_E0();
        }

        // 0x00525D5C
        uint8_t sequenceIndex = elIndustry.sequenceIndex();

        // 0x00525D4E
        const int16_t bbLengthZ = std::min(elIndustry.clearHeight() - elIndustry.baseHeight(), 128) - 2;

        // 0x00E0C3B0
        stdx::span<const std::uint8_t> unkE0C3B0{};
        if ((elIndustry.var_6_003F() & (1 << 5)) && (elIndustry.var_6_003F() & (1 << 4)))
        {
            unkE0C3B0 = indObj->getUnk28(elIndustry.var_6_003F() & 0x3);
        }

        // 0x00E0C3A4
        uint32_t buildingType = elIndustry.buildingType();
        const auto buildingParts = indObj->getBuildingParts(buildingType);
        const auto baseHeight = elIndustry.baseHeight();
        int16_t bbZOffset = baseHeight;
        bool isMultiTile = indObj->buildingSizeFlags & (1 << buildingType);

        session.resetLastPS(); // Odd...
        if (indObj->flags & IndustryObjectFlags::hasShadows)
        {
            if (session.getRenderTarget()->zoomLevel <= 1)
            {
                const auto shadowImageOffset = buildingType * 4 + indObj->var_0E + rotation;
                const ImageId shadowImage = baseColour.withIndex(shadowImageOffset).withTranslucency(Colours::getShadow(elIndustry.var_6_F800()));
                if (isMultiTile)
                {
                    session.addToPlotListAsChild(shadowImage, { 0, 0, baseHeight }, { -8, -8, bbZOffset }, { 38, 38, bbLengthZ });
                }
                else
                {
                    session.addToPlotListAsChild(shadowImage, { 16, 16, baseHeight }, { 3, 3, bbZOffset }, { 26, 26, bbLengthZ });
                }
            }
        }

        // 0x00525D4F
        uint8_t totalSectionHeight = 0;
        if (bl != 0xF0)
        {
            int8_t sectionCount = bl;
            for (const auto buildingPart : buildingParts)
            {
                totalSectionHeight += indObj->buildingPartHeight[buildingPart];
                sectionCount--;
                if (sectionCount == -1)
                {
                    totalSectionHeight = std::max<uint8_t>(1, totalSectionHeight);
                    break;
                }
            }
        }
        // 0x00525D30 (note this should be combined with a applyGhostToImage)
        const auto scaffoldingColour = indObj->scaffoldingColour;

        if (isMultiTile)
        {
            // 0x004540A6
            if ((sequenceIndex ^ (1 << 1)) == ((-session.getRotation()) & 0x3))
            {
                const auto scaffSegType = indObj->scaffoldingSegmentType;
                if (totalSectionHeight != 0 && scaffSegType != 0xFF)
                {
                    const auto* scaffObj = ObjectManager::get<ScaffoldingObject>();
                    const auto segmentHeight = scaffObj->segmentHeights[scaffSegType];
                    const auto& scaffImages = getScaffoldingImages(scaffSegType).get2x2();
                    uint32_t scaffImageIdx = scaffObj->image + scaffImages.part0;
                    ImageId scaffImage{};
                    if (elIndustry.isGhost())
                    {
                        scaffImage = Gfx::applyGhostToImage(scaffImageIdx);
                    }
                    else
                    {
                        scaffImage = ImageId(scaffImageIdx, scaffoldingColour);
                    }
                    auto height = baseHeight;
                    for (int8_t remainingHeight = totalSectionHeight; remainingHeight > 0; remainingHeight -= segmentHeight, height += segmentHeight)
                    {
                        session.addToPlotListAsChild(scaffImage, { 0, 0, height }, { -8, -8, bbZOffset }, { 38, 38, bbLengthZ });
                    }
                }

                int8_t sectionCount = bl;
                auto height = baseHeight;
                for (const auto buildingPart : buildingParts)
                {
                    if (sectionCount == -1)
                    {
                        break;
                    }
                    auto& thing = indObj->var_24[buildingPart];
                    auto adjustedBuildingPart = buildingPart;
                    if (thing.var_00)
                    {
                        auto al = thing.var_00 - 1;
                        auto cl = thing.var_01 & 0x7F;
                        auto tickThing = ticks >> cl;
                        if (thing.var_01 & (1 << 7))
                        {
                            auto pos = Map::TilePos2(session.getUnkPosition());
                            tickThing += pos.x * 5;
                            tickThing += pos.y * 3;
                        }
                        adjustedBuildingPart += al & tickThing;
                    }
                    else
                    {
                        if (!unkE0C3B0.empty())
                        {
                            auto tickThing = (ticks >> thing.var_01) & (unkE0C3B0.size() - 1);
                            adjustedBuildingPart += unkE0C3B0[tickThing];
                        }
                    }
                    const auto sectionHeight = indObj->buildingPartHeight[adjustedBuildingPart];
                    const uint32_t imageIdx = adjustedBuildingPart * 4 + indObj->var_12 + rotation;
                    ImageId image = baseColour.withIndex(imageIdx);
                    if (sectionCount == 0 && !baseColour.isBlended())
                    {
                        image = image.withNoiseMask((bh + 1) & 0x7);
                    }
                    session.addToPlotListAsChild(image, { 0, 0, height }, { -8, -8, bbZOffset }, { 38, 38, bbLengthZ });
                    height += sectionHeight;
                    sectionCount--;
                }
                if (totalSectionHeight != 0 && scaffSegType != 0xFF)
                {
                    const auto* scaffObj = ObjectManager::get<ScaffoldingObject>();
                    const auto segmentHeight = scaffObj->segmentHeights[scaffSegType];
                    const auto& scaffImages = getScaffoldingImages(scaffSegType).get2x2();
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
                    auto scaffImage = baseScaffImage.withIndexOffset(scaffImages.part1);
                    height = baseHeight;
                    for (int8_t remainingHeight = totalSectionHeight; remainingHeight > 0; remainingHeight -= segmentHeight, height += segmentHeight)
                    {
                        session.addToPlotListAsChild(scaffImage, { 0, 0, height }, { -8, -8, bbZOffset }, { 38, 38, bbLengthZ });
                    }
                    scaffImage = baseScaffImage.withIndexOffset(scaffImages.getRoof(rotation));
                    session.addToPlotListAsChild(scaffImage, { 0, 0, height }, { -8, -8, bbZOffset }, { 38, 38, bbLengthZ });
                }
            }
            session.setSegmentSupportHeight(Segment::all, 0xFFFF, 0);
            session.setGeneralSupportHeight(0xFFFF, 0); // TODO: Check if this works previously would not set slope to zero
        }
        else
        {
            const auto scaffSegType = indObj->scaffoldingSegmentType;
            if (totalSectionHeight != 0 && scaffSegType != 0xFF)
            {
                const auto* scaffObj = ObjectManager::get<ScaffoldingObject>();
                const auto segmentHeight = scaffObj->segmentHeights[scaffSegType];
                const auto& scaffImages = getScaffoldingImages(scaffSegType).get1x1();
                uint32_t scaffImageIdx = scaffObj->image + scaffImages.part0;
                ImageId scaffImage{};
                if (elIndustry.isGhost())
                {
                    scaffImage = Gfx::applyGhostToImage(scaffImageIdx);
                }
                else
                {
                    scaffImage = ImageId(scaffImageIdx, scaffoldingColour);
                }
                auto height = baseHeight;
                for (int8_t remainingHeight = totalSectionHeight; remainingHeight > 0; remainingHeight -= segmentHeight, height += segmentHeight)
                {
                    session.addToPlotListAsChild(scaffImage, { 16, 16, height }, { 3, 3, bbZOffset }, { 26, 26, bbLengthZ });
                }
            }

            int8_t sectionCount = bl;
            auto height = baseHeight;
            for (const auto buildingPart : buildingParts)
            {
                if (sectionCount == -1)
                {
                    break;
                }
                auto& thing = indObj->var_24[buildingPart];
                auto adjustedBuildingPart = buildingPart;
                if (thing.var_00)
                {
                    auto al = thing.var_00 - 1;
                    auto cl = thing.var_01 & 0x7F;
                    auto tickThing = ticks >> cl;
                    if (thing.var_01 & (1 << 7))
                    {
                        auto pos = Map::TilePos2(session.getUnkPosition());
                        tickThing += pos.x * 5;
                        tickThing += pos.y * 3;
                    }
                    adjustedBuildingPart += al & tickThing;
                }
                else
                {
                    if (!unkE0C3B0.empty())
                    {
                        auto tickThing = (ticks >> thing.var_01) & (unkE0C3B0.size() - 1);
                        adjustedBuildingPart += unkE0C3B0[tickThing];
                    }
                }
                const auto sectionHeight = indObj->buildingPartHeight[adjustedBuildingPart];
                const uint32_t imageIdx = adjustedBuildingPart * 4 + indObj->var_12 + rotation;
                ImageId image = baseColour.withIndex(imageIdx);
                if (sectionCount == 0 && !baseColour.isBlended())
                {
                    image = image.withNoiseMask((bh + 1) & 0x7);
                }
                session.addToPlotListAsChild(image, { 16, 16, height }, { 3, 3, bbZOffset }, { 26, 26, bbLengthZ });
                height += sectionHeight;
                sectionCount--;
            }
            if (totalSectionHeight != 0 && scaffSegType != 0xFF)
            {
                const auto* scaffObj = ObjectManager::get<ScaffoldingObject>();
                const auto segmentHeight = scaffObj->segmentHeights[scaffSegType];
                const auto& scaffImages = getScaffoldingImages(scaffSegType).get1x1();
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
                auto scaffImage = baseScaffImage.withIndexOffset(scaffImages.part1);
                height = baseHeight;
                for (int8_t remainingHeight = totalSectionHeight; remainingHeight > 0; remainingHeight -= segmentHeight, height += segmentHeight)
                {
                    session.addToPlotListAsChild(scaffImage, { 16, 16, height }, { 3, 3, bbZOffset }, { 26, 26, bbLengthZ });
                }
                scaffImage = baseScaffImage.withIndexOffset(scaffImages.getRoof(rotation));
                session.addToPlotListAsChild(scaffImage, { 16, 16, height }, { 3, 3, bbZOffset }, { 26, 26, bbLengthZ });
            }
        }
    }
}
