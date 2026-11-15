#include "S5/S5WidePositions.h"
#include "GameState.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleHead.h"
#include <OpenLoco/Core/Exception.hpp>
#include <cstring>
#include <limits>

using namespace OpenLoco::World;

namespace OpenLoco::S5
{
    static constexpr coord_t kMinSaveCoord = std::numeric_limits<int16_t>::min();
    static constexpr coord_t kMaxSaveCoord = std::numeric_limits<int16_t>::max();

    bool coordFitsInSaveFormat(const coord_t value)
    {
        return value >= kMinSaveCoord && value <= kMaxSaveCoord;
    }

    // Visits every coordinate that is stored in the vanilla save format. The order of this
    // enumeration is part of the save format: coordinates may only be appended, never reordered,
    // removed or made conditional on anything but the saved game state itself.
    void visitSavedCoordinates(OpenLoco::GameState& state, const std::function<void(uint32_t, coord_t&)>& visitor)
    {
        uint32_t index = 0;
        const auto visit = [&visitor, &index](coord_t& value) {
            visitor(index++, value);
        };
        const auto visitPos2 = [&visit](Pos2& pos) {
            visit(pos.x);
            visit(pos.y);
        };

        // General state
        visit(state.savedViewX);
        visit(state.savedViewY);
        visitPos2(state.tileUpdateStartLocation);

        for (auto& company : state.companies)
        {
            visit(company.headquartersX);
            visit(company.headquartersY);
            visit(company.observationX);
            visit(company.observationY);
            visit(company.ownerStatus.data[0]);
            visit(company.ownerStatus.data[1]);
            visitPos2(company.aiPathfindTargetPos);
            visitPos2(company.aiPathfindTargetPos2);
            visitPos2(company.aiPathfindStartPos);
            visitPos2(company.aiPathfindStartPos2);

            for (auto& thought : company.aiThoughts)
            {
                for (auto& thoughtStation : thought.stations)
                {
                    visitPos2(thoughtStation.pos);
                }
            }

            for (auto& entry : company.trackandRoadHashTable)
            {
                // Empty entries have no position (their marker is not a valid coordinate)
                if (entry.isEmpty())
                {
                    continue;
                }
                visit(entry.posX);
                visit(entry.posY);
            }
        }

        for (auto& town : state.towns)
        {
            visit(town.x);
            visit(town.y);
        }

        for (auto& industry : state.industries)
        {
            visit(industry.x);
            visit(industry.y);
            for (auto& tile : industry.tiles)
            {
                visitPos2(tile);
            }
            visitPos2(industry.tileLoop.currentPos());
        }

        for (auto& station : state.stations)
        {
            visit(station.x);
            visit(station.y);
            for (auto& tile : station.stationTiles)
            {
                visitPos2(tile);
            }
            visitPos2(station.airportStartPos);
        }

        for (auto& entity : state.entities)
        {
            visit(entity.position.x);
            visit(entity.position.y);

            if (entity.baseType == EntityBaseType::vehicle)
            {
                auto* vehicle = entity.asBase<Vehicles::VehicleBase>();
                if (vehicle != nullptr)
                {
                    // Every vehicle component stores the position it was placed at
                    visit(vehicle->tileX);
                    visit(vehicle->tileY);
                }

                if (vehicle != nullptr && vehicle->getSubType() == Vehicles::VehicleEntityType::head)
                {
                    auto* head = vehicle->asVehicleHead();
                    if (head != nullptr)
                    {
                        visitPos2(head->aiPlacementPos);
                        visitPos2(head->journeyStartPos);
                    }
                }
            }
        }

        for (auto& animation : state.animations)
        {
            visitPos2(animation.pos);
        }

        for (auto& wave : state.waves)
        {
            visitPos2(wave.loc);
        }
    }

    std::vector<WidePosition> collectWidePositions(OpenLoco::GameState& state)
    {
        std::vector<WidePosition> positions;
        visitSavedCoordinates(state, [&positions](const uint32_t index, coord_t& value) {
            if (!coordFitsInSaveFormat(value))
            {
                positions.push_back(WidePosition{ index, value });
            }
        });
        return positions;
    }

    void applyWidePositions(OpenLoco::GameState& state, const std::vector<WidePosition>& positions)
    {
        if (positions.empty())
        {
            return;
        }

        // The positions are stored in the order of visitSavedCoordinates
        size_t i = 0;
        visitSavedCoordinates(state, [&positions, &i](const uint32_t index, coord_t& value) {
            while (i < positions.size() && positions[i].index < index)
            {
                i++;
            }
            if (i < positions.size() && positions[i].index == index)
            {
                value = positions[i].value;
            }
        });
    }

    std::vector<std::byte> encodeWidePositions(const std::vector<WidePosition>& positions)
    {
        constexpr auto kHeaderSize = sizeof(uint32_t) * 2;
        std::vector<std::byte> data(kHeaderSize + positions.size() * sizeof(WidePosition));

        const uint32_t version = kCurrentWidePositionVersion;
        const uint32_t count = static_cast<uint32_t>(positions.size());
        std::memcpy(data.data(), &version, sizeof(version));
        std::memcpy(data.data() + sizeof(version), &count, sizeof(count));
        if (!positions.empty())
        {
            std::memcpy(data.data() + kHeaderSize, positions.data(), positions.size() * sizeof(WidePosition));
        }
        return data;
    }

    std::vector<WidePosition> decodeWidePositions(std::span<const std::byte> data)
    {
        constexpr auto kHeaderSize = sizeof(uint32_t) * 2;
        if (data.size() < kHeaderSize)
        {
            throw Exception::RuntimeError("Invalid wide positions chunk");
        }

        uint32_t version;
        uint32_t count;
        std::memcpy(&version, data.data(), sizeof(version));
        std::memcpy(&count, data.data() + sizeof(version), sizeof(count));

        // Data written by a newer version of OpenLoco may use a different enumeration
        if (version > kCurrentWidePositionVersion)
        {
            return {};
        }
        if (count > (data.size() - kHeaderSize) / sizeof(WidePosition))
        {
            throw Exception::RuntimeError("Invalid wide positions chunk");
        }

        std::vector<WidePosition> positions(count);
        if (count != 0)
        {
            std::memcpy(positions.data(), data.data() + kHeaderSize, count * sizeof(WidePosition));
        }
        return positions;
    }
}
