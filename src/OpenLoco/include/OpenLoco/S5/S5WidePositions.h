#pragma once

#include <OpenLoco/Engine/World.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace OpenLoco
{
    struct GameState;
}

namespace OpenLoco::S5
{
    // The vanilla save chunks store world coordinates as 16 bit values, which cannot represent
    // every coordinate of a large map. The coordinates that do not fit are stored in an extra chunk
    // that is written for every map, so that saving and loading works the same for all map sizes.
    // Every such coordinate is identified by its index in a stable enumeration of all saved
    // coordinates (see visitSavedCoordinates), so coordinates may only ever be appended to that
    // enumeration, never reordered or removed.
    struct WidePosition
    {
        uint32_t index;
        coord_t value;

        constexpr bool operator==(const WidePosition&) const = default;
    };

    // Wide position files written by a newer version of OpenLoco are ignored, the coordinates of
    // the vanilla chunks are then used as-is (like for any save file without the extra chunk)
    constexpr uint32_t kCurrentWidePositionVersion = 1;

    // Can the coordinate be represented by the 16 bit coordinates of the vanilla save format?
    bool coordFitsInSaveFormat(const coord_t value);

    // Visits every coordinate of the game state that is stored in the vanilla save format, in a
    // stable order (see WidePosition).
    void visitSavedCoordinates(OpenLoco::GameState& state, const std::function<void(uint32_t, coord_t&)>& visitor);

    // All coordinates of the game state that cannot be represented by the vanilla save format.
    std::vector<WidePosition> collectWidePositions(OpenLoco::GameState& state);

    // Applies previously collected coordinates, overwriting the values that were clamped by the
    // vanilla save format.
    void applyWidePositions(OpenLoco::GameState& state, const std::vector<WidePosition>& positions);

    std::vector<std::byte> encodeWidePositions(const std::vector<WidePosition>& positions);
    std::vector<WidePosition> decodeWidePositions(std::span<const std::byte> data);
}
