#pragma once

#include "../../Core/Span.hpp"
#include "../../Types.hpp"
#include "../Map.hpp"
#include <array>
#include <cstddef>
#include <cstdlib>

namespace OpenLoco::Map::TrackData
{
    using ConnectionsByRotation = std::array<uint8_t, 4>;

    struct PreviewTrack
    {
        uint8_t index; // 0x00
        int16_t x;     // 0x01
        int16_t y;     // 0x03
        int16_t z;     // 0x05
        uint8_t var_07;
        uint8_t var_08;
        uint8_t flags;                      // 0x09
        ConnectionsByRotation connectFlags; // From 0x004F78F8 & 0x004F6F1C
    };

    namespace PreviewTrackFlags
    {
        constexpr uint8_t unk0 = 1 << 0;
        constexpr uint8_t unk1 = 1 << 1;
        constexpr uint8_t unk2 = 1 << 2;
        constexpr uint8_t unk3 = 1 << 3;
        constexpr uint8_t unk4 = 1 << 4;
        constexpr uint8_t unused = 1 << 6; // Not set on any track piece
        constexpr uint8_t diagonal = 1 << 7;
    }

#pragma pack(push, 1)
    // Pos is difference from the next first tile and the track first tile
    struct TrackCoordinates
    {
        uint8_t rotationBegin; // 0x00
        uint8_t rotationEnd;   // 0x01
        Map::Pos3 pos;         // 0x02
    };
    static_assert(sizeof(TrackCoordinates) == 0x8);
#pragma pack(pop)

    const stdx::span<const PreviewTrack> getTrackPiece(size_t trackId);
    const stdx::span<const PreviewTrack> getRoadPiece(size_t trackId);
    const TrackCoordinates& getUnkTrack(uint16_t trackAndDirection);
    const TrackCoordinates& getUnkRoad(uint16_t trackAndDirection);
}
