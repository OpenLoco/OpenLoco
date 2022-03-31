#include "TrackData.h"
#include "../../Interop/Interop.hpp"
#include "../../Utility/Numeric.hpp"
#include <array>
#include <cassert>

namespace OpenLoco::Map::TrackData
{
    constexpr ConnectionsByRotation generateConnections(uint8_t connection)
    {
        return {
            connection,
            Utility::rol(connection, 2),
            Utility::rol(connection, 4),
            Utility::rol(connection, 6),
        };
    }

    const std::array<PreviewTrack, 1> trackPiece0 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 4> trackPiece1 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 9, 0, generateConnections(0x07) },
        PreviewTrack{ 1, 0, 32, 0, 0, 4, PreviewTrackFlags::diagonal, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 0, 0, 1, PreviewTrackFlags::diagonal, generateConnections(0x08) },
        PreviewTrack{ 3, -32, 32, 0, 0, 6, 0, generateConnections(0x70) },
    };
    const std::array<PreviewTrack, 1> trackPiece2 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 7, 0, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 1> trackPiece3 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 11, 0, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 4> trackPiece4 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 7, 0, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, 0, 0, 8, PreviewTrackFlags::diagonal, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, 0, 0, 2, 0, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, 0, 0, 7, 0, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> trackPiece5 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 11, 0, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, 0, 0, 4, PreviewTrackFlags::diagonal, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 0, 0, 1, 0, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, 0, 0, 11, 0, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 5> trackPiece6 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 0, 7, 0, generateConnections(0xF8) },
        PreviewTrack{ 2, -32, -32, 0, 0, 13, 0, generateConnections(0x8F) },
        PreviewTrack{ 3, -64, -32, 0, 0, 7, 0, generateConnections(0xF8) },
        PreviewTrack{ 4, -64, -64, 0, 0, 15, 0, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 5> trackPiece7 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 0, 11, 0, generateConnections(0x3E) },
        PreviewTrack{ 2, -32, 32, 0, 0, 14, 0, generateConnections(0xE3) },
        PreviewTrack{ 3, -64, 32, 0, 0, 11, 0, generateConnections(0x3E) },
        PreviewTrack{ 4, -64, 64, 0, 0, 15, 0, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 5> trackPiece8 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 0, 7, 0, generateConnections(0xF8) },
        PreviewTrack{ 2, -32, -32, 0, 0, 8, 0, generateConnections(0x02) },
        PreviewTrack{ 3, -64, 0, 0, 0, 2, PreviewTrackFlags::diagonal, generateConnections(0x20) },
        PreviewTrack{ 4, -64, -32, 0, 0, 3, 0, generateConnections(0x1C) },
    };
    const std::array<PreviewTrack, 5> trackPiece9 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 0, 11, 0, generateConnections(0x3E) },
        PreviewTrack{ 2, -32, 32, 0, 0, 4, 0, generateConnections(0x80) },
        PreviewTrack{ 3, -64, 0, 0, 0, 1, PreviewTrackFlags::diagonal, generateConnections(0x08) },
        PreviewTrack{ 4, -64, 32, 0, 0, 6, 0, generateConnections(0x70) },
    };
    const std::array<PreviewTrack, 5> trackPiece10 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 9, 0, generateConnections(0x07) },
        PreviewTrack{ 1, -32, 0, 0, 0, 1, 0, generateConnections(0x08) },
        PreviewTrack{ 2, 0, 32, 0, 0, 4, PreviewTrackFlags::diagonal, generateConnections(0x80) },
        PreviewTrack{ 3, -32, 32, 0, 0, 14, 0, generateConnections(0xE3) },
        PreviewTrack{ 4, -64, 32, 0, 0, 15, 0, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 5> trackPiece11 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 9, 0, generateConnections(0x07) },
        PreviewTrack{ 1, 0, 32, 0, 0, 4, 0, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 0, 0, 1, PreviewTrackFlags::diagonal, generateConnections(0x08) },
        PreviewTrack{ 3, -32, 32, 0, 0, 11, 0, generateConnections(0x3E) },
        PreviewTrack{ 4, -32, 64, 0, 0, 15, 0, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 4> trackPiece12 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xF9) },
        PreviewTrack{ 1, -32, 0, 0, 0, 6, 0, generateConnections(0x60) },
        PreviewTrack{ 2, -32, -32, 0, 0, 9, 0, generateConnections(0x06) },
        PreviewTrack{ 3, -64, -32, 0, 0, 15, 0, generateConnections(0x9F) },
    };
    const std::array<PreviewTrack, 4> trackPiece13 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0x3F) },
        PreviewTrack{ 1, -32, 0, 0, 0, 9, 0, generateConnections(0x0C) },
        PreviewTrack{ 2, -32, 32, 0, 0, 6, 0, generateConnections(0xC0) },
        PreviewTrack{ 3, -64, 32, 0, 0, 15, 0, generateConnections(0xF3) },
    };
    const std::array<PreviewTrack, 2> trackPiece14 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 15, PreviewTrackFlags::unk4, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 16, 207, PreviewTrackFlags::unk4, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 2> trackPiece15 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 63, PreviewTrackFlags::unk4, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, -16, 16, 15, PreviewTrackFlags::unk4, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> trackPiece16 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 207, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> trackPiece17 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 63, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 4> trackPiece18 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 7, PreviewTrackFlags::unk4, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, 0, 16, 8, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, 0, 16, 2, PreviewTrackFlags::unk4, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, 0, 16, 103, PreviewTrackFlags::unk4, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> trackPiece19 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 11, PreviewTrackFlags::unk4, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, 0, 16, 4, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 0, 16, 1, PreviewTrackFlags::unk4, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, 0, 16, 155, PreviewTrackFlags::unk4, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 4> trackPiece20 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 55, PreviewTrackFlags::unk4, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, -16, 16, 8, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, -16, 16, 2, PreviewTrackFlags::unk4, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, -16, 16, 7, PreviewTrackFlags::unk4, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> trackPiece21 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 59, PreviewTrackFlags::unk4, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, -16, 16, 4, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, -16, 16, 1, PreviewTrackFlags::unk4, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, -16, 16, 11, PreviewTrackFlags::unk4, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 4> trackPiece22 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 71, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, 16, 0, 8, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, 16, 0, 2, PreviewTrackFlags::unk4, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, 16, 16, 103, PreviewTrackFlags::unk2 | PreviewTrackFlags::unk1, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> trackPiece23 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 139, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, 16, 0, 4, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 16, 0, 1, PreviewTrackFlags::unk4, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, 16, 16, 155, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk0, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 4> trackPiece24 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 55, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, -16, 0, 8, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, -16, 0, 2, PreviewTrackFlags::unk4, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, -32, 16, 23, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk0, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> trackPiece25 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 59, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, -16, 0, 4, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, -16, 0, 1, PreviewTrackFlags::unk4, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, -32, 16, 43, PreviewTrackFlags::unk2 | PreviewTrackFlags::unk1, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 1> trackPiece26 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 6, 0, generateConnections(0xE0) },
    };
    const std::array<PreviewTrack, 1> trackPiece27 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 9, 0, generateConnections(0x0E) },
    };
    const std::array<PreviewTrack, 1> trackPiece28 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 2, 0, generateConnections(0x20) },
    };
    const std::array<PreviewTrack, 1> trackPiece29 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0x8D) },
    };
    const std::array<PreviewTrack, 1> trackPiece30 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0x63) },
    };
    const std::array<PreviewTrack, 1> trackPiece31 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 1, 0, generateConnections(0x08) },
    };
    const std::array<PreviewTrack, 1> trackPiece32 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0x77) },
    };
    const std::array<PreviewTrack, 1> trackPiece33 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xDD) },
    };
    const std::array<PreviewTrack, 1> trackPiece34 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 198, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0xE0) },
    };
    const std::array<PreviewTrack, 1> trackPiece35 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 201, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0x0E) },
    };
    const std::array<PreviewTrack, 1> trackPiece36 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 54, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0xE0) },
    };
    const std::array<PreviewTrack, 1> trackPiece37 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 57, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0x0E) },
    };
    const std::array<PreviewTrack, 1> trackPiece38 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xFD) },
    };
    const std::array<PreviewTrack, 1> trackPiece39 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0x7F) },
    };
    const std::array<PreviewTrack, 1> trackPiece40 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xF7) },
    };
    const std::array<PreviewTrack, 1> trackPiece41 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xDF) },
    };
    const std::array<PreviewTrack, 1> trackPiece42 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 3, 0, generateConnections(0x38) },
    };
    const std::array<PreviewTrack, 1> trackPiece43 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 3, 0, generateConnections(0x38) },
    };

    // 0x004F73D8, 0x004F78F8
    const std::array<stdx::span<const PreviewTrack>, 44> trackPieces = { {
        trackPiece0,
        trackPiece1,
        trackPiece2,
        trackPiece3,
        trackPiece4,
        trackPiece5,
        trackPiece6,
        trackPiece7,
        trackPiece8,
        trackPiece9,
        trackPiece10,
        trackPiece11,
        trackPiece12,
        trackPiece13,
        trackPiece14,
        trackPiece15,
        trackPiece16,
        trackPiece17,
        trackPiece18,
        trackPiece19,
        trackPiece20,
        trackPiece21,
        trackPiece22,
        trackPiece23,
        trackPiece24,
        trackPiece25,
        trackPiece26,
        trackPiece27,
        trackPiece28,
        trackPiece29,
        trackPiece30,
        trackPiece31,
        trackPiece32,
        trackPiece33,
        trackPiece34,
        trackPiece35,
        trackPiece36,
        trackPiece37,
        trackPiece38,
        trackPiece39,
        trackPiece40,
        trackPiece41,
        trackPiece42,
        trackPiece43,
    } };

    const std::array<PreviewTrack, 1> roadPiece0 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> roadPiece1 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 1> roadPiece2 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 4> roadPiece3 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 7, 0, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, 0, 0, 8, PreviewTrackFlags::diagonal, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, 0, 0, 2, 0, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, 0, 0, 7, 0, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> roadPiece4 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 11, 0, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, 0, 0, 4, PreviewTrackFlags::diagonal, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 0, 0, 1, 0, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, 0, 0, 11, 0, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 2> roadPiece5 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 15, PreviewTrackFlags::unk4, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 16, 207, PreviewTrackFlags::unk4, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 2> roadPiece6 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 63, PreviewTrackFlags::unk4, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, -16, 16, 15, PreviewTrackFlags::unk4, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> roadPiece7 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 207, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> roadPiece8 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 63, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> roadPiece9 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 3, 0, generateConnections(0x38) },
    };

    // 0x004F6D1C, 0x004F6F1C
    const std::array<stdx::span<const PreviewTrack>, 10> roadPieces = { {
        roadPiece0,
        roadPiece1,
        roadPiece2,
        roadPiece3,
        roadPiece4,
        roadPiece5,
        roadPiece6,
        roadPiece7,
        roadPiece8,
        roadPiece9,
    } };

    const stdx::span<const PreviewTrack> getTrackPiece(size_t trackId)
    {
        assert(trackId < trackPieces.size());
        return trackPieces[trackId];
    }

    const stdx::span<const PreviewTrack> getRoadPiece(size_t trackId)
    {
        assert(trackId < roadPieces.size());
        return roadPieces[trackId];
    }

    static Interop::loco_global<TrackCoordinates[80], 0x004F6F8C> _4F6F8C;
    static Interop::loco_global<TrackCoordinates[352], 0x004F7B5C> _4F7B5C;

    const TrackCoordinates& getUnkTrack(uint16_t trackAndDirection)
    {
        return _4F7B5C[trackAndDirection];
    }

    const TrackCoordinates& getUnkRoad(uint16_t trackAndDirection)
    {
        return _4F6F8C[trackAndDirection];
    }
}
