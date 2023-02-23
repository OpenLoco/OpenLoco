#pragma once

#include "Types.hpp"
#include <OpenLoco/Engine/World.hpp>
#include <vector>

namespace OpenLoco
{
    enum class Pitch : uint8_t;
}

namespace OpenLoco::World::TrackData
{
#pragma pack(push, 1)
    struct MoveInfo
    {
        World::Pos3 loc; // 0x00
        uint8_t yaw;     // 0x06
        Pitch pitch;     // 0x07
    };
#pragma pack(pop)
    static_assert(sizeof(MoveInfo) == 0x8);

    const std::vector<MoveInfo> getTrackSubPositon(const uint16_t trackAndDirection);
    const std::vector<MoveInfo> getRoadSubPositon(const uint16_t trackAndDirection);
    const std::vector<MoveInfo> getRoadPlacementSubPositon(const uint16_t trackAndDirection);
}
