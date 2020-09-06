#include "TrackData.h"
#include <array>
#include <cassert>

namespace openloco::map::TrackData
{
    const std::vector<PreviewTrack> trackPiece0 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece1 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 9, 0 },
        PreviewTrack{ 1, 0, 32, 0, 0, 4, PreviewTrackFlags::diagonal },
        PreviewTrack{ 2, -32, 0, 0, 0, 1, PreviewTrackFlags::diagonal },
        PreviewTrack{ 3, -32, 32, 0, 0, 6, 0 },
    };
    const std::vector<PreviewTrack> trackPiece2 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 7, 0 },
    };
    const std::vector<PreviewTrack> trackPiece3 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 11, 0 },
    };
    const std::vector<PreviewTrack> trackPiece4 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 7, 0 },
        PreviewTrack{ 1, 0, -32, 0, 0, 8, PreviewTrackFlags::diagonal },
        PreviewTrack{ 2, -32, 0, 0, 0, 2, 0 },
        PreviewTrack{ 3, -32, -32, 0, 0, 7, 0 },
    };
    const std::vector<PreviewTrack> trackPiece5 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 11, 0 },
        PreviewTrack{ 1, 0, 32, 0, 0, 4, PreviewTrackFlags::diagonal },
        PreviewTrack{ 2, -32, 0, 0, 0, 1, 0 },
        PreviewTrack{ 3, -32, 32, 0, 0, 11, 0 },
    };
    const std::vector<PreviewTrack> trackPiece6 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
        PreviewTrack{ 1, -32, 0, 0, 0, 7, 0 },
        PreviewTrack{ 2, -32, -32, 0, 0, 13, 0 },
        PreviewTrack{ 3, -64, -32, 0, 0, 7, 0 },
        PreviewTrack{ 4, -64, -64, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece7 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
        PreviewTrack{ 1, -32, 0, 0, 0, 11, 0 },
        PreviewTrack{ 2, -32, 32, 0, 0, 14, 0 },
        PreviewTrack{ 3, -64, 32, 0, 0, 11, 0 },
        PreviewTrack{ 4, -64, 64, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece8 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
        PreviewTrack{ 1, -32, 0, 0, 0, 7, 0 },
        PreviewTrack{ 2, -32, -32, 0, 0, 8, 0 },
        PreviewTrack{ 3, -64, 0, 0, 0, 2, PreviewTrackFlags::diagonal },
        PreviewTrack{ 4, -64, -32, 0, 0, 3, 0 },
    };
    const std::vector<PreviewTrack> trackPiece9 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
        PreviewTrack{ 1, -32, 0, 0, 0, 11, 0 },
        PreviewTrack{ 2, -32, 32, 0, 0, 4, 0 },
        PreviewTrack{ 3, -64, 0, 0, 0, 1, PreviewTrackFlags::diagonal },
        PreviewTrack{ 4, -64, 32, 0, 0, 6, 0 },
    };
    const std::vector<PreviewTrack> trackPiece10 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 9, 0 },
        PreviewTrack{ 1, -32, 0, 0, 0, 1, 0 },
        PreviewTrack{ 2, 0, 32, 0, 0, 4, PreviewTrackFlags::diagonal },
        PreviewTrack{ 3, -32, 32, 0, 0, 14, 0 },
        PreviewTrack{ 4, -64, 32, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece11 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 9, 0 },
        PreviewTrack{ 1, 0, 32, 0, 0, 4, 0 },
        PreviewTrack{ 2, -32, 0, 0, 0, 1, PreviewTrackFlags::diagonal },
        PreviewTrack{ 3, -32, 32, 0, 0, 11, 0 },
        PreviewTrack{ 4, -32, 64, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece12 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
        PreviewTrack{ 1, -32, 0, 0, 0, 6, 0 },
        PreviewTrack{ 2, -32, -32, 0, 0, 9, 0 },
        PreviewTrack{ 3, -64, -32, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece13 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
        PreviewTrack{ 1, -32, 0, 0, 0, 9, 0 },
        PreviewTrack{ 2, -32, 32, 0, 0, 6, 0 },
        PreviewTrack{ 3, -64, 32, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece14 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 15, PreviewTrackFlags::unk4 },
        PreviewTrack{ 1, -32, 0, 0, 16, 207, PreviewTrackFlags::unk4 },
    };
    const std::vector<PreviewTrack> trackPiece15 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 63, PreviewTrackFlags::unk4 },
        PreviewTrack{ 1, -32, 0, -16, 16, 15, PreviewTrackFlags::unk4 },
    };
    const std::vector<PreviewTrack> trackPiece16 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 207, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2 },
    };
    const std::vector<PreviewTrack> trackPiece17 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 63, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0 },
    };
    const std::vector<PreviewTrack> trackPiece18 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 7, PreviewTrackFlags::unk4 },
        PreviewTrack{ 1, 0, -32, 0, 16, 8, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4 },
        PreviewTrack{ 2, -32, 0, 0, 16, 2, PreviewTrackFlags::unk4 },
        PreviewTrack{ 3, -32, -32, 0, 16, 103, PreviewTrackFlags::unk4 },
    };
    const std::vector<PreviewTrack> trackPiece19 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 11, PreviewTrackFlags::unk4 },
        PreviewTrack{ 1, 0, 32, 0, 16, 4, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4 },
        PreviewTrack{ 2, -32, 0, 0, 16, 1, PreviewTrackFlags::unk4 },
        PreviewTrack{ 3, -32, 32, 0, 16, 155, PreviewTrackFlags::unk4 },
    };
    const std::vector<PreviewTrack> trackPiece20 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 55, PreviewTrackFlags::unk4 },
        PreviewTrack{ 1, 0, -32, -16, 16, 8, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4 },
        PreviewTrack{ 2, -32, 0, -16, 16, 2, PreviewTrackFlags::unk4 },
        PreviewTrack{ 3, -32, -32, -16, 16, 7, PreviewTrackFlags::unk4 },
    };
    const std::vector<PreviewTrack> trackPiece21 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 59, PreviewTrackFlags::unk4 },
        PreviewTrack{ 1, 0, 32, -16, 16, 4, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4 },
        PreviewTrack{ 2, -32, 0, -16, 16, 1, PreviewTrackFlags::unk4 },
        PreviewTrack{ 3, -32, 32, -16, 16, 11, PreviewTrackFlags::unk4 },
    };
    const std::vector<PreviewTrack> trackPiece22 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 71, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2 },
        PreviewTrack{ 1, 0, -32, 16, 0, 8, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4 },
        PreviewTrack{ 2, -32, 0, 16, 0, 2, PreviewTrackFlags::unk4 },
        PreviewTrack{ 3, -32, -32, 16, 16, 103, PreviewTrackFlags::unk2 | PreviewTrackFlags::unk1 },
    };
    const std::vector<PreviewTrack> trackPiece23 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 139, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2 },
        PreviewTrack{ 1, 0, 32, 16, 0, 4, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4 },
        PreviewTrack{ 2, -32, 0, 16, 0, 1, PreviewTrackFlags::unk4 },
        PreviewTrack{ 3, -32, 32, 16, 16, 155, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk0 },
    };
    const std::vector<PreviewTrack> trackPiece24 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 55, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0 },
        PreviewTrack{ 1, 0, -32, -16, 0, 8, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4 },
        PreviewTrack{ 2, -32, 0, -16, 0, 2, PreviewTrackFlags::unk4 },
        PreviewTrack{ 3, -32, -32, -32, 16, 23, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk0 },
    };
    const std::vector<PreviewTrack> trackPiece25 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 59, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0 },
        PreviewTrack{ 1, 0, 32, -16, 0, 4, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4 },
        PreviewTrack{ 2, -32, 0, -16, 0, 1, PreviewTrackFlags::unk4 },
        PreviewTrack{ 3, -32, 32, -32, 16, 43, PreviewTrackFlags::unk2 | PreviewTrackFlags::unk1 },
    };
    const std::vector<PreviewTrack> trackPiece26 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 6, 0 },
    };
    const std::vector<PreviewTrack> trackPiece27 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 9, 0 },
    };
    const std::vector<PreviewTrack> trackPiece28 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 2, 0 },
    };
    const std::vector<PreviewTrack> trackPiece29 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece30 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece31 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 1, 0 },
    };
    const std::vector<PreviewTrack> trackPiece32 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece33 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece34 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 198, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2 },
    };
    const std::vector<PreviewTrack> trackPiece35 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 201, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2 },
    };
    const std::vector<PreviewTrack> trackPiece36 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 54, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0 },
    };
    const std::vector<PreviewTrack> trackPiece37 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 57, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0 },
    };
    const std::vector<PreviewTrack> trackPiece38 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece39 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece40 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece41 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> trackPiece42 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 3, 0 },
    };
    const std::vector<PreviewTrack> trackPiece43 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 3, 0 },
    };

    // 0x004F73D8
    const std::array<std::vector<PreviewTrack>, 44> trackPieces = { {
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

    const std::vector<PreviewTrack> roadPiece0 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> roadPiece1 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> roadPiece2 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 15, 0 },
    };
    const std::vector<PreviewTrack> roadPiece3 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 7, 0 },
        PreviewTrack{ 1, 0, -32, 0, 0, 8, PreviewTrackFlags::diagonal },
        PreviewTrack{ 2, -32, 0, 0, 0, 2, 0 },
        PreviewTrack{ 3, -32, -32, 0, 0, 7, 0 },
    };
    const std::vector<PreviewTrack> roadPiece4 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 11, 0 },
        PreviewTrack{ 1, 0, 32, 0, 0, 4, PreviewTrackFlags::diagonal },
        PreviewTrack{ 2, -32, 0, 0, 0, 1, 0 },
        PreviewTrack{ 3, -32, 32, 0, 0, 11, 0 },
    };
    const std::vector<PreviewTrack> roadPiece5 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 15, PreviewTrackFlags::unk4 },
        PreviewTrack{ 1, -32, 0, 0, 16, 207, PreviewTrackFlags::unk4 },
    };
    const std::vector<PreviewTrack> roadPiece6 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 63, PreviewTrackFlags::unk4 },
        PreviewTrack{ 1, -32, 0, -16, 16, 15, PreviewTrackFlags::unk4 },
    };
    const std::vector<PreviewTrack> roadPiece7 = {
        PreviewTrack{ 0, 0, 0, 0, 16, 207, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2 },
    };
    const std::vector<PreviewTrack> roadPiece8 = {
        PreviewTrack{ 0, 0, 0, -16, 16, 63, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0 },
    };
    const std::vector<PreviewTrack> roadPiece9 = {
        PreviewTrack{ 0, 0, 0, 0, 0, 3, 0 },
    };

    // 0x004F6D1C
    const std::array<std::vector<PreviewTrack>, 10> roadPieces = { { roadPiece0, roadPiece1, roadPiece2, roadPiece3, roadPiece4, roadPiece5, roadPiece6, roadPiece7, roadPiece8, roadPiece9 } };

    const std::vector<PreviewTrack>& getTrackPiece(size_t trackId)
    {
        assert(trackId < trackPieces.size());
        return trackPieces[trackId];
    }

    const std::vector<PreviewTrack>& getRoadPiece(size_t trackId)
    {
        assert(trackId < roadPieces.size());
        return roadPieces[trackId];
    }
}
