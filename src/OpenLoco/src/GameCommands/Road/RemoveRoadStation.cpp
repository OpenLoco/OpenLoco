#include "GameCommands/Road/RemoveRoadStation.h"
#include "Economy/Economy.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/TileElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadStationObject.h"
#include "ViewportManager.h"
#include "World/Station.h"
#include "World/StationManager.h"

namespace OpenLoco::GameCommands
{
    // TODO: based on CreateRoadStation.cpp
    static World::RoadElement* getElRoad(World::Pos3 pos, uint8_t rotation, uint8_t roadObjectId, uint8_t roadId, uint8_t index)
    {
        auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* elRoad = el.as<World::RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }
            if (elRoad->baseHeight() != pos.z)
            {
                continue;
            }
            if (elRoad->rotation() != rotation)
            {
                continue;
            }
            if (elRoad->roadId() != roadId)
            {
                continue;
            }
            // TODO: not seeing this check in disasm
            if (elRoad->roadObjectId() != roadObjectId)
            {
                continue;
            }
            if (elRoad->sequenceIndex() != index)
            {
                continue;
            }
            return elRoad;
        }
        return nullptr;
    }

    struct RoadElements
    {
        World::TileElementEntry* begin;
        World::TileElementEntry* end;
    };

    static RoadElements getRoadElementsRange(const World::Pos3& roadLoc)
    {
        RoadElements range{ nullptr, nullptr };
        auto tile = World::TileManager::get(roadLoc);
        for (auto& el : tile)
        {
            auto* elRoad = el.as<World::RoadElement>();
            if (elRoad == nullptr)
            {
                if (range.end != nullptr)
                {
                    break;
                }
                continue;
            }
            if (elRoad->baseHeight() != roadLoc.z)
            {
                if (range.end != nullptr)
                {
                    break;
                }
                continue;
            }
            range.end = el.next();
            if (range.begin == nullptr)
            {
                range.begin = &el;
            }
        }
        return range;
    }

    // 0x0048D2AC
    static currency32_t removeRoadStation(const RoadStationRemovalArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));
        bool updateStationTileRegistration = true;

        // Get road at position
        auto* initialElRoad = getElRoad(args.pos, args.rotation, args.roadObjectId, args.roadId, args.index);
        if (initialElRoad == nullptr)
        {
            return kFailure;
        }

        // No station element on this road?
        if (!initialElRoad->hasStationElement())
        {
            return kFailure;
        }

        // Find the station element for an ownership check
        {
            // Station element must be the next element after the last consecutive road element
            const auto roadRange = getRoadElementsRange(args.pos);
            if (roadRange.begin == nullptr || roadRange.end == nullptr)
            {
                return kFailure;
            }
            auto* stationEl = roadRange.end->as<World::StationElement>();

            if (stationEl == nullptr)
            {
                return kFailure;
            }

            // NB: vanilla would query owner from station struct, not the station element
            if (!checkCompanyCompatibility(stationEl->owner(), *stationEl))
            {
                return kFailure;
            }
        }

        const auto& roadPieces = World::TrackData::getRoadPiece(args.roadId);
        const auto& argPiece = roadPieces[args.index];
        const auto roadStart = args.pos - World::Pos3(Math::Vector::rotate(World::Pos2(argPiece.x, argPiece.y), args.rotation), argPiece.z);

        StationId foundStationId = StationId::null;
        currency32_t totalCost = 0;

        for (auto& piece : roadPieces)
        {
            const auto roadLoc = roadStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

            const auto roadRange = getRoadElementsRange(args.pos);
            if (roadRange.begin == nullptr || roadRange.end == nullptr)
            {
                return kFailure;
            }
            auto* stationEntry = roadRange.end;
            auto* stationEl = stationEntry->as<World::StationElement>();
            if (stationEl == nullptr)
            {
                return kFailure;
            }

            if (stationEl->isGhost())
            {
                updateStationTileRegistration = false;
            }

            foundStationId = stationEl->stationId();
            auto* stationObj = ObjectManager::get<RoadStationObject>(stationEl->objectId());

            if (piece.index == 0)
            {
                auto removeCostBase = Economy::getInflationAdjustedCost(stationObj->sellCostFactor, stationObj->costIndex, 8);
                totalCost += (removeCostBase * World::TrackData::getRoadMiscData(args.roadId).costFactor) / 256;
            }

            if ((flags & Flags::apply) != 0)
            {
                for (auto* roadEntry = roadRange.begin; roadEntry != roadRange.end; ++roadEntry)
                {
                    auto* elRoad = roadEntry->as<World::RoadElement>();
                    elRoad->setHasStationElement(false);
                    elRoad->setClearZ(elRoad->clearZ() - stationObj->height);
                }
                Ui::ViewportManager::invalidate(World::Pos2(roadLoc), stationEl->baseHeight(), stationEl->clearHeight(), ZoomLevel::eighth);
                World::TileManager::removeElement(*stationEntry);
            }
        }

        if (updateStationTileRegistration && (flags & Flags::apply) != 0)
        {
            auto* station = StationManager::get(foundStationId);
            removeTileFromStation(foundStationId, roadStart, args.rotation);
            station->invalidate();

            recalculateStationModes(foundStationId);
            recalculateStationCenter(foundStationId);
            station->updateLabel();
            station->invalidate();
        }

        return totalCost;
    }

    void removeRoadStation(registers& regs, const uint8_t flags)
    {
        regs.ebx = removeRoadStation(RoadStationRemovalArgs(regs), flags);
    }
}
