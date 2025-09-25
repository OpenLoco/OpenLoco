#include "PaintAirport.h"
#include "Graphics/RenderTarget.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Objects/AirportObject.h"
#include "Objects/ObjectManager.h"
#include "Paint.h"
#include "ScenarioManager.h"
#include "Ui/ViewportInteraction.h"
#include "World/CompanyManager.h"

namespace OpenLoco::Paint
{
    // TODO: Docks, Airport, Building, Industry all have very similar code...

    constexpr World::Pos3 kImageOffsetBase1x1 = { 16, 16, 0 };
    constexpr World::Pos3 kImageOffsetBase2x2 = { 0, 0, 0 };
    constexpr World::Pos3 kBBOffsetBase1x1 = { 3, 3, 0 };
    constexpr World::Pos3 kBBOffsetBase2x2 = { -8, -8, 0 };
    constexpr World::Pos3 kBBSizeBase1x1 = { 26, 26, 0 };
    constexpr World::Pos3 kBBSizeBase2x2 = { 38, 38, 0 };

    static void paintAirportBuilding(PaintSession& session, const World::StationElement& elStation, const AirportObject& airportObj, const World::Pos3& imageOffset, const World::Pos3& bbOffset, const World::Pos3& bbSize, const ImageId& baseColour, const uint8_t rotation)
    {
        // 0xE0C3A0
        const auto ticks = ScenarioManager::getScenarioTicks();

        const auto variation = elStation.buildingType();
        const auto parts = airportObj.getBuildingParts(variation);
        const auto heights = airportObj.getBuildingPartHeights();
        const auto animations = airportObj.getBuildingPartAnimations();

        auto sectionImageOffset = imageOffset;
        for (const auto part : parts)
        {
            const auto partAnimation = animations[part];

            auto frameMask = partAnimation.numFrames - 1;
            auto cl = partAnimation.animationSpeed & 0x7F;
            auto tickThing = ticks >> cl;
            if (partAnimation.animationSpeed & (1 << 7))
            {
                auto pos = World::toTileSpace(session.getUnkPosition());
                tickThing += pos.x * 5;
                tickThing += pos.y * 3;
            }
            const auto adjustedPart = part + (frameMask & tickThing);

            const auto sectionHeight = heights[adjustedPart];
            const uint32_t imageIdx = adjustedPart * 4 + airportObj.buildingImage + rotation;
            ImageId image = baseColour.withIndex(imageIdx);
            session.addToPlotListAsChild(image, sectionImageOffset, bbOffset, bbSize);
            sectionImageOffset.z += sectionHeight;
        }
    }

    // 0x0048B4D0
    void paintAirport(PaintSession& session, const World::StationElement& elStation)
    {
        session.setItemType(Ui::ViewportInteraction::InteractionItem::airport);
        auto* airportObj = ObjectManager::get<AirportObject>(elStation.objectId());

        const auto companyColour = CompanyManager::getCompanyColour(elStation.owner());

        ImageId baseColour(0, companyColour);
        if (elStation.isGhost())
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            baseColour = Gfx::applyGhostToImage(0);
        }
        // Combine this with any imageId
        const uint8_t rotation = (session.getRotation() + elStation.rotation()) & 0x3;

        const auto variation = elStation.buildingType();

        // Odd... different to industry, building, dock
        auto clearHeight = 0;
        const auto heights = airportObj->getBuildingPartHeights();
        for (auto part : airportObj->getBuildingParts(variation))
        {
            clearHeight += heights[part];
        }
        // ceil to 4
        clearHeight += 3;
        clearHeight &= ~3;
        const int16_t bbLengthZ = std::min(clearHeight, 128) - 2;

        const auto baseHeight = elStation.baseHeight();

        bool isMultiTile = airportObj->largeTiles & (1U << variation);

        // Note: Image offsets will change as you move up the building but bboffset/size does not
        const World::Pos3 imageOffset = (isMultiTile ? kImageOffsetBase2x2 : kImageOffsetBase1x1) + World::Pos3{ 0, 0, baseHeight };
        const World::Pos3 bbOffset = (isMultiTile ? kBBOffsetBase2x2 : kBBOffsetBase1x1) + World::Pos3{ 0, 0, baseHeight };
        const World::Pos3 bbSize = (isMultiTile ? kBBSizeBase2x2 : kBBSizeBase1x1) + World::Pos3{ 0, 0, bbLengthZ };

        session.resetLastPS(); // Odd...
        if (airportObj->hasFlags(AirportObjectFlags::hasShadows))
        {
            if (session.getRenderTarget()->zoomLevel <= 1)
            {
                const auto shadowImageOffset = variation * 4 + airportObj->image + rotation + 1;
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
            const auto sequenceIndex = elStation.sequenceIndex();
            // Only the front of the 2x2 area will draw. Images are sized to overlap into the other tiles.
            if ((sequenceIndex ^ (1 << 1)) == ((-session.getRotation()) & 0x3))
            {
                paintAirportBuilding(session, elStation, *airportObj, imageOffset, bbOffset, bbSize, baseColour, rotation);
            }
            session.setSegmentsSupportHeight(SegmentFlags::all, 0xFFFF, 0);
            session.setGeneralSupportHeight(0xFFFF, 0); // TODO: Check if this works previously would not set slope to zero
        }
        else
        {
            paintAirportBuilding(session, elStation, *airportObj, imageOffset, bbOffset, bbSize, baseColour, rotation);
        }
    }
}
