#include "PaintRoadStation.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/TileElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadStationObject.h"
#include "Paint.h"
#include "PaintStation.h"
#include "Ui/ViewportInteraction.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco::Paint
{
    // THIS IS MOSTLY A SIMPLIFIED COPY OF TRAIN STATION PAINT

    struct PaintDetail
    {
        uint32_t imageId;
        World::Pos3 bbOffset;
        World::Pos3 bbSize;
    };
    struct PlatformImage
    {
        PaintDetail back;
        PaintDetail front;
        PaintDetail canopy;
        PaintDetail canopyTranslucent;
    };

    constexpr PlatformImage kNeStationPlatformImage = {
        {
            RoadStation::ImageIds::Style0::straightBackNE,
            { 2, 2, 8 },
            { 28, 4, 3 },
        },
        {
            RoadStation::ImageIds::Style0::straightFrontNE,
            { 2, 24, 8 },
            { 28, 4, 3 },
        },
        {
            RoadStation::ImageIds::Style0::straightCanopyNE,
            { 2, 2, 26 },
            { 28, 28, 1 },
        },
        {
            RoadStation::ImageIds::Style0::straightCanopyTranslucentNE,
            {},
            {},
        },
    };
    constexpr PlatformImage kSeStationPlatformImage = {
        {
            RoadStation::ImageIds::Style0::straightBackSE,
            { 2, 2, 8 },
            { 4, 28, 3 },
        },
        {
            RoadStation::ImageIds::Style0::straightFrontSE,
            { 24, 2, 8 },
            { 4, 28, 3 },
        },
        {
            RoadStation::ImageIds::Style0::straightCanopySE,
            { 2, 2, 26 },
            { 28, 28, 1 },
        },
        {
            RoadStation::ImageIds::Style0::straightCanopyTranslucentSE,
            {},
            {},
        },
    };

    constexpr std::array<PlatformImage, 4> kStationPlatformImages = {
        kNeStationPlatformImage,
        kSeStationPlatformImage,
        kNeStationPlatformImage,
        kSeStationPlatformImage,
    };

    // 0x004D7CA4 (0x00411EDA, 0x00412099, 0x00412258, 0x00412417)
    static void paintRoadStationStyle0StraightTrack(PaintSession& session, const World::StationElement& elStation, const ImageId imageBase, const ImageId imageTranslucentBase)
    {
        const auto rotation = (session.getRotation() + elStation.rotation()) & 0x3;
        const auto* stationObj = ObjectManager::get<RoadStationObject>(elStation.objectId());
        // This was part of paintStationCargo
        const auto unkPosHash = (session.getUnkPosition().x + session.getUnkPosition().y) / 32;
        const auto cargoOffsets = stationObj->getCargoOffsets(rotation, unkPosHash & 0x3);

        constexpr std::array<std::array<uint8_t, 4>, 4> cargoRotationFlags = {
            std::array<uint8_t, 4>{ 0x9, 0x5, 0x6, 0xA },
            std::array<uint8_t, 4>{ 0x9, 0x5, 0x6, 0xA },
            std::array<uint8_t, 4>{ 0xA, 0x6, 0x5, 0x9 },
            std::array<uint8_t, 4>{ 0xA, 0x6, 0x5, 0x9 },
        };

        const auto& cargoFlags = cargoRotationFlags[rotation];
        const auto& platformImages = kStationPlatformImages[rotation];

        const World::Pos3 heightOffset(0, 0, elStation.baseHeight());

        uint32_t cargoTypeFlags = 0xFFFFFFFFU;
        if (stationObj->hasFlags(RoadStationFlags::freight | RoadStationFlags::passenger))
        {
            cargoTypeFlags &= ~(1U << stationObj->cargoType);
            if (!stationObj->hasFlags(RoadStationFlags::freight))
            {
                cargoTypeFlags = ~cargoTypeFlags;
            }
        }

        // Paint Back part of platform
        {
            World::Pos3 bbOffset = platformImages.back.bbOffset + heightOffset;
            session.addToPlotList4FD150(imageBase.withIndexOffset(platformImages.back.imageId), heightOffset, bbOffset, platformImages.back.bbSize);
            paintStationCargo(session, elStation, cargoFlags[0], cargoTypeFlags, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.back.bbSize);
            paintStationCargo(session, elStation, cargoFlags[1], cargoTypeFlags, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.back.bbSize);
        }

        // Paint Front part of platform
        {
            World::Pos3 bbOffset = platformImages.front.bbOffset + heightOffset;
            session.addToPlotList4FD150(imageBase.withIndexOffset(platformImages.front.imageId), heightOffset, bbOffset, platformImages.front.bbSize);

            paintStationCargo(session, elStation, cargoFlags[2], cargoTypeFlags, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.front.bbSize);
            paintStationCargo(session, elStation, cargoFlags[3], cargoTypeFlags, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.front.bbSize);
        }

        // Paint Canopy of platform
        {
            World::Pos3 bbOffset = platformImages.canopy.bbOffset + heightOffset;
            session.addToPlotListTrackRoadAddition(imageBase.withIndexOffset(platformImages.canopy.imageId), 1, heightOffset, bbOffset, platformImages.canopy.bbSize);
            session.attachToPrevious(imageTranslucentBase.withIndexOffset(platformImages.canopyTranslucent.imageId), { 0, 0 });
        }
        session.set525CF8(session.get525CF8() | SegmentFlags::all);
    }

    // 0x004D7C6C
    static void paintRoadStationStyle0(PaintSession& session, const World::StationElement& elStation, const uint8_t roadId, [[maybe_unused]] const uint8_t sequenceIndex, const ImageId imageBase, const ImageId imageGlassBase)
    {
        switch (roadId)
        {
            case 0:
                paintRoadStationStyle0StraightTrack(session, elStation, imageBase, imageGlassBase);
                break;
            default:
                return;
        }
    }

    // 0x0048B403
    void paintRoadStation(PaintSession& session, const World::StationElement& elStation)
    {
        session.setItemType(Ui::ViewportInteraction::InteractionItem::roadStation);

        const auto* stationObj = ObjectManager::get<RoadStationObject>(elStation.objectId());
        session.setOccupiedAdditionSupportSegments(SegmentFlags::all);

        const auto* elRoad = elStation.prev()->as<World::RoadElement>();
        if (elRoad == nullptr)
        {
            return;
        }
        auto* station = StationManager::get(elStation.stationId());
        const auto companyColour = CompanyManager::getCompanyColour(station->owner);
        auto translucentColour = Colours::getTranslucent(companyColour);
        if (!stationObj->hasFlags(RoadStationFlags::recolourable))
        {
            translucentColour = ExtColour::unk2E;
        }

        ImageId imageIdbase{};            // 0x0112C720
        ImageId imageIdTranslucentBase{}; // 0x0112C724

        if (elStation.isGhost())
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            imageIdbase = Gfx::applyGhostToImage(stationObj->var_10[1]);
            imageIdTranslucentBase = ImageId{ stationObj->var_10[1] }.withTranslucency(ExtColour::unk2F);
        }
        else
        {
            imageIdbase = ImageId{ stationObj->var_10[1], companyColour };
            imageIdTranslucentBase = ImageId{ stationObj->var_10[1] }.withTranslucency(translucentColour);
        }

        switch (stationObj->paintStyle)
        {
            case 0:
                paintRoadStationStyle0(session, elStation, elRoad->roadId(), elRoad->sequenceIndex(), imageIdbase, imageIdTranslucentBase);
                break;
            default:
                // Road only have 1 style of drawing
                break;
        }
    }
}
