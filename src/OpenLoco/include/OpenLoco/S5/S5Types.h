#pragma once

#include <OpenLoco/Engine/World.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace OpenLoco::S5
{
    // Positions as they are stored in the (vanilla) save format: 16 bit world coordinates. These are
    // deliberately separate from the runtime types (which are 32 bit) so that the save file layout
    // does not depend on the size of the runtime coordinates.
    struct Pos2
    {
        int16_t x;
        int16_t y;
    };
    static_assert(sizeof(Pos2) == 4);

    struct Pos3
    {
        int16_t x;
        int16_t y;
        int16_t z;
    };
    static_assert(sizeof(Pos3) == 6);

    // The save format cannot represent coordinates outside the 16 bit range. Values are clamped
    // when they are written to the vanilla save chunks; the values that cannot be represented are
    // stored separately so that they can be restored when the save file is loaded again
    // (see S5WidePositions.h).
    inline int16_t toSaveCoord(const coord_t value)
    {
        return static_cast<int16_t>(std::clamp<coord_t>(value, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max()));
    }

    inline Pos2 toSavePos(const World::Pos2& pos)
    {
        return { toSaveCoord(pos.x), toSaveCoord(pos.y) };
    }

    inline Pos3 toSavePos(const World::Pos3& pos)
    {
        return { toSaveCoord(pos.x), toSaveCoord(pos.y), toSaveCoord(pos.z) };
    }

    inline World::Pos2 fromSavePos(const Pos2& pos)
    {
        return { pos.x, pos.y };
    }

    inline World::Pos3 fromSavePos(const Pos3& pos)
    {
        return { pos.x, pos.y, pos.z };
    }

    inline coord_t fromSaveCoord(const int16_t value)
    {
        return value;
    }

    // Some save fields are unsigned but hold signed values with special meanings (e.g. owner status
    // uses -1 and -2, hash table entries pack flags into the low bit), so their sign must be kept
    inline uint16_t toSaveUCoord(const coord_t value)
    {
        return static_cast<uint16_t>(toSaveCoord(value));
    }

    inline coord_t fromSaveUCoord(const uint16_t value)
    {
        return static_cast<int16_t>(value);
    }

    template<typename TSavePosArray, typename TRuntimePosArray>
    void toSavePosArray(const TRuntimePosArray& src, TSavePosArray& dst)
    {
        for (size_t i = 0; i < std::size(dst); ++i)
        {
            dst[i] = toSavePos(src[i]);
        }
    }

    template<typename TSavePosArray, typename TRuntimePosArray>
    void fromSavePosArray(const TSavePosArray& src, TRuntimePosArray& dst)
    {
        for (size_t i = 0; i < std::size(dst); ++i)
        {
            dst[i] = fromSavePos(src[i]);
        }
    }
}
