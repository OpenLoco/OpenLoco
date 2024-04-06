#include "PaintStation.h"
#include "Graphics/Colour.h"
#include "Map/StationElement.h"
#include "Map/TrackElement.h"
#include "Objects/CargoObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrainStationObject.h"
#include "Paint.h"
#include "Ui.h"
#include "Ui/ViewportInteraction.h"
#include "World/CompanyManager.h"
#include "World/Station.h"
#include "World/StationManager.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco::Paint
{
    // 0x0042F550
    static void paintStationCargo(PaintSession& session, const World::StationElement& elStation, const uint8_t flags, const uint32_t cargoTypes, const std::vector<TrainStationObject::CargoOffset>& cargoOffsets, const int16_t offsetZ, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
    {
        if (elStation.isGhost())
        {
            return;
        }
        if (session.getRenderTarget()->zoomLevel > 0)
        {
            return;
        }

        const auto* station = StationManager::get(elStation.stationId());
        const auto unkPosHash = (session.getUnkPosition().x + session.getUnkPosition().y) / 32;

        bool hasDrawn = false;
        uint8_t cargoId = 0;
        for (auto& cargoStat : station->cargoStats)
        {
            const auto* cargoObj = ObjectManager::get<CargoObject>(cargoId);
            if (!(cargoTypes & (1 << cargoId)))
            {
                cargoId++;
                continue;
            }
            cargoId++;
            if (cargoStat.densityPerTile == 0)
            {
                continue;
            }

            if (cargoObj->numPlatformVariations == 0)
            {
                continue;
            }

            uint32_t variation = cargoObj->numPlatformVariations - 1;
            variation &= unkPosHash;
            for (uint32_t i = 0; i < cargoStat.densityPerTile; ++i)
            {
                if (variation > cargoObj->numPlatformVariations)
                {
                    variation = 0;
                }
                // TODO: Investigate if updateCargoDistribution should cap to prevent this ever being hit
                if (i >= cargoOffsets.size())
                {
                    break;
                }

                if (hasDrawn)
                {
                    hasDrawn = true;
                    if (!(flags & (1 << 1)))
                    {
                        continue;
                    }
                }
                else
                {
                    hasDrawn = true;
                    if (!(flags & (1 << 0)))
                    {
                        continue;
                    }
                }
                World::Pos3 offset;
                if (cargoObj->hasFlags(CargoObjectFlags::unk0))
                {
                    if (flags & (1 << 2))
                    {
                        offset = cargoOffsets[i][1];
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    if (flags & (1 << 3))
                    {
                        offset = cargoOffsets[i][0];
                    }
                    else
                    {
                        continue;
                    }
                }
                const auto imageId = ImageId{ cargoObj->unitInlineSprite + Cargo::ImageIds::kStationPlatformBegin + variation };
                session.addToPlotListAsChild(imageId, offset + World::Pos3{ 0, 0, offsetZ }, boundBoxOffset, boundBoxSize);
            }
        }
    }

    // 0x004D79AC (0x00411456, 0x004115EB, 0x00411780, 0x00419915)
    static void paintTrainStationStyle0StraightTrack(PaintSession& session, const World::StationElement& elStation, const ImageId imageBase, const ImageId imageTranslucentBase)
    {
        const auto rotation = (session.getRotation() + elStation.rotation()) & 0x3;

        const auto* stationObj = ObjectManager::get<TrainStationObject>(elStation.objectId());
        // This was part of paintStationCargo
        const auto unkPosHash = (session.getUnkPosition().x + session.getUnkPosition().y) / 32;
        const auto cargoOffsets = stationObj->getCargoOffsets(rotation, unkPosHash & 0x3);

        constexpr std::array<std::array<uint8_t, 4>, 4> cargoRotationFlags = {
            std::array<uint8_t, 4>{ 0x9, 0x5, 0x6, 0xA },
            std::array<uint8_t, 4>{ 0x9, 0x5, 0x6, 0xA },
            std::array<uint8_t, 4>{ 0xA, 0x6, 0x5, 0x9 },
            std::array<uint8_t, 4>{ 0xA, 0x6, 0x5, 0x9 },
        };

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

        constexpr PlatformImage neStationPlatformImage = {
            {
                TrainStation::ImageIds::Style0::straightBackNE,
                { 2, 2, 8 },
                { 28, 4, 3 },
            },
            {
                TrainStation::ImageIds::Style0::straightFrontNE,
                { 2, 24, 8 },
                { 28, 4, 3 },
            },
            {
                TrainStation::ImageIds::Style0::straightCanopyNE,
                { 2, 2, 26 },
                { 28, 28, 1 },
            },
            {
                TrainStation::ImageIds::Style0::straightCanopyTranslucentNE,
                {},
                {},
            },
        };
        constexpr PlatformImage seStationPlatformImage = {
            {
                TrainStation::ImageIds::Style0::straightBackSE,
                { 2, 2, 8 },
                { 4, 28, 3 },
            },
            {
                TrainStation::ImageIds::Style0::straightFrontSE,
                { 24, 2, 8 },
                { 4, 28, 3 },
            },
            {
                TrainStation::ImageIds::Style0::straightCanopySE,
                { 2, 2, 26 },
                { 28, 28, 1 },
            },
            {
                TrainStation::ImageIds::Style0::straightCanopyTranslucentSE,
                {},
                {},
            },
        };

        constexpr std::array<PlatformImage, 4> stationPlatformImages = {
            neStationPlatformImage,
            seStationPlatformImage,
            neStationPlatformImage,
            seStationPlatformImage,
        };

        const auto& cargoFlags = cargoRotationFlags[rotation];
        const auto& platformImages = stationPlatformImages[rotation];

        const World::Pos3 heightOffset(0, 0, elStation.baseHeight());

        // Paint Back part of platform
        {
            World::Pos3 bbOffset = platformImages.back.bbOffset + heightOffset;
            session.addToPlotList4FD150(imageBase.withIndexOffset(platformImages.back.imageId), heightOffset, bbOffset, platformImages.back.bbSize);

            paintStationCargo(session, elStation, cargoFlags[0], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.back.bbSize);
            paintStationCargo(session, elStation, cargoFlags[1], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.back.bbSize);
        }
        // Paint Front part of platform
        {
            World::Pos3 bbOffset = platformImages.front.bbOffset + heightOffset;
            session.addToPlotList4FD150(imageBase.withIndexOffset(platformImages.front.imageId), heightOffset, bbOffset, platformImages.front.bbSize);

            paintStationCargo(session, elStation, cargoFlags[2], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.front.bbSize);
            paintStationCargo(session, elStation, cargoFlags[3], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.front.bbSize);
        }

        // Paint Canopy of platform
        {
            World::Pos3 bbOffset = platformImages.canopy.bbOffset + heightOffset;
            session.addToPlotListTrackRoadAddition(imageBase.withIndexOffset(platformImages.canopy.imageId), 1, heightOffset, bbOffset, platformImages.canopy.bbSize);
            session.attachToPrevious(imageTranslucentBase.withIndexOffset(platformImages.canopyTranslucent.imageId), { 0, 0 });
        }
        session.set525CF8(session.get525CF8() | SegmentFlags::all);
    }

    // 0x00411AC6
    static void paintTrainStationStyle0DiagonalTrack0NE(PaintSession& session, const World::StationElement& elStation, const ImageId imageBase, [[maybe_unused]] const ImageId imageTranslucentBase)
    {
        const World::Pos3 heightOffset(0, 0, elStation.baseHeight());
        World::Pos3 bbOffset = World::Pos3{ 2, 2, 8 } + heightOffset;
        World::Pos3 bbSize = World::Pos3{ 2, 2, 3 };
        session.addToPlotList4FD150(imageBase.withIndexOffset(TrainStation::ImageIds::Style0::diagonalNE0), heightOffset, bbOffset, bbSize);
    }

    // 0x00411B09
    static void paintTrainStationStyle0DiagonalTrack1NE(PaintSession& session, const World::StationElement& elStation, const ImageId imageBase, const ImageId imageTranslucentBase)
    {
        const World::Pos3 heightOffset(0, 0, elStation.baseHeight());
        // Platform
        {
            World::Pos3 bbOffset = World::Pos3{ 6, 6, 8 } + heightOffset;
            World::Pos3 bbSize = World::Pos3{ 2, 2, 11 };
            session.addToPlotList4FD150(imageBase.withIndexOffset(TrainStation::ImageIds::Style0::diagonalNE1), heightOffset, bbOffset, bbSize);
        }
        // Canopy
        {
            World::Pos3 bbOffset = World::Pos3{ 6, 6, 26 } + heightOffset;
            World::Pos3 bbSize = World::Pos3{ 2, 2, 1 };
            session.addToPlotList4FD150(imageBase.withIndexOffset(TrainStation::ImageIds::Style0::diagonalCanopyNE1), heightOffset, bbOffset, bbSize);
            session.attachToPrevious(imageTranslucentBase.withIndexOffset(TrainStation::ImageIds::Style0::diagonalCanopyTranslucentNE1), { 0, 0 });
        }
    }

    // 0x00411BA6
    static void paintTrainStationStyle0DiagonalTrack2NE([[maybe_unused]] PaintSession& session, [[maybe_unused]] const World::StationElement& elStation, [[maybe_unused]] const ImageId imageBase, [[maybe_unused]] const ImageId imageTranslucentBase)
    {
        return;
    }

    // 0x00411BA8
    static void paintTrainStationStyle0DiagonalTrack3NE(PaintSession& session, const World::StationElement& elStation, const ImageId imageBase, [[maybe_unused]] const ImageId imageTranslucentBase)
    {
        const World::Pos3 heightOffset(0, 0, elStation.baseHeight());
        World::Pos3 bbOffset = World::Pos3{ 2, 2, 8 } + heightOffset;
        World::Pos3 bbSize = World::Pos3{ 2, 2, 3 };
        session.addToPlotList4FD150(imageBase.withIndexOffset(TrainStation::ImageIds::Style0::diagonalNE3), heightOffset, bbOffset, bbSize);
    }

    // 0x00411BEB
    static void paintTrainStationStyle0DiagonalTrack0SE([[maybe_unused]] PaintSession& session, [[maybe_unused]] const World::StationElement& elStation, [[maybe_unused]] const ImageId imageBase, [[maybe_unused]] const ImageId imageTranslucentBase)
    {
        return;
    }

    // 0x00411BED
    static void paintTrainStationStyle0DiagonalTrack1SE(PaintSession& session, const World::StationElement& elStation, const ImageId imageBase, [[maybe_unused]] const ImageId imageTranslucentBase)
    {
        const World::Pos3 heightOffset(0, 0, elStation.baseHeight());
        World::Pos3 bbOffset = World::Pos3{ 28, 34, 8 } + heightOffset;
        World::Pos3 bbSize = World::Pos3{ 2, 2, 3 };
        session.addToPlotList4FD150(imageBase.withIndexOffset(TrainStation::ImageIds::Style0::diagonalSE1), heightOffset, bbOffset, bbSize);
    }

    // 0x00411C30
    static void paintTrainStationStyle0DiagonalTrack2SE(PaintSession& session, const World::StationElement& elStation, const ImageId imageBase, [[maybe_unused]] const ImageId imageTranslucentBase)
    {
        const World::Pos3 heightOffset(0, 0, elStation.baseHeight());
        World::Pos3 bbOffset = World::Pos3{ 34, 28, 8 } + heightOffset;
        World::Pos3 bbSize = World::Pos3{ 2, 2, 3 };
        session.addToPlotList4FD150(imageBase.withIndexOffset(TrainStation::ImageIds::Style0::diagonalSE2), heightOffset, bbOffset, bbSize);
    }

    // 0x00411C73
    static void paintTrainStationStyle0DiagonalTrack3SE(PaintSession& session, const World::StationElement& elStation, const ImageId imageBase, const ImageId imageTranslucentBase)
    {
        const World::Pos3 heightOffset(0, 0, elStation.baseHeight());
        World::Pos3 bbOffset = World::Pos3{ 0, 0, 26 } + heightOffset;
        World::Pos3 bbSize = World::Pos3{ 30, 30, 1 };
        session.addToPlotListTrackRoadAddition(imageBase.withIndexOffset(TrainStation::ImageIds::Style0::diagonalSE3), 1, heightOffset, bbOffset, bbSize);
        session.attachToPrevious(imageTranslucentBase.withIndexOffset(TrainStation::ImageIds::Style0::diagonalCanopyTranslucentSE3), { 0, 0 });
    }

    // 0x004D7A5C
    static void paintTrainStationStyle0DiagonalTrack(PaintSession& session, const World::StationElement& elStation, const ImageId imageBase, const ImageId imageTranslucentBase)
    {
        const auto* elTrack = elStation.prev()->as<World::TrackElement>();
        if (elTrack == nullptr)
        {
            return;
        }
        const auto rotation = (session.getRotation() + elStation.rotation()) & 0x3;
        const auto seqIndex = rotation & (1 << 1) ? 3 - elTrack->sequenceIndex() : elTrack->sequenceIndex();

        if (rotation & (1 << 0))
        {
            switch (seqIndex)
            {
                case 0:
                    paintTrainStationStyle0DiagonalTrack0SE(session, elStation, imageBase, imageTranslucentBase);
                    break;
                case 1:
                    paintTrainStationStyle0DiagonalTrack1SE(session, elStation, imageBase, imageTranslucentBase);
                    break;
                case 2:
                    paintTrainStationStyle0DiagonalTrack2SE(session, elStation, imageBase, imageTranslucentBase);
                    break;
                case 3:
                    paintTrainStationStyle0DiagonalTrack3SE(session, elStation, imageBase, imageTranslucentBase);
                    break;
            }
        }
        else
        {
            switch (seqIndex)
            {
                case 0:
                    paintTrainStationStyle0DiagonalTrack0NE(session, elStation, imageBase, imageTranslucentBase);
                    break;
                case 1:
                    paintTrainStationStyle0DiagonalTrack1NE(session, elStation, imageBase, imageTranslucentBase);
                    break;
                case 2:
                    paintTrainStationStyle0DiagonalTrack2NE(session, elStation, imageBase, imageTranslucentBase);
                    break;
                case 3:
                    paintTrainStationStyle0DiagonalTrack3NE(session, elStation, imageBase, imageTranslucentBase);
                    break;
            }
        }
    }

    // 0x004D78EC
    static void paintTrainStationStyle0(PaintSession& session, const World::StationElement& elStation, const uint8_t trackId, [[maybe_unused]] const uint8_t sequenceIndex, const ImageId imageBase, const ImageId imageGlassBase)
    {
        switch (trackId)
        {
            case 0:
                paintTrainStationStyle0StraightTrack(session, elStation, imageBase, imageGlassBase);
                break;
            case 1:
                // Vanilla had hard to reach code for diagonal train stations
                paintTrainStationStyle0DiagonalTrack(session, elStation, imageBase, imageGlassBase);
                break;
            default:
                return;
        }
    }

    // 0x0048B34D
    static void paintTrainStation(PaintSession& session, const World::StationElement& elStation)
    {
        session.setItemType(Ui::ViewportInteraction::InteractionItem::trainStation);

        const auto* stationObj = ObjectManager::get<TrainStationObject>(elStation.objectId());
        session.setOccupiedAdditionSupportSegments(SegmentFlags::all);

        const auto* elTrack = elStation.prev()->as<World::TrackElement>();
        if (elTrack == nullptr)
        {
            return;
        }
        const auto companyColour = CompanyManager::getCompanyColour(elTrack->owner());
        auto translucentColour = Colours::getTranslucent(companyColour);
        if (!stationObj->hasFlags(TrainStationFlags::recolourable))
        {
            translucentColour = ExtColour::unk2E;
        }

        ImageId imageIdbase{};            // 0x0112C720
        ImageId imageIdTranslucentBase{}; // 0x0112C724

        if (elStation.isGhost())
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            imageIdbase = Gfx::applyGhostToImage(stationObj->var_12[elStation.multiTileIndex()]);
            imageIdTranslucentBase = ImageId{ stationObj->var_12[elStation.multiTileIndex()] }.withTranslucency(ExtColour::unk2F);
        }
        else
        {
            imageIdbase = ImageId{ stationObj->var_12[elStation.multiTileIndex()], companyColour };
            imageIdTranslucentBase = ImageId{ stationObj->var_12[elStation.multiTileIndex()] }.withTranslucency(translucentColour);
        }

        switch (stationObj->drawStyle)
        {
            case 0:
                paintTrainStationStyle0(session, elStation, elTrack->trackId(), elTrack->sequenceIndex(), imageIdbase, imageIdTranslucentBase);
                break;
            default:
                // Track only have 1 style of drawing
                break;
        }
    }

    // 0x0048B403
    static void paintRoadStation(PaintSession& session, const World::StationElement& elStation)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(&elStation);
        regs.ecx = (session.getRotation() + elStation.rotation()) & 0x3;
        regs.dx = elStation.baseHeight();
        regs.bl = elStation.objectId();
        Interop::call(0x0048B403, regs);
    }

    // 0x0048B4D0
    static void paintAirport(PaintSession& session, const World::StationElement& elStation)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(&elStation);
        regs.ecx = (session.getRotation() + elStation.rotation()) & 0x3;
        regs.dx = elStation.baseHeight();
        regs.bl = elStation.objectId();
        Interop::call(0x0048B4D0, regs);
    }

    // 0x0048B86E
    static void paintDocks(PaintSession& session, const World::StationElement& elStation)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(&elStation);
        regs.ecx = (session.getRotation() + elStation.rotation()) & 0x3;
        regs.dx = elStation.baseHeight();
        regs.bl = elStation.objectId();
        Interop::call(0x0048B86E, regs);
    }

    // 0x0048B313
    void paintStation(PaintSession& session, const World::StationElement& elStation)
    {
        if (elStation.isAiAllocated())
        {
            return;
        }

        if (elStation.isGhost()
            && CompanyManager::getSecondaryPlayerId() != CompanyId::null
            && CompanyManager::getSecondaryPlayerId() == elStation.owner())
        {
            return;
        }

        switch (elStation.stationType())
        {
            case StationType::trainStation:
                paintTrainStation(session, elStation);
                break;
            case StationType::roadStation:
                paintRoadStation(session, elStation);
                break;
            case StationType::airport:
                paintAirport(session, elStation);
                break;
            case StationType::docks:
                paintDocks(session, elStation);
                break;
        }
    }
}
