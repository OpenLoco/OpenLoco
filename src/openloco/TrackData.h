#pragma once

#include "types.hpp"
#include <vector>

namespace openloco::map::TrackData
{

#pragma pack(push, 1)
    struct PreviewTrack
    {
        uint8_t index; // 0x00
        int16_t x;     // 0x01
        int16_t y;     // 0x03
        int16_t z;     // 0x05
        uint8_t var_07;
        uint8_t var_08;
        uint8_t flags; // 0x09
    };
#pragma pack(pop)

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

    const std::vector<PreviewTrack>& getTrackPiece(size_t trackId);
    const std::vector<PreviewTrack>& getRoadPiece(size_t trackId);
}
