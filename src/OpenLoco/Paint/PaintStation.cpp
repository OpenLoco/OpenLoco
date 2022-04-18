#include "PaintStation.h"
#include "../CompanyManager.h"
#include "../Graphics/Colour.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../Objects/CargoObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/TrainStationObject.h"
#include "../Station.h"
#include "../StationManager.h"
#include "../Ui.h"
#include "Paint.h"

namespace OpenLoco::Paint
{
    // 0x0042F550
    static void paintStationCargo(PaintSession& session, const Map::StationElement& elStation, const uint8_t flags, const uint32_t cargoTypes, const std::vector<TrainStationObject::CargoOffset>& cargoOffsets, const int16_t offsetZ, const Map::Pos3& boundBoxOffset, const Map::Pos3& boundBoxSize)
    {
        if (elStation.isGhost())
        {
            return;
        }
        if (session.getContext()->zoom_level > 0)
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
                if (i > cargoOffsets.size())
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
                Map::Pos3 offset;
                if (cargoObj->flags & CargoObjectFlags::unk0)
                {
                    if (flags & (1 << 2))
                    {
                        offset = cargoOffsets[i].offsets[1];
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
                        offset = cargoOffsets[i].offsets[0];
                    }
                    else
                    {
                        continue;
                    }
                }
                const auto imageId = cargoObj->unit_inline_sprite + Cargo::ImageIds::stationPlatformBegin + variation;
                session.addToPlotList4FD1E0(imageId, offset + Map::Pos3{ 0, 0, offsetZ }, boundBoxOffset, boundBoxSize);
            }
        }
    }

    // 0x004D79AC (0x00411456, 0x004115EB, 0x00411780, 0x00419915)
    static void paintTrainStationStyle0StraightTrack(PaintSession& session, const Map::StationElement& elStation, const uint32_t imageBase, const uint32_t imageTranslucentBase)
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
            Map::Pos3 bbOffset;
            Map::Pos3 bbSize;
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

        const Map::Pos3 heightOffset(0, 0, elStation.baseHeight());

        // Paint Back part of platform
        {
            Map::Pos3 bbOffset = platformImages.back.bbOffset + heightOffset;
            session.addToPlotList4FD150(imageBase + platformImages.back.imageId, heightOffset, bbOffset, platformImages.back.bbSize);

            paintStationCargo(session, elStation, cargoFlags[0], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.back.bbSize);
            paintStationCargo(session, elStation, cargoFlags[1], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.back.bbSize);
        }
        // Paint Front part of platform
        {
            Map::Pos3 bbOffset = platformImages.front.bbOffset + heightOffset;
            session.addToPlotList4FD150(imageBase + platformImages.front.imageId, heightOffset, bbOffset, platformImages.front.bbSize);

            paintStationCargo(session, elStation, cargoFlags[2], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.front.bbSize);
            paintStationCargo(session, elStation, cargoFlags[3], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.front.bbSize);
        }

        // Paint Canopy of platform
        {
            Map::Pos3 bbOffset = platformImages.canopy.bbOffset + heightOffset;
            session.addToPlotList4FD180(imageBase + platformImages.canopy.imageId, 1, heightOffset, bbOffset, platformImages.canopy.bbSize);
            session.attachToPrevious(imageTranslucentBase + platformImages.canopyTranslucent.imageId, { 0, 0 });
        }
        session.set525CF8(session.get525CF8() | 0x1FF);
    }

    // 0x00411AC6
    static void paintTrainStationStyle0DiagonalTrack0NE(PaintSession& session, const Map::StationElement& elStation, const uint32_t imageBase, const uint32_t imageTranslucentBase)
    {
        const Map::Pos3 heightOffset(0, 0, elStation.baseZ() * 4);
        Map::Pos3 bbOffset = Map::Pos3{ 2, 2, 8 } + heightOffset;
        Map::Pos3 bbSize = Map::Pos3{ 2, 2, 3 };
        session.addToPlotList4FD150(imageBase + TrainStation::ImageIds::Style0::diagonalNE0, heightOffset, bbOffset, bbSize);
    }

    // 0x00411B09
    static void paintTrainStationStyle0DiagonalTrack1NE(PaintSession& session, const Map::StationElement& elStation, const uint32_t imageBase, const uint32_t imageTranslucentBase)
    {
        const Map::Pos3 heightOffset(0, 0, elStation.baseZ() * 4);
        // Platform
        {
            Map::Pos3 bbOffset = Map::Pos3{ 6, 6, 8 } + heightOffset;
            Map::Pos3 bbSize = Map::Pos3{ 2, 2, 11 };
            session.addToPlotList4FD150(imageBase + TrainStation::ImageIds::Style0::diagonalNE1, heightOffset, bbOffset, bbSize);
        }
        // Canopy
        {
            Map::Pos3 bbOffset = Map::Pos3{ 6, 6, 26 } + heightOffset;
            Map::Pos3 bbSize = Map::Pos3{ 2, 2, 1 };
            session.addToPlotList4FD150(imageBase + TrainStation::ImageIds::Style0::diagonalCanopyNE1, heightOffset, bbOffset, bbSize);
            session.attachToPrevious(imageTranslucentBase + TrainStation::ImageIds::Style0::diagonalCanopyTranslucentNE1, { 0, 0 });
        }
    }

    // 0x00411BA6
    static void paintTrainStationStyle0DiagonalTrack2NE(PaintSession& session, const Map::StationElement& elStation, const uint32_t imageBase, const uint32_t imageTranslucentBase)
    {
        return;
    }

    // 0x00411BA8
    static void paintTrainStationStyle0DiagonalTrack3NE(PaintSession& session, const Map::StationElement& elStation, const uint32_t imageBase, const uint32_t imageTranslucentBase)
    {
        const Map::Pos3 heightOffset(0, 0, elStation.baseZ() * 4);
        Map::Pos3 bbOffset = Map::Pos3{ 2, 2, 8 } + heightOffset;
        Map::Pos3 bbSize = Map::Pos3{ 2, 2, 3 };
        session.addToPlotList4FD150(imageBase + TrainStation::ImageIds::Style0::diagonalNE3, heightOffset, bbOffset, bbSize);
    }

    // 0x00411BEB
    static void paintTrainStationStyle0DiagonalTrack0SE(PaintSession& session, const Map::StationElement& elStation, const uint32_t imageBase, const uint32_t imageTranslucentBase)
    {
        return;
    }

    // 0x00411BED
    static void paintTrainStationStyle0DiagonalTrack1SE(PaintSession& session, const Map::StationElement& elStation, const uint32_t imageBase, const uint32_t imageTranslucentBase)
    {
        const Map::Pos3 heightOffset(0, 0, elStation.baseZ() * 4);
        Map::Pos3 bbOffset = Map::Pos3{ 28, 34, 8 } + heightOffset;
        Map::Pos3 bbSize = Map::Pos3{ 2, 2, 3 };
        session.addToPlotList4FD150(imageBase + TrainStation::ImageIds::Style0::diagonalSE1, heightOffset, bbOffset, bbSize);
    }

    // 0x00411C30
    static void paintTrainStationStyle0DiagonalTrack2SE(PaintSession& session, const Map::StationElement& elStation, const uint32_t imageBase, const uint32_t imageTranslucentBase)
    {
        const Map::Pos3 heightOffset(0, 0, elStation.baseZ() * 4);
        Map::Pos3 bbOffset = Map::Pos3{ 34, 28, 8 } + heightOffset;
        Map::Pos3 bbSize = Map::Pos3{ 2, 2, 3 };
        session.addToPlotList4FD150(imageBase + TrainStation::ImageIds::Style0::diagonalSE2, heightOffset, bbOffset, bbSize);
    }

    // 0x00411C73
    static void paintTrainStationStyle0DiagonalTrack3SE(PaintSession& session, const Map::StationElement& elStation, const uint32_t imageBase, const uint32_t imageTranslucentBase)
    {
        const Map::Pos3 heightOffset(0, 0, elStation.baseZ() * 4);
        Map::Pos3 bbOffset = Map::Pos3{ 0, 0, 26 } + heightOffset;
        Map::Pos3 bbSize = Map::Pos3{ 30, 30, 1 };
        session.addToPlotList4FD180(imageBase + TrainStation::ImageIds::Style0::diagonalSE3, 1, heightOffset, bbOffset, bbSize);
        session.attachToPrevious(imageTranslucentBase + TrainStation::ImageIds::Style0::diagonalCanopyTranslucentSE3, { 0, 0 });
    }

    // 0x004D7A5C
    static void paintTrainStationStyle0DiagonalTrack(PaintSession& session, const Map::StationElement& elStation, const uint32_t imageBase, const uint32_t imageTranslucentBase)
    {
        const auto* elTrack = elStation.prev()->as<Map::TrackElement>();
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
    static void paintTrainStationStyle0(PaintSession& session, const Map::StationElement& elStation, const uint8_t trackId, const uint8_t sequenceIndex, const uint32_t imageBase, const uint32_t imageGlassBase)
    {
        switch (trackId)
        {
            case 0:
                paintTrainStationStyle0StraightTrack(session, elStation, imageBase, imageGlassBase);
                break;
            case 1:
                // Vanllia had hard to reach code for diagonal train stations
                paintTrainStationStyle0DiagonalTrack(session, elStation, imageBase, imageGlassBase);
                break;
            default:
                return;
        }
    }

    // 0x0048B34D
    static void paintTrainStation(PaintSession& session, const Map::StationElement& elStation)
    {
        session.setItemType(Ui::ViewportInteraction::InteractionItem::trackStation);

        const auto* stationObj = ObjectManager::get<TrainStationObject>(elStation.objectId());
        session.setF003F6(-1);

        const auto* elTrack = elStation.prev()->as<Map::TrackElement>();
        if (elTrack == nullptr)
        {
            return;
        }
        const auto companyColour = CompanyManager::getCompanyColour(elTrack->owner());
        auto translucentColour = Colours::getTranslucent(companyColour);
        if (!(stationObj->flags & TrainStationFlags::recolourable))
        {
            translucentColour = ExtColour::unk2E;
        }

        uint32_t imageIdbase = 0;            // 0x0112C720
        uint32_t imageIdTranslucentBase = 0; // 0x0112C724

        if (elStation.isGhost())
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            imageIdbase = Gfx::applyGhostToImage(stationObj->var_12[elStation.multiTileIndex()]);
            imageIdTranslucentBase = Gfx::recolourTranslucent(stationObj->var_12[elStation.multiTileIndex()], ExtColour::unk2F);
        }
        else
        {
            imageIdbase = Gfx::recolour(stationObj->var_12[elStation.multiTileIndex()], companyColour);
            imageIdTranslucentBase = Gfx::recolourTranslucent(stationObj->var_12[elStation.multiTileIndex()], translucentColour);
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
    static void paintRoadStation(PaintSession& session, const Map::StationElement& elStation)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(&elStation);
        regs.ecx = (session.getRotation() + elStation.rotation()) & 0x3;
        regs.dx = elStation.baseHeight();
        regs.bl = elStation.objectId();
        Interop::call(0x0048B403, regs);
    }

    // 0x0048B4D0
    static void paintAirport(PaintSession& session, const Map::StationElement& elStation)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(&elStation);
        regs.ecx = (session.getRotation() + elStation.rotation()) & 0x3;
        regs.dx = elStation.baseHeight();
        regs.bl = elStation.objectId();
        Interop::call(0x0048B4D0, regs);
    }

    // 0x0048B86E
    static void paintDocks(PaintSession& session, const Map::StationElement& elStation)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(&elStation);
        regs.ecx = (session.getRotation() + elStation.rotation()) & 0x3;
        regs.dx = elStation.baseHeight();
        regs.bl = elStation.objectId();
        Interop::call(0x0048B86E, regs);
    }

    // 0x0048B313
    void paintStation(PaintSession& session, const Map::StationElement& elStation)
    {
        if (elStation.isFlag5())
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
