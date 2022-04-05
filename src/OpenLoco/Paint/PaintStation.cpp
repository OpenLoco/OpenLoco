#pragma once
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
                TrainStation::ImageIds::style0StraightBackNE,
                { 2, 2, 8 },
                { 28, 4, 3 },
            },
            {
                TrainStation::ImageIds::style0StraightFrontNE,
                { 2, 24, 8 },
                { 28, 4, 3 },
            },
            {
                TrainStation::ImageIds::style0StraightCanopyNE,
                { 2, 2, 26 },
                { 28, 28, 1 },
            },
            {
                TrainStation::ImageIds::style0StraightCanopyTranslucentNE,
                {},
                {},
            },
        };
        constexpr PlatformImage seStationPlatformImage = {
            {
                TrainStation::ImageIds::style0StraightBackSE,
                { 2, 2, 8 },
                { 4, 28, 3 },
            },
            {
                TrainStation::ImageIds::style0StraightFrontSE,
                { 24, 2, 8 },
                { 4, 28, 3 },
            },
            {
                TrainStation::ImageIds::style0StraightCanopySE,
                { 2, 2, 26 },
                { 28, 28, 1 },
            },
            {
                TrainStation::ImageIds::style0StraightCanopyTranslucentSE,
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

        const Map::Pos3 heightOffest(0, 0, elStation.baseHeight());

        // Paint Back part of platform
        {
            Map::Pos3 bbOffset = platformImages.back.bbOffset + heightOffest;
            session.addToPlotList4FD150(imageBase + platformImages.back.imageId, heightOffest, bbOffset, platformImages.back.bbSize);

            paintStationCargo(session, elStation, cargoFlags[0], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.back.bbSize);
            paintStationCargo(session, elStation, cargoFlags[1], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.back.bbSize);
        }
        // Paint Front part of platform
        {
            Map::Pos3 bbOffset = platformImages.front.bbOffset + heightOffest;
            session.addToPlotList4FD150(imageBase + platformImages.front.imageId, heightOffest, bbOffset, platformImages.front.bbSize);

            paintStationCargo(session, elStation, cargoFlags[2], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.front.bbSize);
            paintStationCargo(session, elStation, cargoFlags[3], 0xFFFFFFFF, cargoOffsets, elStation.baseHeight(), bbOffset, platformImages.front.bbSize);
        }

        // Paint Canopy of platform
        {
            Map::Pos3 bbOffset = platformImages.canopy.bbOffset + heightOffest;
            session.addToPlotList4FD180(imageBase + platformImages.canopy.imageId, 1, heightOffest, bbOffset, platformImages.canopy.bbSize);
            session.attachToPrevious(imageTranslucentBase + platformImages.canopyTranslucent.imageId, { 0, 0 });
        }
        session.set525CF8(session.get525CF8() | 0x1FF);
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
                // 0x004D7A5C
                // Vanllia had hard to reach code for diagonal train stations
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
        auto translucentColour = Colour::getTranslucent(companyColour);
        if (!(stationObj->flags & TrainStationFlags::recolourable))
        {
            translucentColour = PaletteIndex::index_2E;
        }

        uint32_t imageIdbase = 0;            // 0x0112C720
        uint32_t imageIdTranslucentBase = 0; // 0x0112C724

        if (elStation.isGhost())
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            imageIdbase = Gfx::applyGhostToImage(stationObj->var_12[elStation.multiTileIndex()]);
            imageIdTranslucentBase = Gfx::recolourTranslucent(stationObj->var_12[elStation.multiTileIndex()], PaletteIndex::index_2F);
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
