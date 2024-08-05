#include "PaintDocks.h"
#include "Graphics/RenderTarget.h"
#include "Map/StationElement.h"
#include "Objects/DockObject.h"
#include "Objects/ObjectManager.h"
#include "Paint.h"
#include "ScenarioManager.h"
#include "Ui/ViewportInteraction.h"
#include "World/CompanyManager.h"

namespace OpenLoco::Paint
{
    // TODO: Docks, Airport, Building, Industry all have very similar code...

    constexpr World::Pos3 kImageOffsetBase2x2 = { 0, 0, 0 };
    constexpr World::Pos3 kBBOffsetBase2x2 = { -8, -8, 0 };
    constexpr World::Pos3 kBBSizeBase2x2 = { 38, 38, 0 };

    static void paintDocksBuilding(PaintSession& session, const World::StationElement& elStation, const DockObject& dockObj, const World::Pos3& imageOffset, const World::Pos3& bbOffset, const World::Pos3& bbSize, const ImageId& baseColour, const uint8_t rotation)
    {
        const auto ticks = ScenarioManager::getScenarioTicks();
        const auto variation = elStation.buildingType();
        const auto parts = dockObj.getBuildingParts(variation);

        auto sectionImageOffset = imageOffset;
        for (const auto part : parts)
        {
            const auto partAnimation = dockObj.buildingPartAnimations[part];

            const auto frameMask = partAnimation.numFrames - 1;
            const auto cl = partAnimation.animationSpeed & 0x7F;
            auto tickThing = ticks >> cl;
            if (partAnimation.animationSpeed & (1 << 7))
            {
                const auto pos = World::toTileSpace(session.getUnkPosition());
                tickThing += pos.x * 5;
                tickThing += pos.y * 3;
            }
            const auto adjustedPart = part + (frameMask & tickThing);

            const auto sectionHeight = dockObj.partHeights[adjustedPart];
            const uint32_t imageIdx = adjustedPart * 4 + dockObj.buildingImage + rotation;
            const auto image = baseColour.withIndex(imageIdx);
            session.addToPlotListAsChild(image, sectionImageOffset, bbOffset, bbSize);
            sectionImageOffset.z += sectionHeight;
        }
    }

    // 0x0048B86E
    void paintDocks(PaintSession& session, const World::StationElement& elStation)
    {
        session.setItemType(Ui::ViewportInteraction::InteractionItem::dock);

        auto* dockObj = ObjectManager::get<DockObject>(elStation.objectId());

        const auto companyColour = CompanyManager::getCompanyColour(elStation.owner());

        ImageId baseColour(0, companyColour);
        if (elStation.isGhost())
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            baseColour = Gfx::applyGhostToImage(0);
        }

        // Combine this with any imageId
        const auto rotation = (session.getRotation() + elStation.rotation()) & 0x3;

        const int16_t bbLengthZ = std::min(elStation.clearHeight() - elStation.baseHeight(), 128) - 2;

        const auto baseHeight = elStation.baseHeight();

        const auto variation = elStation.buildingType();

        // Note: Image offsets will change as you move up the building but bboffset/size does not
        const World::Pos3 imageOffset = kImageOffsetBase2x2 + World::Pos3{ 0, 0, baseHeight };
        const World::Pos3 bbOffset = kBBOffsetBase2x2 + World::Pos3{ 0, 0, baseHeight };
        const World::Pos3 bbSize = kBBSizeBase2x2 + World::Pos3{ 0, 0, bbLengthZ };

        session.resetLastPS(); // Odd...
        if (dockObj->hasFlags(DockObjectFlags::hasShadows))
        {
            if (session.getRenderTarget()->zoomLevel <= 1)
            {
                const auto shadowImageOffset = variation * 4 + dockObj->image + rotation + 1;
                const ImageId shadowImage = ImageId(shadowImageOffset).withTranslucency(Colours::getShadow(Colour::orange));

                session.addToPlotListAsChild(shadowImage, imageOffset, bbOffset, bbSize);
            }
        }

        const auto sequenceIndex = elStation.sequenceIndex();
        // Only the front of the 2x2 area will draw. Images are sized to overlap into the other tiles.
        if ((sequenceIndex ^ (1 << 1)) == ((-session.getRotation()) & 0x3))
        {
            paintDocksBuilding(session, elStation, *dockObj, imageOffset, bbOffset, bbSize, baseColour, rotation);
        }
        session.setSegmentsSupportHeight(SegmentFlags::all, 0xFFFF, 0);
        session.setGeneralSupportHeight(0xFFFF, 0); // TODO: Check if this works previously would not set slope to zero
    }
}
