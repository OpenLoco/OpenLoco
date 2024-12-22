#include "PaintStation.h"
#include "Graphics/Colour.h"
#include "Map/StationElement.h"
#include "Map/TrackElement.h"
#include "Objects/CargoObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrainStationObject.h"
#include "Paint.h"
#include "PaintAirport.h"
#include "PaintDocks.h"
#include "PaintRoadStation.h"
#include "PaintTrainStation.h"
#include "Ui.h"
#include "Ui/ViewportInteraction.h"
#include "World/CompanyManager.h"
#include "World/Station.h"
#include "World/StationManager.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco::Paint
{
    // 0x0042F550
    void paintStationCargo(PaintSession& session, const World::StationElement& elStation, const uint8_t flags, const uint32_t cargoTypes, std::span<const std::array<World::Pos3, 2>> cargoOffsets, const int16_t offsetZ, const World::Pos3& boundBoxOffset, const World::Pos3& boundBoxSize)
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
                if (variation >= cargoObj->numPlatformVariations)
                {
                    variation = 0;
                }
                const auto curVariation = variation;
                variation++;

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
                const auto imageId = ImageId{ cargoObj->unitInlineSprite + Cargo::ImageIds::kStationPlatformBegin + curVariation };
                session.addToPlotListAsChild(imageId, offset + World::Pos3{ 0, 0, offsetZ }, boundBoxOffset, boundBoxSize);
            }
        }
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
