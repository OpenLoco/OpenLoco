#pragma once

#include "Map/QuarterTile.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Span.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <array>
#include <cstddef>
#include <cstdlib>

namespace OpenLoco::World::TrackData
{
    using ConnectionsByRotation = std::array<uint8_t, 4>;

    enum class PreviewTrackFlags : uint8_t
    {
        none = 0U,
        unk0 = 1U << 0,
        unk1 = 1U << 1,
        unk2 = 1U << 2,
        unk3 = 1U << 3,
        unk4 = 1U << 4,
        unused = 1U << 6, // Not set on any track piece
        diagonal = 1U << 7,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(PreviewTrackFlags);

    struct PreviewTrack
    {
        uint8_t index;                      // 0x00
        int16_t x;                          // 0x01
        int16_t y;                          // 0x03
        int16_t z;                          // 0x05
        uint8_t clearZ;                     // 0x07
        QuarterTile subTileClearance;       // 0x08
        PreviewTrackFlags flags;            // 0x09
        ConnectionsByRotation connectFlags; // From 0x004F78F8 & 0x004F6F1C

        constexpr bool hasFlags(PreviewTrackFlags flagsToTest) const
        {
            return (flags & flagsToTest) != PreviewTrackFlags::none;
        };
    };

#pragma pack(push, 1)
    // Pos is difference from the next first tile and the track first tile
    struct TrackCoordinates
    {
        uint8_t rotationBegin; // 0x00
        uint8_t rotationEnd;   // 0x01
        World::Pos3 pos;       // 0x02
    };
    static_assert(sizeof(TrackCoordinates) == 0x8);
#pragma pack(pop)

    const stdx::span<const PreviewTrack> getTrackPiece(size_t trackId);
    const stdx::span<const PreviewTrack> getRoadPiece(size_t trackId);
    const TrackCoordinates& getUnkTrack(uint16_t trackAndDirection);
    const TrackCoordinates& getUnkRoad(uint16_t trackAndDirection);
}
