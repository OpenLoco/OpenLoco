#include "TrackData.h"
#include "TrackEnum.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <array>
#include <bit>
#include <cassert>

using namespace OpenLoco::World::Track;

namespace OpenLoco::World::TrackData
{
    constexpr ConnectionsByRotation generateConnections(uint8_t connection)
    {
        return {
            connection,
            std::rotl(connection, 2),
            std::rotl(connection, 4),
            std::rotl(connection, 6),
        };
    }

    const std::array<PreviewTrack, 1> trackPiece0 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 4> trackPiece1 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1001, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x07) },
        PreviewTrack{ 1, 0, 32, 0, 0, QuarterTile{ 0b0100, 0b0000 }, PreviewTrackFlags::diagonal, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 0, 0, QuarterTile{ 0b0001, 0b0000 }, PreviewTrackFlags::diagonal, generateConnections(0x08) },
        PreviewTrack{ 3, -32, 32, 0, 0, QuarterTile{ 0b0110, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x70) },
    };
    const std::array<PreviewTrack, 1> trackPiece2 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b0111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 1> trackPiece3 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 4> trackPiece4 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b0111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, 0, 0, QuarterTile{ 0b1000, 0b0000 }, PreviewTrackFlags::diagonal, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, 0, 0, QuarterTile{ 0b0010, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, 0, 0, QuarterTile{ 0b0111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> trackPiece5 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, 0, 0, QuarterTile{ 0b0100, 0b0000 }, PreviewTrackFlags::diagonal, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 0, 0, QuarterTile{ 0b0001, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, 0, 0, QuarterTile{ 0b1011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 5> trackPiece6 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 0, QuarterTile{ 0b0111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF8) },
        PreviewTrack{ 2, -32, -32, 0, 0, QuarterTile{ 0b1101, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x8F) },
        PreviewTrack{ 3, -64, -32, 0, 0, QuarterTile{ 0b0111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF8) },
        PreviewTrack{ 4, -64, -64, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 5> trackPiece7 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 0, QuarterTile{ 0b1011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x3E) },
        PreviewTrack{ 2, -32, 32, 0, 0, QuarterTile{ 0b1110, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xE3) },
        PreviewTrack{ 3, -64, 32, 0, 0, QuarterTile{ 0b1011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x3E) },
        PreviewTrack{ 4, -64, 64, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 5> trackPiece8 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 0, QuarterTile{ 0b0111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF8) },
        PreviewTrack{ 2, -32, -32, 0, 0, QuarterTile{ 0b1000, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x02) },
        PreviewTrack{ 3, -64, 0, 0, 0, QuarterTile{ 0b0010, 0b0000 }, PreviewTrackFlags::diagonal, generateConnections(0x20) },
        PreviewTrack{ 4, -64, -32, 0, 0, QuarterTile{ 0b0011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x1C) },
    };
    const std::array<PreviewTrack, 5> trackPiece9 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 0, QuarterTile{ 0b1011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x3E) },
        PreviewTrack{ 2, -32, 32, 0, 0, QuarterTile{ 0b0100, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x80) },
        PreviewTrack{ 3, -64, 0, 0, 0, QuarterTile{ 0b0001, 0b0000 }, PreviewTrackFlags::diagonal, generateConnections(0x08) },
        PreviewTrack{ 4, -64, 32, 0, 0, QuarterTile{ 0b0110, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x70) },
    };
    const std::array<PreviewTrack, 5> trackPiece10 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1001, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x07) },
        PreviewTrack{ 1, -32, 0, 0, 0, QuarterTile{ 0b0001, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x08) },
        PreviewTrack{ 2, 0, 32, 0, 0, QuarterTile{ 0b0100, 0b0000 }, PreviewTrackFlags::diagonal, generateConnections(0x80) },
        PreviewTrack{ 3, -32, 32, 0, 0, QuarterTile{ 0b1110, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xE3) },
        PreviewTrack{ 4, -64, 32, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 5> trackPiece11 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1001, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x07) },
        PreviewTrack{ 1, 0, 32, 0, 0, QuarterTile{ 0b0100, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 0, 0, QuarterTile{ 0b0001, 0b0000 }, PreviewTrackFlags::diagonal, generateConnections(0x08) },
        PreviewTrack{ 3, -32, 32, 0, 0, QuarterTile{ 0b1011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x3E) },
        PreviewTrack{ 4, -32, 64, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 4> trackPiece12 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF9) },
        PreviewTrack{ 1, -32, 0, 0, 0, QuarterTile{ 0b0110, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x60) },
        PreviewTrack{ 2, -32, -32, 0, 0, QuarterTile{ 0b1001, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x06) },
        PreviewTrack{ 3, -64, -32, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x9F) },
    };
    const std::array<PreviewTrack, 4> trackPiece13 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x3F) },
        PreviewTrack{ 1, -32, 0, 0, 0, QuarterTile{ 0b1001, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x0C) },
        PreviewTrack{ 2, -32, 32, 0, 0, QuarterTile{ 0b0110, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xC0) },
        PreviewTrack{ 3, -64, 32, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF3) },
    };
    const std::array<PreviewTrack, 2> trackPiece14 = {
        PreviewTrack{ 0, 0, 0, 0, 16, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 16, QuarterTile{ 0b1111, 0b1100 }, PreviewTrackFlags::unk4, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 2> trackPiece15 = {
        PreviewTrack{ 0, 0, 0, -16, 16, QuarterTile{ 0b1111, 0b0011 }, PreviewTrackFlags::unk4, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, -16, 16, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> trackPiece16 = {
        PreviewTrack{ 0, 0, 0, 0, 16, QuarterTile{ 0b1111, 0b1100 }, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> trackPiece17 = {
        PreviewTrack{ 0, 0, 0, -16, 16, QuarterTile{ 0b1111, 0b0011 }, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 4> trackPiece18 = {
        PreviewTrack{ 0, 0, 0, 0, 16, QuarterTile{ 0b0111, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, 0, 16, QuarterTile{ 0b1000, 0b0000 }, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, 0, 16, QuarterTile{ 0b0010, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, 0, 16, QuarterTile{ 0b0111, 0b0110 }, PreviewTrackFlags::unk4, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> trackPiece19 = {
        PreviewTrack{ 0, 0, 0, 0, 16, QuarterTile{ 0b1011, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, 0, 16, QuarterTile{ 0b0100, 0b0000 }, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 0, 16, QuarterTile{ 0b0001, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, 0, 16, QuarterTile{ 0b1011, 0b1001 }, PreviewTrackFlags::unk4, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 4> trackPiece20 = {
        PreviewTrack{ 0, 0, 0, -16, 16, QuarterTile{ 0b0111, 0b0011 }, PreviewTrackFlags::unk4, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, -16, 16, QuarterTile{ 0b1000, 0b0000 }, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, -16, 16, QuarterTile{ 0b0010, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, -16, 16, QuarterTile{ 0b0111, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> trackPiece21 = {
        PreviewTrack{ 0, 0, 0, -16, 16, QuarterTile{ 0b1011, 0b0011 }, PreviewTrackFlags::unk4, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, -16, 16, QuarterTile{ 0b0100, 0b0000 }, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, -16, 16, QuarterTile{ 0b0001, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, -16, 16, QuarterTile{ 0b1011, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 4> trackPiece22 = {
        PreviewTrack{ 0, 0, 0, 0, 16, QuarterTile{ 0b0111, 0b0100 }, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, 16, 0, QuarterTile{ 0b1000, 0b0000 }, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, 16, 0, QuarterTile{ 0b0010, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, 16, 16, QuarterTile{ 0b0111, 0b0110 }, PreviewTrackFlags::unk2 | PreviewTrackFlags::unk1, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> trackPiece23 = {
        PreviewTrack{ 0, 0, 0, 0, 16, QuarterTile{ 0b1011, 0b1000 }, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, 16, 0, QuarterTile{ 0b0100, 0b0000 }, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 16, 0, QuarterTile{ 0b0001, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, 16, 16, QuarterTile{ 0b1011, 0b1001 }, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk0, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 4> trackPiece24 = {
        PreviewTrack{ 0, 0, 0, -16, 16, QuarterTile{ 0b0111, 0b0011 }, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, -16, 0, QuarterTile{ 0b1000, 0b0000 }, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, -16, 0, QuarterTile{ 0b0010, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, -32, 16, QuarterTile{ 0b0111, 0b0001 }, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk0, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> trackPiece25 = {
        PreviewTrack{ 0, 0, 0, -16, 16, QuarterTile{ 0b1011, 0b0011 }, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, -16, 0, QuarterTile{ 0b0100, 0b0000 }, PreviewTrackFlags::diagonal | PreviewTrackFlags::unk4, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, -16, 0, QuarterTile{ 0b0001, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, -32, 16, QuarterTile{ 0b1011, 0b0010 }, PreviewTrackFlags::unk2 | PreviewTrackFlags::unk1, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 1> trackPiece26 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b0110, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xE0) },
    };
    const std::array<PreviewTrack, 1> trackPiece27 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1001, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x0E) },
    };
    const std::array<PreviewTrack, 1> trackPiece28 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b0010, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x20) },
    };
    const std::array<PreviewTrack, 1> trackPiece29 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x8D) },
    };
    const std::array<PreviewTrack, 1> trackPiece30 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x63) },
    };
    const std::array<PreviewTrack, 1> trackPiece31 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b0001, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x08) },
    };
    const std::array<PreviewTrack, 1> trackPiece32 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x77) },
    };
    const std::array<PreviewTrack, 1> trackPiece33 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xDD) },
    };
    const std::array<PreviewTrack, 1> trackPiece34 = {
        PreviewTrack{ 0, 0, 0, 0, 16, QuarterTile{ 0b0110, 0b1100 }, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0xE0) },
    };
    const std::array<PreviewTrack, 1> trackPiece35 = {
        PreviewTrack{ 0, 0, 0, 0, 16, QuarterTile{ 0b1001, 0b1100 }, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0x0E) },
    };
    const std::array<PreviewTrack, 1> trackPiece36 = {
        PreviewTrack{ 0, 0, 0, -16, 16, QuarterTile{ 0b0110, 0b0011 }, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0xE0) },
    };
    const std::array<PreviewTrack, 1> trackPiece37 = {
        PreviewTrack{ 0, 0, 0, -16, 16, QuarterTile{ 0b1001, 0b0011 }, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0x0E) },
    };
    const std::array<PreviewTrack, 1> trackPiece38 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xFD) },
    };
    const std::array<PreviewTrack, 1> trackPiece39 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x7F) },
    };
    const std::array<PreviewTrack, 1> trackPiece40 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF7) },
    };
    const std::array<PreviewTrack, 1> trackPiece41 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xDF) },
    };
    const std::array<PreviewTrack, 1> trackPiece42 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b0011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x38) },
    };
    const std::array<PreviewTrack, 1> trackPiece43 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b0011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x38) },
    };

    // 0x004F73D8, 0x004F78F8
    const std::array<std::span<const PreviewTrack>, 44> trackPieces = { {
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
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> roadPiece1 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 1> roadPiece2 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 4> roadPiece3 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b0111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF8) },
        PreviewTrack{ 1, 0, -32, 0, 0, QuarterTile{ 0b1000, 0b0000 }, PreviewTrackFlags::diagonal, generateConnections(0x02) },
        PreviewTrack{ 2, -32, 0, 0, 0, QuarterTile{ 0b0010, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x70) },
        PreviewTrack{ 3, -32, -32, 0, 0, QuarterTile{ 0b0111, 0b0000 }, PreviewTrackFlags::none, generateConnections(0xF8) },
    };
    const std::array<PreviewTrack, 4> roadPiece4 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b1011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x3E) },
        PreviewTrack{ 1, 0, 32, 0, 0, QuarterTile{ 0b0100, 0b0000 }, PreviewTrackFlags::diagonal, generateConnections(0x80) },
        PreviewTrack{ 2, -32, 0, 0, 0, QuarterTile{ 0b0001, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x1C) },
        PreviewTrack{ 3, -32, 32, 0, 0, QuarterTile{ 0b1011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x3E) },
    };
    const std::array<PreviewTrack, 2> roadPiece5 = {
        PreviewTrack{ 0, 0, 0, 0, 16, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, 0, 16, QuarterTile{ 0b1111, 0b1100 }, PreviewTrackFlags::unk4, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 2> roadPiece6 = {
        PreviewTrack{ 0, 0, 0, -16, 16, QuarterTile{ 0b1111, 0b0011 }, PreviewTrackFlags::unk4, generateConnections(0xFF) },
        PreviewTrack{ 1, -32, 0, -16, 16, QuarterTile{ 0b1111, 0b0000 }, PreviewTrackFlags::unk4, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> roadPiece7 = {
        PreviewTrack{ 0, 0, 0, 0, 16, QuarterTile{ 0b1111, 0b1100 }, PreviewTrackFlags::unk3 | PreviewTrackFlags::unk2, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> roadPiece8 = {
        PreviewTrack{ 0, 0, 0, -16, 16, QuarterTile{ 0b1111, 0b0011 }, PreviewTrackFlags::unk1 | PreviewTrackFlags::unk0, generateConnections(0xFF) },
    };
    const std::array<PreviewTrack, 1> roadPiece9 = {
        PreviewTrack{ 0, 0, 0, 0, 0, QuarterTile{ 0b0011, 0b0000 }, PreviewTrackFlags::none, generateConnections(0x38) },
    };

    // 0x004F6D1C, 0x004F6F1C
    const std::array<std::span<const PreviewTrack>, 10> roadPieces = { {
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

    const std::span<const PreviewTrack> getTrackPiece(size_t trackId)
    {
        assert(trackId < trackPieces.size());
        return trackPieces[trackId];
    }

    const std::span<const PreviewTrack> getRoadPiece(size_t trackId)
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

    // 0x004F891C
    constexpr std::array<uint16_t, 44> kTrackCompatibleFlags = {
        0x0000,
        0x0001,
        0x0010,
        0x0010,
        0x0008,
        0x0008,
        0x0004,
        0x0004,
        0x0002,
        0x0002,
        0x0002,
        0x0002,
        0x0200,
        0x0200,
        0x0020,
        0x0020,
        0x0040,
        0x0040,
        0x0128,
        0x0128,
        0x0128,
        0x0128,
        0x0148,
        0x0148,
        0x0148,
        0x0148,
        0x0080,
        0x0080,
        0x0090,
        0x0090,
        0x0090,
        0x0090,
        0x0080,
        0x0080,
        0x00C0,
        0x00C0,
        0x00C0,
        0x00C0,
        0x0080,
        0x0080,
        0x0080,
        0x0080,
        0x0080,
        0x0080,
    };

    static Interop::loco_global<uint16_t[44], 0x004F870C> _costFactor;
    static Interop::loco_global<uint16_t[44], 0x004F8764> _flags;
    static Interop::loco_global<uint8_t[44 * 8], 0x004F87BC> _variousThings;
    static Interop::loco_global<uint16_t[44], 0x004F891C> _compatibleFlags;
    static Interop::loco_global<uint16_t[44], 0x004F8974> _curveSpeedFraction;
    static Interop::loco_global<uint32_t[44], 0x004F89CC> _unkWeighting;
    static Interop::loco_global<bool[44], 0x004F8A7C> _sparkDirection;

    constexpr std::array<MiscData, 44> _miscData = {
        MiscData{
            .costFactor = 0x100,
            .flags = MiscFlags::none,
            .reverseTrackId = 0,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::none,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 32,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x16A,
            .flags = MiscFlags::diagonal,
            .reverseTrackId = 1,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::diagonal,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 45,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x0C9,
            .flags = MiscFlags::verySmallCurve,
            .reverseTrackId = 3,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::verySmallCurve,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 25,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x0C9,
            .flags = MiscFlags::verySmallCurve,
            .reverseTrackId = 2,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::verySmallCurve,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 25,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x25B,
            .flags = MiscFlags::smallCurve,
            .reverseTrackId = 5,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::smallCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 75,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x25B,
            .flags = MiscFlags::smallCurve,
            .reverseTrackId = 4,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::smallCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 75,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x3ED,
            .flags = MiscFlags::curve,
            .reverseTrackId = 7,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::normalCurve,
            .curveSpeedFraction = 0x2666,
            .unkWeighting = 126,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x3ED,
            .flags = MiscFlags::curve,
            .reverseTrackId = 6,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::normalCurve,
            .curveSpeedFraction = 0x2666,
            .unkWeighting = 126,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x2BF,
            .flags = MiscFlags::largeCurve,
            .reverseTrackId = 11,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::largeCurve,
            .curveSpeedFraction = 0x4000,
            .unkWeighting = 88,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x2BF,
            .flags = MiscFlags::largeCurve,
            .reverseTrackId = 10,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::largeCurve,
            .curveSpeedFraction = 0x4000,
            .unkWeighting = 88,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x2BF,
            .flags = MiscFlags::largeCurve,
            .reverseTrackId = 9,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::largeCurve,
            .curveSpeedFraction = 0x4000,
            .unkWeighting = 88,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x2BF,
            .flags = MiscFlags::largeCurve,
            .reverseTrackId = 8,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::largeCurve,
            .curveSpeedFraction = 0x4000,
            .unkWeighting = 88,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x380,
            .flags = MiscFlags::sBendCurve,
            .reverseTrackId = 12,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::sBend,
            .curveSpeedFraction = 0x2666,
            .unkWeighting = 106,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x380,
            .flags = MiscFlags::sBendCurve,
            .reverseTrackId = 13,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::sBend,
            .curveSpeedFraction = 0x2666,
            .unkWeighting = 106,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x220,
            .flags = MiscFlags::slope,
            .reverseTrackId = 15,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 1,
            .signalHeightOffsetRight = 15,
            .compatibleFlags = TrackPieceFlags::slope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 66,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x220,
            .flags = MiscFlags::slope,
            .reverseTrackId = 14,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 15,
            .signalHeightOffsetRight = 1,
            .compatibleFlags = TrackPieceFlags::slope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 66,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x138,
            .flags = MiscFlags::steepSlope,
            .reverseTrackId = 17,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 2,
            .signalHeightOffsetRight = 14,
            .compatibleFlags = TrackPieceFlags::steepSlope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x138,
            .flags = MiscFlags::steepSlope,
            .reverseTrackId = 16,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 14,
            .signalHeightOffsetRight = 2,
            .compatibleFlags = TrackPieceFlags::steepSlope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x350,
            .flags = MiscFlags::slope | MiscFlags::curveSlope | MiscFlags::smallCurve,
            .reverseTrackId = 21,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 1,
            .signalHeightOffsetRight = 15,
            .compatibleFlags = TrackPieceFlags::smallCurve | TrackPieceFlags::slope | TrackPieceFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 77,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x350,
            .flags = MiscFlags::slope | MiscFlags::curveSlope | MiscFlags::smallCurve,
            .reverseTrackId = 20,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 1,
            .signalHeightOffsetRight = 15,
            .compatibleFlags = TrackPieceFlags::smallCurve | TrackPieceFlags::slope | TrackPieceFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 77,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x350,
            .flags = MiscFlags::slope | MiscFlags::curveSlope | MiscFlags::smallCurve,
            .reverseTrackId = 19,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 15,
            .signalHeightOffsetRight = 1,
            .compatibleFlags = TrackPieceFlags::smallCurve | TrackPieceFlags::slope | TrackPieceFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 77,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x350,
            .flags = MiscFlags::slope | MiscFlags::curveSlope | MiscFlags::smallCurve,
            .reverseTrackId = 18,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 15,
            .signalHeightOffsetRight = 1,
            .compatibleFlags = TrackPieceFlags::smallCurve | TrackPieceFlags::slope | TrackPieceFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 77,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x41F,
            .flags = MiscFlags::steepSlope | MiscFlags::curveSlope | MiscFlags::smallCurve,
            .reverseTrackId = 25,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 2,
            .signalHeightOffsetRight = 14,
            .compatibleFlags = TrackPieceFlags::smallCurve | TrackPieceFlags::steepSlope | TrackPieceFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 82,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x41F,
            .flags = MiscFlags::steepSlope | MiscFlags::curveSlope | MiscFlags::smallCurve,
            .reverseTrackId = 24,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 2,
            .signalHeightOffsetRight = 14,
            .compatibleFlags = TrackPieceFlags::smallCurve | TrackPieceFlags::steepSlope | TrackPieceFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 82,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x41F,
            .flags = MiscFlags::steepSlope | MiscFlags::curveSlope | MiscFlags::smallCurve,
            .reverseTrackId = 23,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 14,
            .signalHeightOffsetRight = 2,
            .compatibleFlags = TrackPieceFlags::smallCurve | TrackPieceFlags::steepSlope | TrackPieceFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 82,
            .sparkDirection = true },
        MiscData{
            .costFactor = 0x41F,
            .flags = MiscFlags::steepSlope | MiscFlags::curveSlope | MiscFlags::smallCurve,
            .reverseTrackId = 22,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 14,
            .signalHeightOffsetRight = 2,
            .compatibleFlags = TrackPieceFlags::smallCurve | TrackPieceFlags::steepSlope | TrackPieceFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 82,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x100,
            .flags = MiscFlags::oneSided,
            .reverseTrackId = 27,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 32,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x100,
            .flags = MiscFlags::oneSided,
            .reverseTrackId = 26,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 32,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x064,
            .flags = MiscFlags::verySmallCurve | MiscFlags::oneSided,
            .reverseTrackId = 31,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::verySmallCurve | TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 13,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x12D,
            .flags = MiscFlags::verySmallCurve | MiscFlags::oneSided,
            .reverseTrackId = 30,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::verySmallCurve | TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 38,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x12D,
            .flags = MiscFlags::verySmallCurve | MiscFlags::oneSided,
            .reverseTrackId = 29,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::verySmallCurve | TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 38,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x064,
            .flags = MiscFlags::verySmallCurve | MiscFlags::oneSided,
            .reverseTrackId = 28,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::verySmallCurve | TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 13,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x126,
            .flags = MiscFlags::sBendCurve | MiscFlags::oneSided,
            .reverseTrackId = 32,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 36,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x126,
            .flags = MiscFlags::sBendCurve | MiscFlags::oneSided,
            .reverseTrackId = 33,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 36,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x138,
            .flags = MiscFlags::steepSlope | MiscFlags::oneSided,
            .reverseTrackId = 37,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 2,
            .signalHeightOffsetRight = 14,
            .compatibleFlags = TrackPieceFlags::steepSlope | TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x138,
            .flags = MiscFlags::steepSlope | MiscFlags::oneSided,
            .reverseTrackId = 36,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 2,
            .signalHeightOffsetRight = 14,
            .compatibleFlags = TrackPieceFlags::steepSlope | TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x138,
            .flags = MiscFlags::steepSlope | MiscFlags::oneSided,
            .reverseTrackId = 35,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 14,
            .signalHeightOffsetRight = 2,
            .compatibleFlags = TrackPieceFlags::steepSlope | TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x138,
            .flags = MiscFlags::steepSlope | MiscFlags::oneSided,
            .reverseTrackId = 34,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 14,
            .signalHeightOffsetRight = 2,
            .compatibleFlags = TrackPieceFlags::steepSlope | TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x114,
            .flags = MiscFlags::sBendCurve | MiscFlags::oneSided,
            .reverseTrackId = 41,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 34,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x114,
            .flags = MiscFlags::sBendCurve | MiscFlags::oneSided,
            .reverseTrackId = 40,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 34,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x114,
            .flags = MiscFlags::sBendCurve | MiscFlags::oneSided,
            .reverseTrackId = 39,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 34,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x114,
            .flags = MiscFlags::sBendCurve | MiscFlags::oneSided,
            .reverseTrackId = 38,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 34,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x25B,
            .flags = MiscFlags::verySmallCurve | MiscFlags::oneSided,
            .reverseTrackId = 43,
            .reverseRotation = 0,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 26,
            .sparkDirection = false },
        MiscData{
            .costFactor = 0x25B,
            .flags = MiscFlags::verySmallCurve | MiscFlags::oneSided,
            .reverseTrackId = 42,
            .reverseRotation = 0,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackPieceFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 26,
            .sparkDirection = false },
    };

    std::array<std::string, 11> _pieceFlagsToString{
        "TrackPieceFlags::diagonal",
        "TrackPieceFlags::largeCurve",
        "TrackPieceFlags::normalCurve",
        "TrackPieceFlags::smallCurve",
        "TrackPieceFlags::verySmallCurve",
        "TrackPieceFlags::slope",
        "TrackPieceFlags::steepSlope",
        "TrackPieceFlags::oneSided",
        "TrackPieceFlags::slopedCurve",
        "TrackPieceFlags::sBend",
        "TrackPieceFlags::junction",
    };

    std::array<std::string, 10> _miscToString{
        "MiscFlags::slope",
        "MiscFlags::steepSlope",
        "MiscFlags::curveSlope",
        "MiscFlags::diagonal",
        "MiscFlags::verySmallCurve",
        "MiscFlags::smallCurve",
        "MiscFlags::curve",
        "MiscFlags::largeCurve",
        "MiscFlags::sBendCurve",
        "MiscFlags::oneSided",
    };

    std::string toMiscString(uint16_t val)
    {
        std::string out;
        for (auto i = 0; i < 10; ++i)
        {
            if (val & (1U << i))
            {
                if (out.empty())
                {
                    out = _miscToString[i];
                }
                else
                {
                    out += " | " + _miscToString[i];
                }
            }
        }
        if (out.empty())
        {
            out = "MiscFlags::none";
        }
        return out;
    }
    std::string toTrackPieceString(uint16_t val)
    {
        std::string out;
        for (auto i = 0; i < 11; ++i)
        {
            if (val & (1U << i))
            {
                if (out.empty())
                {
                    out = _pieceFlagsToString[i];
                }
                else
                {
                    out += " | " + _pieceFlagsToString[i];
                }
            }
        }
        if (out.empty())
        {
            out = "TrackPieceFlags::none";
        }
        return out;
    }

    const MiscData& getTrackMiscData(size_t trackId)
    {
        return _miscData[trackId];
    }

    uint16_t getTrackCompatibleFlags(size_t trackId)
    {
        for (auto i = 0; i < 44; ++i)
        {
            MiscData data{};
            data.costFactor = _costFactor[i];
            data.flags = static_cast<MiscFlags>(_flags[i]);
            data.reverseTrackId = _variousThings[i * 8 + 0];
            data.reverseRotation = _variousThings[i * 8 + 1];
            data.signalHeightOffsetLeft = _variousThings[i * 8 + 2];
            data.signalHeightOffsetRight = _variousThings[i * 8 + 3];
            data.compatibleFlags = static_cast<TrackPieceFlags>(_compatibleFlags[i]);
            data.curveSpeedFraction = _curveSpeedFraction[i];
            data.unkWeighting = _unkWeighting[i];
            data.sparkDirection = _sparkDirection[i];

            fmt::println("MiscData {{");
            fmt::println(".costFactor = 0x{:03X},\n.flags = {},\n.reverseTrackId = {},", data.costFactor, toMiscString(enumValue(data.flags)), data.reverseTrackId);
            fmt::println(".reverseRotation = {},\n.signalHeightOffsetLeft = {},\n.signalHeightOffsetRight = {},", data.reverseRotation, data.signalHeightOffsetLeft, data.signalHeightOffsetRight);
            fmt::println(".compatibleFlags = {},\n.curveSpeedFraction = 0x{:04X},\n.unkWeighting = {},\n.sparkDirection = {}", toTrackPieceString(enumValue(data.compatibleFlags)), data.curveSpeedFraction, data.unkWeighting, data.sparkDirection);
            fmt::println("}},");
        }
        return kTrackCompatibleFlags[trackId];
    }

    // 0x004F870C
    constexpr std::array<uint16_t, 44> kTrackCostFactor = {
        0x100,
        0x16A,
        0x0C9,
        0x0C9,
        0x25B,
        0x25B,
        0x3ED,
        0x3ED,
        0x2BF,
        0x2BF,
        0x2BF,
        0x2BF,
        0x380,
        0x380,
        0x220,
        0x220,
        0x138,
        0x138,
        0x350,
        0x350,
        0x350,
        0x350,
        0x41F,
        0x41F,
        0x41F,
        0x41F,
        0x100,
        0x100,
        0x064,
        0x12D,
        0x12D,
        0x064,
        0x126,
        0x126,
        0x138,
        0x138,
        0x138,
        0x138,
        0x114,
        0x114,
        0x114,
        0x114,
        0x25B,
        0x25B,
    };

    uint16_t getTrackCostFactor(size_t trackId)
    {
        return kTrackCostFactor[trackId];
    }

    // 0x004F72E8
    constexpr std::array<uint16_t, 10> kRoadCompatibleFlags = {
        0x00,
        0x02,
        0x02,
        0x01,
        0x01,
        0x04,
        0x04,
        0x08,
        0x08,
        0x20,
    };

    uint16_t getRoadCompatibleFlags(size_t roadId)
    {
        return kRoadCompatibleFlags[roadId];
    }

    constexpr std::array<uint16_t, 10> kRoadCostFactor = {
        0x100,
        0x0C9,
        0x0C9,
        0x25B,
        0x25B,
        0x220,
        0x220,
        0x138,
        0x138,
        0x25B,
    };

    uint16_t getRoadCostFactor(size_t roadId)
    {
        return kRoadCostFactor[roadId];
    }
}
