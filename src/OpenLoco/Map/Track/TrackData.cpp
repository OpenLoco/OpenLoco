#include "TrackData.h"
#include "../../Interop/Interop.hpp"
#include <array>
#include <cassert>

namespace OpenLoco::Map::TrackData
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

    using ConnectionsByRotation = std::array<uint8_t, 4>;

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece0 = {
        { 0xFF, 0xFF, 0xFF, 0xFF },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece1 = {
        { 0x07, 0x1C, 0x70, 0xC1 },
        { 0x80, 0x02, 0x08, 0x20 },
        { 0x08, 0x20, 0x80, 0x02 },
        { 0x70, 0xC1, 0x07, 0x1C },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece2 = {
        { 0xF8, 0xE3, 0x8F, 0x3E },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece3 = {
        { 0x3E, 0xF8, 0xE3, 0x8F },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece4 = {
        { 0xF8, 0xE3, 0x8F, 0x3E },
        { 0x02, 0x08, 0x20, 0x80 },
        { 0x70, 0xC1, 0x07, 0x1C },
        { 0xF8, 0xE3, 0x8F, 0x3E },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece5 = {
        { 0x3E, 0xF8, 0xE3, 0x8F },
        { 0x80, 0x02, 0x08, 0x20 },
        { 0x1C, 0x70, 0xC1, 0x07 },
        { 0x3E, 0xF8, 0xE3, 0x8F },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece6 = {
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xF8, 0xE3, 0x8F, 0x3E },
        { 0x8F, 0x3E, 0xF8, 0xE3 },
        { 0xF8, 0xE3, 0x8F, 0x3E },
        { 0xFF, 0xFF, 0xFF, 0xFF },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece7 = {
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0x3E, 0xF8, 0xE3, 0x8F },
        { 0xE3, 0x8F, 0x3E, 0xF8 },
        { 0x3E, 0xF8, 0xE3, 0x8F },
        { 0xFF, 0xFF, 0xFF, 0xFF },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece8 = {
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xF8, 0xE3, 0x8F, 0x3E },
        { 0x02, 0x08, 0x20, 0x80 },
        { 0x20, 0x80, 0x02, 0x08 },
        { 0x1C, 0x70, 0xC1, 0x07 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece9 = {
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0x3E, 0xF8, 0xE3, 0x8F },
        { 0x80, 0x02, 0x08, 0x20 },
        { 0x08, 0x20, 0x80, 0x02 },
        { 0x70, 0xC1, 0x07, 0x1C },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece10 = {
        { 0x07, 0x1C, 0x70, 0xC1 },
        { 0x08, 0x20, 0x80, 0x02 },
        { 0x80, 0x02, 0x08, 0x20 },
        { 0xE3, 0x8F, 0x3E, 0xF8 },
        { 0xFF, 0xFF, 0xFF, 0xFF },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece11 = {
        { 0x07, 0x1C, 0x70, 0xC1 },
        { 0x80, 0x02, 0x08, 0x20 },
        { 0x08, 0x20, 0x80, 0x02 },
        { 0x3E, 0xF8, 0xE3, 0x8F },
        { 0xFF, 0xFF, 0xFF, 0xFF },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece12 = {
        { 0xF9, 0xE7, 0x9F, 0x7E },
        { 0x60, 0x81, 0x06, 0x18 },
        { 0x06, 0x18, 0x60, 0x81 },
        { 0x9F, 0x7E, 0xF9, 0xE7 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece13 = {
        { 0x3F, 0xFC, 0xF3, 0xCF },
        { 0x0C, 0x30, 0xC0, 0x03 },
        { 0xC0, 0x03, 0x0C, 0x30 },
        { 0xF3, 0xCF, 0x3F, 0xFC },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece14 = {
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece15 = {
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece16 = {
        { 0xFF, 0xFF, 0xFF, 0xFF },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece17 = {
        { 0xFF, 0xFF, 0xFF, 0xFF },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece18 = {
        { 0xF8, 0xE3, 0x8F, 0x3E },
        { 0x02, 0x08, 0x20, 0x80 },
        { 0x70, 0xC1, 0x07, 0x1C },
        { 0xF8, 0xE3, 0x8F, 0x3E },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece19 = {
        { 0x3E, 0xF8, 0xE3, 0x8F },
        { 0x80, 0x02, 0x08, 0x20 },
        { 0x1C, 0x70, 0xC1, 0x7 },
        { 0x3E, 0xF8, 0xE3, 0x8F },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece20 = {
        { 0xF8, 0xE3, 0x8F, 0x3E },
        { 0x02, 0x08, 0x20, 0x80 },
        { 0x70, 0xC1, 0x07, 0x1C },
        { 0xF8, 0xE3, 0x8F, 0x3E },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece21 = {
        { 0x3E, 0xF8, 0xE3, 0x8F },
        { 0x80, 0x02, 0x08, 0x20 },
        { 0x1C, 0x70, 0xC1, 0x07 },
        { 0x3E, 0xF8, 0xE3, 0x8F },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece22 = {
        { 0xF8, 0xE3, 0x8F, 0x3E },
        { 0x02, 0x08, 0x20, 0x80 },
        { 0x70, 0xC1, 0x07, 0x1C },
        { 0xF8, 0xE3, 0x8F, 0x3E },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece23 = {
        { 0x3E, 0xF8, 0xE3, 0x8F },
        { 0x80, 0x02, 0x08, 0x20 },
        { 0x1C, 0x70, 0xC1, 0x07 },
        { 0x3E, 0xF8, 0xE3, 0x8F },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece24 = {
        { 0xF8, 0xE3, 0x8F, 0x3E },
        { 0x02, 0x08, 0x20, 0x80 },
        { 0x70, 0xC1, 0x07, 0x1C },
        { 0xF8, 0xE3, 0x8F, 0x3E },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece25 = {
        { 0x3E, 0xF8, 0xE3, 0x8F },
        { 0x80, 0x02, 0x08, 0x20 },
        { 0x1C, 0x70, 0xC1, 0x7 },
        { 0x3E, 0xF8, 0xE3, 0x8F },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece26 = {
        { 0xE0, 0x83, 0x0E, 0x38 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece27 = {
        { 0x0E, 0x38, 0xE0, 0x83 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece28 = {
        { 0x20, 0x80, 0x02, 0x08 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece29 = {
        { 0x8D, 0x36, 0xD8, 0x63 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece30 = {
        { 0x63, 0x8D, 0x36, 0xD8 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece31 = {
        { 0x8, 0x20, 0x80, 0x02 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece32 = {
        { 0x77, 0xDD, 0x77, 0xDD },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece33 = {
        { 0xDD, 0x77, 0xDD, 0x77 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece34 = {
        { 0xE0, 0x83, 0x0E, 0x38 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece35 = {
        { 0x0E, 0x38, 0xE0, 0x83 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece36 = {
        { 0xE0, 0x83, 0x0E, 0x38 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece37 = {
        { 0x0E, 0x38, 0xE0, 0x83 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece38 = {
        { 0xFD, 0xF7, 0xDF, 0x7F },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece39 = {
        { 0x7F, 0xFD, 0xF7, 0xDF },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece40 = {
        { 0xF7, 0xDF, 0x7F, 0xFD },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece41 = {
        { 0xDF, 0x7F, 0xFD, 0xF7 },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece42 = {
        { 0x38, 0xE0, 0x83, 0x0E },
    };

    const std::vector<ConnectionsByRotation> kUnkFlagTrackPiece43 = {
        { 0x38, 0xE0, 0x83, 0x0E },
    };

    // 0x004F78F8
    const std::array<std::vector<ConnectionsByRotation>, 44> kUnkFlagTrackPieces = { {
        kUnkFlagTrackPiece0,
        kUnkFlagTrackPiece1,
        kUnkFlagTrackPiece2,
        kUnkFlagTrackPiece3,
        kUnkFlagTrackPiece4,
        kUnkFlagTrackPiece5,
        kUnkFlagTrackPiece6,
        kUnkFlagTrackPiece7,
        kUnkFlagTrackPiece8,
        kUnkFlagTrackPiece9,
        kUnkFlagTrackPiece10,
        kUnkFlagTrackPiece11,
        kUnkFlagTrackPiece12,
        kUnkFlagTrackPiece13,
        kUnkFlagTrackPiece14,
        kUnkFlagTrackPiece15,
        kUnkFlagTrackPiece16,
        kUnkFlagTrackPiece17,
        kUnkFlagTrackPiece18,
        kUnkFlagTrackPiece19,
        kUnkFlagTrackPiece20,
        kUnkFlagTrackPiece21,
        kUnkFlagTrackPiece22,
        kUnkFlagTrackPiece23,
        kUnkFlagTrackPiece24,
        kUnkFlagTrackPiece25,
        kUnkFlagTrackPiece26,
        kUnkFlagTrackPiece27,
        kUnkFlagTrackPiece28,
        kUnkFlagTrackPiece29,
        kUnkFlagTrackPiece30,
        kUnkFlagTrackPiece31,
        kUnkFlagTrackPiece32,
        kUnkFlagTrackPiece33,
        kUnkFlagTrackPiece34,
        kUnkFlagTrackPiece35,
        kUnkFlagTrackPiece36,
        kUnkFlagTrackPiece37,
        kUnkFlagTrackPiece38,
        kUnkFlagTrackPiece39,
        kUnkFlagTrackPiece40,
        kUnkFlagTrackPiece41,
        kUnkFlagTrackPiece42,
        kUnkFlagTrackPiece43,
    } };
}
