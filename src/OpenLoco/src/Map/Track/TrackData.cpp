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

    // 0x4F6F8C
    static constexpr std::array<TrackCoordinates, 80> kRoadCoordinates = { {
        { 0, 0, { -32, 0, 0 } },
        { 1, 1, { 0, 32, 0 } },
        { 2, 2, { 32, 0, 0 } },
        { 3, 3, { 0, -32, 0 } },
        { 2, 2, { 32, 0, 0 } },
        { 3, 3, { 0, -32, 0 } },
        { 0, 0, { -32, 0, 0 } },
        { 1, 1, { 0, 32, 0 } },
        { 0, 3, { 0, -32, 0 } },
        { 1, 0, { -32, 0, 0 } },
        { 2, 1, { 0, 32, 0 } },
        { 3, 2, { 32, 0, 0 } },
        { 1, 2, { 32, 0, 0 } },
        { 2, 3, { 0, -32, 0 } },
        { 3, 0, { -32, 0, 0 } },
        { 0, 1, { 0, 32, 0 } },
        { 0, 1, { 0, 32, 0 } },
        { 1, 2, { 32, 0, 0 } },
        { 2, 3, { 0, -32, 0 } },
        { 3, 0, { -32, 0, 0 } },
        { 3, 2, { 32, 0, 0 } },
        { 0, 3, { 0, -32, 0 } },
        { 1, 0, { -32, 0, 0 } },
        { 2, 1, { 0, 32, 0 } },
        { 0, 3, { -32, -64, 0 } },
        { 1, 0, { -64, 32, 0 } },
        { 2, 1, { 32, 64, 0 } },
        { 3, 2, { 64, -32, 0 } },
        { 1, 2, { 64, 32, 0 } },
        { 2, 3, { 32, -64, 0 } },
        { 3, 0, { -64, -32, 0 } },
        { 0, 1, { -32, 64, 0 } },
        { 0, 1, { -32, 64, 0 } },
        { 1, 2, { 64, 32, 0 } },
        { 2, 3, { 32, -64, 0 } },
        { 3, 0, { -64, -32, 0 } },
        { 3, 2, { 64, -32, 0 } },
        { 0, 3, { -32, -64, 0 } },
        { 1, 0, { -64, 32, 0 } },
        { 2, 1, { 32, 64, 0 } },
        { 0, 0, { -64, 0, 16 } },
        { 1, 1, { 0, 64, 16 } },
        { 2, 2, { 64, 0, 16 } },
        { 3, 3, { 0, -64, 16 } },
        { 2, 2, { 64, 0, -16 } },
        { 3, 3, { 0, -64, -16 } },
        { 0, 0, { -64, 0, -16 } },
        { 1, 1, { 0, 64, -16 } },
        { 0, 0, { -64, 0, -16 } },
        { 1, 1, { 0, 64, -16 } },
        { 2, 2, { 64, 0, -16 } },
        { 3, 3, { 0, -64, -16 } },
        { 2, 2, { 64, 0, 16 } },
        { 3, 3, { 0, -64, 16 } },
        { 0, 0, { -64, 0, 16 } },
        { 1, 1, { 0, 64, 16 } },
        { 0, 0, { -32, 0, 16 } },
        { 1, 1, { 0, 32, 16 } },
        { 2, 2, { 32, 0, 16 } },
        { 3, 3, { 0, -32, 16 } },
        { 2, 2, { 32, 0, -16 } },
        { 3, 3, { 0, -32, -16 } },
        { 0, 0, { -32, 0, -16 } },
        { 1, 1, { 0, 32, -16 } },
        { 0, 0, { -32, 0, -16 } },
        { 1, 1, { 0, 32, -16 } },
        { 2, 2, { 32, 0, -16 } },
        { 3, 3, { 0, -32, -16 } },
        { 2, 2, { 32, 0, 16 } },
        { 3, 3, { 0, -32, 16 } },
        { 0, 0, { -32, 0, 16 } },
        { 1, 1, { 0, 32, 16 } },
        { 0, 2, { 32, 0, 0 } },
        { 1, 3, { 0, -32, 0 } },
        { 2, 0, { -32, 0, 0 } },
        { 3, 1, { 0, 32, 0 } },
        { 0, 2, { 32, 0, 0 } },
        { 1, 3, { 0, -32, 0 } },
        { 2, 0, { -32, 0, 0 } },
        { 3, 1, { 0, 32, 0 } },
    } };

    // 0x4F7B5C
    static constexpr std::array<TrackCoordinates, 352> kTrackCoordinates = { {
        { 0, 0, { -32, 0, 0 } },
        { 1, 1, { 0, 32, 0 } },
        { 2, 2, { 32, 0, 0 } },
        { 3, 3, { 0, -32, 0 } },
        { 2, 2, { 32, 0, 0 } },
        { 3, 3, { 0, -32, 0 } },
        { 0, 0, { -32, 0, 0 } },
        { 1, 1, { 0, 32, 0 } },
        { 12, 12, { -32, 32, 0 } },
        { 13, 13, { 32, 32, 0 } },
        { 14, 14, { 32, -32, 0 } },
        { 15, 15, { -32, -32, 0 } },
        { 14, 14, { 32, -32, 0 } },
        { 15, 15, { -32, -32, 0 } },
        { 12, 12, { -32, 32, 0 } },
        { 13, 13, { 32, 32, 0 } },
        { 0, 3, { 0, -32, 0 } },
        { 1, 0, { -32, 0, 0 } },
        { 2, 1, { 0, 32, 0 } },
        { 3, 2, { 32, 0, 0 } },
        { 1, 2, { 32, 0, 0 } },
        { 2, 3, { 0, -32, 0 } },
        { 3, 0, { -32, 0, 0 } },
        { 0, 1, { 0, 32, 0 } },
        { 0, 1, { 0, 32, 0 } },
        { 1, 2, { 32, 0, 0 } },
        { 2, 3, { 0, -32, 0 } },
        { 3, 0, { -32, 0, 0 } },
        { 3, 2, { 32, 0, 0 } },
        { 0, 3, { 0, -32, 0 } },
        { 1, 0, { -32, 0, 0 } },
        { 2, 1, { 0, 32, 0 } },
        { 0, 3, { -32, -64, 0 } },
        { 1, 0, { -64, 32, 0 } },
        { 2, 1, { 32, 64, 0 } },
        { 3, 2, { 64, -32, 0 } },
        { 1, 2, { 64, 32, 0 } },
        { 2, 3, { 32, -64, 0 } },
        { 3, 0, { -64, -32, 0 } },
        { 0, 1, { -32, 64, 0 } },
        { 0, 1, { -32, 64, 0 } },
        { 1, 2, { 64, 32, 0 } },
        { 2, 3, { 32, -64, 0 } },
        { 3, 0, { -64, -32, 0 } },
        { 3, 2, { 64, -32, 0 } },
        { 0, 3, { -32, -64, 0 } },
        { 1, 0, { -64, 32, 0 } },
        { 2, 1, { 32, 64, 0 } },
        { 0, 3, { -64, -96, 0 } },
        { 1, 0, { -96, 64, 0 } },
        { 2, 1, { 64, 96, 0 } },
        { 3, 2, { 96, -64, 0 } },
        { 1, 2, { 96, 64, 0 } },
        { 2, 3, { 64, -96, 0 } },
        { 3, 0, { -96, -64, 0 } },
        { 0, 1, { -64, 96, 0 } },
        { 0, 1, { -64, 96, 0 } },
        { 1, 2, { 96, 64, 0 } },
        { 2, 3, { 64, -96, 0 } },
        { 3, 0, { -96, -64, 0 } },
        { 3, 2, { 96, -64, 0 } },
        { 0, 3, { -64, -96, 0 } },
        { 1, 0, { -96, 64, 0 } },
        { 2, 1, { 64, 96, 0 } },
        { 0, 15, { -64, -32, 0 } },
        { 1, 12, { -32, 64, 0 } },
        { 2, 13, { 64, 32, 0 } },
        { 3, 14, { 32, -64, 0 } },
        { 13, 2, { 96, 32, 0 } },
        { 14, 3, { 32, -96, 0 } },
        { 15, 0, { -96, -32, 0 } },
        { 12, 1, { -32, 96, 0 } },
        { 0, 12, { -64, 32, 0 } },
        { 1, 13, { 32, 64, 0 } },
        { 2, 14, { 64, -32, 0 } },
        { 3, 15, { -32, -64, 0 } },
        { 14, 2, { 96, -32, 0 } },
        { 15, 3, { -32, -96, 0 } },
        { 12, 0, { -96, 32, 0 } },
        { 13, 1, { 32, 96, 0 } },
        { 12, 0, { -96, 32, 0 } },
        { 13, 1, { 32, 96, 0 } },
        { 14, 2, { 96, -32, 0 } },
        { 15, 3, { -32, -96, 0 } },
        { 2, 14, { 64, -32, 0 } },
        { 3, 15, { -32, -64, 0 } },
        { 0, 12, { -64, 32, 0 } },
        { 1, 13, { 32, 64, 0 } },
        { 12, 1, { -32, 96, 0 } },
        { 13, 2, { 96, 32, 0 } },
        { 14, 3, { 32, -96, 0 } },
        { 15, 0, { -96, -32, 0 } },
        { 3, 14, { 32, -64, 0 } },
        { 0, 15, { -64, -32, 0 } },
        { 1, 12, { -32, 64, 0 } },
        { 2, 13, { 64, 32, 0 } },
        { 0, 0, { -96, -32, 0 } },
        { 1, 1, { -32, 96, 0 } },
        { 2, 2, { 96, 32, 0 } },
        { 3, 3, { 32, -96, 0 } },
        { 2, 2, { 96, 32, 0 } },
        { 3, 3, { 32, -96, 0 } },
        { 0, 0, { -96, -32, 0 } },
        { 1, 1, { -32, 96, 0 } },
        { 0, 0, { -96, 32, 0 } },
        { 1, 1, { 32, 96, 0 } },
        { 2, 2, { 96, -32, 0 } },
        { 3, 3, { -32, -96, 0 } },
        { 2, 2, { 96, -32, 0 } },
        { 3, 3, { -32, -96, 0 } },
        { 0, 0, { -96, 32, 0 } },
        { 1, 1, { 32, 96, 0 } },
        { 0, 0, { -64, 0, 16 } },
        { 1, 1, { 0, 64, 16 } },
        { 2, 2, { 64, 0, 16 } },
        { 3, 3, { 0, -64, 16 } },
        { 2, 2, { 64, 0, -16 } },
        { 3, 3, { 0, -64, -16 } },
        { 0, 0, { -64, 0, -16 } },
        { 1, 1, { 0, 64, -16 } },
        { 0, 0, { -64, 0, -16 } },
        { 1, 1, { 0, 64, -16 } },
        { 2, 2, { 64, 0, -16 } },
        { 3, 3, { 0, -64, -16 } },
        { 2, 2, { 64, 0, 16 } },
        { 3, 3, { 0, -64, 16 } },
        { 0, 0, { -64, 0, 16 } },
        { 1, 1, { 0, 64, 16 } },
        { 0, 0, { -32, 0, 16 } },
        { 1, 1, { 0, 32, 16 } },
        { 2, 2, { 32, 0, 16 } },
        { 3, 3, { 0, -32, 16 } },
        { 2, 2, { 32, 0, -16 } },
        { 3, 3, { 0, -32, -16 } },
        { 0, 0, { -32, 0, -16 } },
        { 1, 1, { 0, 32, -16 } },
        { 0, 0, { -32, 0, -16 } },
        { 1, 1, { 0, 32, -16 } },
        { 2, 2, { 32, 0, -16 } },
        { 3, 3, { 0, -32, -16 } },
        { 2, 2, { 32, 0, 16 } },
        { 3, 3, { 0, -32, 16 } },
        { 0, 0, { -32, 0, 16 } },
        { 1, 1, { 0, 32, 16 } },
        { 0, 3, { -32, -64, 16 } },
        { 1, 0, { -64, 32, 16 } },
        { 2, 1, { 32, 64, 16 } },
        { 3, 2, { 64, -32, 16 } },
        { 1, 2, { 64, 32, -16 } },
        { 2, 3, { 32, -64, -16 } },
        { 3, 0, { -64, -32, -16 } },
        { 0, 1, { -32, 64, -16 } },
        { 0, 1, { -32, 64, 16 } },
        { 1, 2, { 64, 32, 16 } },
        { 2, 3, { 32, -64, 16 } },
        { 3, 0, { -64, -32, 16 } },
        { 3, 2, { 64, -32, -16 } },
        { 0, 3, { -32, -64, -16 } },
        { 1, 0, { -64, 32, -16 } },
        { 2, 1, { 32, 64, -16 } },
        { 0, 3, { -32, -64, -16 } },
        { 1, 0, { -64, 32, -16 } },
        { 2, 1, { 32, 64, -16 } },
        { 3, 2, { 64, -32, -16 } },
        { 1, 2, { 64, 32, 16 } },
        { 2, 3, { 32, -64, 16 } },
        { 3, 0, { -64, -32, 16 } },
        { 0, 1, { -32, 64, 16 } },
        { 0, 1, { -32, 64, -16 } },
        { 1, 2, { 64, 32, -16 } },
        { 2, 3, { 32, -64, -16 } },
        { 3, 0, { -64, -32, -16 } },
        { 3, 2, { 64, -32, 16 } },
        { 0, 3, { -32, -64, 16 } },
        { 1, 0, { -64, 32, 16 } },
        { 2, 1, { 32, 64, 16 } },
        { 0, 3, { -32, -64, 32 } },
        { 1, 0, { -64, 32, 32 } },
        { 2, 1, { 32, 64, 32 } },
        { 3, 2, { 64, -32, 32 } },
        { 1, 2, { 64, 32, -32 } },
        { 2, 3, { 32, -64, -32 } },
        { 3, 0, { -64, -32, -32 } },
        { 0, 1, { -32, 64, -32 } },
        { 0, 1, { -32, 64, 32 } },
        { 1, 2, { 64, 32, 32 } },
        { 2, 3, { 32, -64, 32 } },
        { 3, 0, { -64, -32, 32 } },
        { 3, 2, { 64, -32, -32 } },
        { 0, 3, { -32, -64, -32 } },
        { 1, 0, { -64, 32, -32 } },
        { 2, 1, { 32, 64, -32 } },
        { 0, 3, { -32, -64, -32 } },
        { 1, 0, { -64, 32, -32 } },
        { 2, 1, { 32, 64, -32 } },
        { 3, 2, { 64, -32, -32 } },
        { 1, 2, { 64, 32, 32 } },
        { 2, 3, { 32, -64, 32 } },
        { 3, 0, { -64, -32, 32 } },
        { 0, 1, { -32, 64, 32 } },
        { 0, 1, { -32, 64, -32 } },
        { 1, 2, { 64, 32, -32 } },
        { 2, 3, { 32, -64, -32 } },
        { 3, 0, { -64, -32, -32 } },
        { 3, 2, { 64, -32, 32 } },
        { 0, 3, { -32, -64, 32 } },
        { 1, 0, { -64, 32, 32 } },
        { 2, 1, { 32, 64, 32 } },
        { 4, 4, { -32, 0, 0 } },
        { 5, 5, { 0, 32, 0 } },
        { 6, 6, { 32, 0, 0 } },
        { 7, 7, { 0, -32, 0 } },
        { 10, 10, { 32, 0, 0 } },
        { 11, 11, { 0, -32, 0 } },
        { 8, 8, { -32, 0, 0 } },
        { 9, 9, { 0, 32, 0 } },
        { 8, 8, { -32, 0, 0 } },
        { 9, 9, { 0, 32, 0 } },
        { 10, 10, { 32, 0, 0 } },
        { 11, 11, { 0, -32, 0 } },
        { 6, 6, { 32, 0, 0 } },
        { 7, 7, { 0, -32, 0 } },
        { 4, 4, { -32, 0, 0 } },
        { 5, 5, { 0, 32, 0 } },
        { 4, 7, { 0, -32, 0 } },
        { 5, 4, { -32, 0, 0 } },
        { 6, 5, { 0, 32, 0 } },
        { 7, 6, { 32, 0, 0 } },
        { 9, 10, { 32, 0, 0 } },
        { 10, 11, { 0, -32, 0 } },
        { 11, 8, { -32, 0, 0 } },
        { 8, 9, { 0, 32, 0 } },
        { 8, 11, { 0, -32, 0 } },
        { 9, 8, { -32, 0, 0 } },
        { 10, 9, { 0, 32, 0 } },
        { 11, 10, { 32, 0, 0 } },
        { 5, 6, { 32, 0, 0 } },
        { 6, 7, { 0, -32, 0 } },
        { 7, 4, { -32, 0, 0 } },
        { 4, 5, { 0, 32, 0 } },
        { 4, 5, { 0, 32, 0 } },
        { 5, 6, { 32, 0, 0 } },
        { 6, 7, { 0, -32, 0 } },
        { 7, 4, { -32, 0, 0 } },
        { 11, 10, { 32, 0, 0 } },
        { 8, 11, { 0, -32, 0 } },
        { 9, 8, { -32, 0, 0 } },
        { 10, 9, { 0, 32, 0 } },
        { 8, 9, { 0, 32, 0 } },
        { 9, 10, { 32, 0, 0 } },
        { 10, 11, { 0, -32, 0 } },
        { 11, 8, { -32, 0, 0 } },
        { 7, 6, { 32, 0, 0 } },
        { 4, 7, { 0, -32, 0 } },
        { 5, 4, { -32, 0, 0 } },
        { 6, 5, { 0, 32, 0 } },
        { 4, 8, { -32, 0, 0 } },
        { 5, 9, { 0, 32, 0 } },
        { 6, 10, { 32, 0, 0 } },
        { 7, 11, { 0, -32, 0 } },
        { 6, 10, { 32, 0, 0 } },
        { 7, 11, { 0, -32, 0 } },
        { 4, 8, { -32, 0, 0 } },
        { 5, 9, { 0, 32, 0 } },
        { 8, 4, { -32, 0, 0 } },
        { 9, 5, { 0, 32, 0 } },
        { 10, 6, { 32, 0, 0 } },
        { 11, 7, { 0, -32, 0 } },
        { 10, 6, { 32, 0, 0 } },
        { 11, 7, { 0, -32, 0 } },
        { 8, 4, { -32, 0, 0 } },
        { 9, 5, { 0, 32, 0 } },
        { 4, 4, { -32, 0, 16 } },
        { 5, 5, { 0, 32, 16 } },
        { 6, 6, { 32, 0, 16 } },
        { 7, 7, { 0, -32, 16 } },
        { 10, 10, { 32, 0, -16 } },
        { 11, 11, { 0, -32, -16 } },
        { 8, 8, { -32, 0, -16 } },
        { 9, 9, { 0, 32, -16 } },
        { 8, 8, { -32, 0, 16 } },
        { 9, 9, { 0, 32, 16 } },
        { 10, 10, { 32, 0, 16 } },
        { 11, 11, { 0, -32, 16 } },
        { 6, 6, { 32, 0, -16 } },
        { 7, 7, { 0, -32, -16 } },
        { 4, 4, { -32, 0, -16 } },
        { 5, 5, { 0, 32, -16 } },
        { 4, 4, { -32, 0, -16 } },
        { 5, 5, { 0, 32, -16 } },
        { 6, 6, { 32, 0, -16 } },
        { 7, 7, { 0, -32, -16 } },
        { 10, 10, { 32, 0, 16 } },
        { 11, 11, { 0, -32, 16 } },
        { 8, 8, { -32, 0, 16 } },
        { 9, 9, { 0, 32, 16 } },
        { 8, 8, { -32, 0, -16 } },
        { 9, 9, { 0, 32, -16 } },
        { 10, 10, { 32, 0, -16 } },
        { 11, 11, { 0, -32, -16 } },
        { 6, 6, { 32, 0, 16 } },
        { 7, 7, { 0, -32, 16 } },
        { 4, 4, { -32, 0, 16 } },
        { 5, 5, { 0, 32, 16 } },
        { 0, 4, { -32, 0, 0 } },
        { 1, 5, { 0, 32, 0 } },
        { 2, 6, { 32, 0, 0 } },
        { 3, 7, { 0, -32, 0 } },
        { 10, 2, { 32, 0, 0 } },
        { 11, 3, { 0, -32, 0 } },
        { 8, 0, { -32, 0, 0 } },
        { 9, 1, { 0, 32, 0 } },
        { 0, 8, { -32, 0, 0 } },
        { 1, 9, { 0, 32, 0 } },
        { 2, 10, { 32, 0, 0 } },
        { 3, 11, { 0, -32, 0 } },
        { 6, 2, { 32, 0, 0 } },
        { 7, 3, { 0, -32, 0 } },
        { 4, 0, { -32, 0, 0 } },
        { 5, 1, { 0, 32, 0 } },
        { 4, 0, { -32, 0, 0 } },
        { 5, 1, { 0, 32, 0 } },
        { 6, 2, { 32, 0, 0 } },
        { 7, 3, { 0, -32, 0 } },
        { 2, 10, { 32, 0, 0 } },
        { 3, 11, { 0, -32, 0 } },
        { 0, 8, { -32, 0, 0 } },
        { 1, 9, { 0, 32, 0 } },
        { 8, 0, { -32, 0, 0 } },
        { 9, 1, { 0, 32, 0 } },
        { 10, 2, { 32, 0, 0 } },
        { 11, 3, { 0, -32, 0 } },
        { 2, 6, { 32, 0, 0 } },
        { 3, 7, { 0, -32, 0 } },
        { 0, 4, { -32, 0, 0 } },
        { 1, 5, { 0, 32, 0 } },
        { 4, 6, { 32, 0, 0 } },
        { 5, 7, { 0, -32, 0 } },
        { 6, 4, { -32, 0, 0 } },
        { 7, 5, { 0, 32, 0 } },
        { 8, 10, { 32, 0, 0 } },
        { 9, 11, { 0, -32, 0 } },
        { 10, 8, { -32, 0, 0 } },
        { 11, 9, { 0, 32, 0 } },
        { 8, 10, { 32, 0, 0 } },
        { 9, 11, { 0, -32, 0 } },
        { 10, 8, { -32, 0, 0 } },
        { 11, 9, { 0, 32, 0 } },
        { 4, 6, { 32, 0, 0 } },
        { 5, 7, { 0, -32, 0 } },
        { 6, 4, { -32, 0, 0 } },
        { 7, 5, { 0, 32, 0 } },
    } };

    const TrackCoordinates& getUnkTrack(uint16_t trackAndDirection)
    {
        return kTrackCoordinates[trackAndDirection];
    }

    const TrackCoordinates& getUnkRoad(uint16_t trackAndDirection)
    {
        return kRoadCoordinates[trackAndDirection];
    }

    constexpr std::array<TrackMiscData, 44> _miscData = {
        // straight
        TrackMiscData{
            .costFactor = 0x100,
            .flags = CommonTraitFlags::none,
            .reverseTrackId = 0,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::none,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 32,
            .sparkDirection = true },
        // diagonal
        TrackMiscData{
            .costFactor = 0x16A,
            .flags = CommonTraitFlags::diagonal,
            .reverseTrackId = 1,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::diagonal,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 45,
            .sparkDirection = false },
        // leftCurveVerySmall
        TrackMiscData{
            .costFactor = 0x0C9,
            .flags = CommonTraitFlags::verySmallCurve,
            .reverseTrackId = 3,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::verySmallCurve,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 25,
            .sparkDirection = true },
        // rightCurveVerySmall
        TrackMiscData{
            .costFactor = 0x0C9,
            .flags = CommonTraitFlags::verySmallCurve,
            .reverseTrackId = 2,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::verySmallCurve,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 25,
            .sparkDirection = false },
        // leftCurveSmall
        TrackMiscData{
            .costFactor = 0x25B,
            .flags = CommonTraitFlags::smallCurve,
            .reverseTrackId = 5,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::smallCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 75,
            .sparkDirection = true },
        // rightCurveSmall
        TrackMiscData{
            .costFactor = 0x25B,
            .flags = CommonTraitFlags::smallCurve,
            .reverseTrackId = 4,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::smallCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 75,
            .sparkDirection = false },
        // leftCurve
        TrackMiscData{
            .costFactor = 0x3ED,
            .flags = CommonTraitFlags::curve,
            .reverseTrackId = 7,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::normalCurve,
            .curveSpeedFraction = 0x2666,
            .unkWeighting = 126,
            .sparkDirection = true },
        // rightCurve
        TrackMiscData{
            .costFactor = 0x3ED,
            .flags = CommonTraitFlags::curve,
            .reverseTrackId = 6,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::normalCurve,
            .curveSpeedFraction = 0x2666,
            .unkWeighting = 126,
            .sparkDirection = false },
        // leftCurveLarge
        TrackMiscData{
            .costFactor = 0x2BF,
            .flags = CommonTraitFlags::largeCurve,
            .reverseTrackId = 11,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::largeCurve,
            .curveSpeedFraction = 0x4000,
            .unkWeighting = 88,
            .sparkDirection = true },
        // rightCurveLarge
        TrackMiscData{
            .costFactor = 0x2BF,
            .flags = CommonTraitFlags::largeCurve,
            .reverseTrackId = 10,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::largeCurve,
            .curveSpeedFraction = 0x4000,
            .unkWeighting = 88,
            .sparkDirection = false },
        // diagonalLeftCurveLarge
        TrackMiscData{
            .costFactor = 0x2BF,
            .flags = CommonTraitFlags::largeCurve,
            .reverseTrackId = 9,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::largeCurve,
            .curveSpeedFraction = 0x4000,
            .unkWeighting = 88,
            .sparkDirection = true },
        // diagonalRightCurveLarge
        TrackMiscData{
            .costFactor = 0x2BF,
            .flags = CommonTraitFlags::largeCurve,
            .reverseTrackId = 8,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::largeCurve,
            .curveSpeedFraction = 0x4000,
            .unkWeighting = 88,
            .sparkDirection = false },
        // sBendLeft
        TrackMiscData{
            .costFactor = 0x380,
            .flags = CommonTraitFlags::sBendCurve,
            .reverseTrackId = 12,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::sBend,
            .curveSpeedFraction = 0x2666,
            .unkWeighting = 106,
            .sparkDirection = false },
        // sBendRight
        TrackMiscData{
            .costFactor = 0x380,
            .flags = CommonTraitFlags::sBendCurve,
            .reverseTrackId = 13,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::sBend,
            .curveSpeedFraction = 0x2666,
            .unkWeighting = 106,
            .sparkDirection = false },
        // straightSlopeUp
        TrackMiscData{
            .costFactor = 0x220,
            .flags = CommonTraitFlags::slope,
            .reverseTrackId = 15,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 1,
            .signalHeightOffsetRight = 15,
            .compatibleFlags = TrackTraitFlags::slope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 66,
            .sparkDirection = false },
        // straightSlopeDown
        TrackMiscData{
            .costFactor = 0x220,
            .flags = CommonTraitFlags::slope,
            .reverseTrackId = 14,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 15,
            .signalHeightOffsetRight = 1,
            .compatibleFlags = TrackTraitFlags::slope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 66,
            .sparkDirection = true },
        // straightSteepSlopeUp
        TrackMiscData{
            .costFactor = 0x138,
            .flags = CommonTraitFlags::steepSlope,
            .reverseTrackId = 17,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 2,
            .signalHeightOffsetRight = 14,
            .compatibleFlags = TrackTraitFlags::steepSlope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = false },
        // straightSteepSlopeDown
        TrackMiscData{
            .costFactor = 0x138,
            .flags = CommonTraitFlags::steepSlope,
            .reverseTrackId = 16,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 14,
            .signalHeightOffsetRight = 2,
            .compatibleFlags = TrackTraitFlags::steepSlope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = true },
        // leftCurveSmallSlopeUp
        TrackMiscData{
            .costFactor = 0x350,
            .flags = CommonTraitFlags::slope | CommonTraitFlags::curveSlope | CommonTraitFlags::smallCurve,
            .reverseTrackId = 21,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 1,
            .signalHeightOffsetRight = 15,
            .compatibleFlags = TrackTraitFlags::smallCurve | TrackTraitFlags::slope | TrackTraitFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 77,
            .sparkDirection = true },
        // rightCurveSmallSlopeUp
        TrackMiscData{
            .costFactor = 0x350,
            .flags = CommonTraitFlags::slope | CommonTraitFlags::curveSlope | CommonTraitFlags::smallCurve,
            .reverseTrackId = 20,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 1,
            .signalHeightOffsetRight = 15,
            .compatibleFlags = TrackTraitFlags::smallCurve | TrackTraitFlags::slope | TrackTraitFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 77,
            .sparkDirection = false },
        // leftCurveSmallSlopeDown
        TrackMiscData{
            .costFactor = 0x350,
            .flags = CommonTraitFlags::slope | CommonTraitFlags::curveSlope | CommonTraitFlags::smallCurve,
            .reverseTrackId = 19,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 15,
            .signalHeightOffsetRight = 1,
            .compatibleFlags = TrackTraitFlags::smallCurve | TrackTraitFlags::slope | TrackTraitFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 77,
            .sparkDirection = true },
        // rightCurveSmallSlopeDown
        TrackMiscData{
            .costFactor = 0x350,
            .flags = CommonTraitFlags::slope | CommonTraitFlags::curveSlope | CommonTraitFlags::smallCurve,
            .reverseTrackId = 18,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 15,
            .signalHeightOffsetRight = 1,
            .compatibleFlags = TrackTraitFlags::smallCurve | TrackTraitFlags::slope | TrackTraitFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 77,
            .sparkDirection = false },
        // leftCurveSmallSteepSlopeUp
        TrackMiscData{
            .costFactor = 0x41F,
            .flags = CommonTraitFlags::steepSlope | CommonTraitFlags::curveSlope | CommonTraitFlags::smallCurve,
            .reverseTrackId = 25,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 2,
            .signalHeightOffsetRight = 14,
            .compatibleFlags = TrackTraitFlags::smallCurve | TrackTraitFlags::steepSlope | TrackTraitFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 82,
            .sparkDirection = true },
        // rightCurveSmallSteepSlopeUp
        TrackMiscData{
            .costFactor = 0x41F,
            .flags = CommonTraitFlags::steepSlope | CommonTraitFlags::curveSlope | CommonTraitFlags::smallCurve,
            .reverseTrackId = 24,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 2,
            .signalHeightOffsetRight = 14,
            .compatibleFlags = TrackTraitFlags::smallCurve | TrackTraitFlags::steepSlope | TrackTraitFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 82,
            .sparkDirection = false },
        // leftCurveSmallSteepSlopeDown
        TrackMiscData{
            .costFactor = 0x41F,
            .flags = CommonTraitFlags::steepSlope | CommonTraitFlags::curveSlope | CommonTraitFlags::smallCurve,
            .reverseTrackId = 23,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 14,
            .signalHeightOffsetRight = 2,
            .compatibleFlags = TrackTraitFlags::smallCurve | TrackTraitFlags::steepSlope | TrackTraitFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 82,
            .sparkDirection = true },
        // rightCurveSmallSteepSlopeDown
        TrackMiscData{
            .costFactor = 0x41F,
            .flags = CommonTraitFlags::steepSlope | CommonTraitFlags::curveSlope | CommonTraitFlags::smallCurve,
            .reverseTrackId = 22,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 14,
            .signalHeightOffsetRight = 2,
            .compatibleFlags = TrackTraitFlags::smallCurve | TrackTraitFlags::steepSlope | TrackTraitFlags::slopedCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 82,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x100,
            .flags = CommonTraitFlags::oneSided,
            .reverseTrackId = 27,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 32,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x100,
            .flags = CommonTraitFlags::oneSided,
            .reverseTrackId = 26,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 32,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x064,
            .flags = CommonTraitFlags::verySmallCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 31,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::verySmallCurve | TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 13,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x12D,
            .flags = CommonTraitFlags::verySmallCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 30,
            .reverseRotation = 1,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::verySmallCurve | TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 38,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x12D,
            .flags = CommonTraitFlags::verySmallCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 29,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::verySmallCurve | TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 38,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x064,
            .flags = CommonTraitFlags::verySmallCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 28,
            .reverseRotation = 3,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::verySmallCurve | TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 13,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x126,
            .flags = CommonTraitFlags::sBendCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 32,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 36,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x126,
            .flags = CommonTraitFlags::sBendCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 33,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 36,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x138,
            .flags = CommonTraitFlags::steepSlope | CommonTraitFlags::oneSided,
            .reverseTrackId = 37,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 2,
            .signalHeightOffsetRight = 14,
            .compatibleFlags = TrackTraitFlags::steepSlope | TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x138,
            .flags = CommonTraitFlags::steepSlope | CommonTraitFlags::oneSided,
            .reverseTrackId = 36,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 2,
            .signalHeightOffsetRight = 14,
            .compatibleFlags = TrackTraitFlags::steepSlope | TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x138,
            .flags = CommonTraitFlags::steepSlope | CommonTraitFlags::oneSided,
            .reverseTrackId = 35,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 14,
            .signalHeightOffsetRight = 2,
            .compatibleFlags = TrackTraitFlags::steepSlope | TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x138,
            .flags = CommonTraitFlags::steepSlope | CommonTraitFlags::oneSided,
            .reverseTrackId = 34,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 14,
            .signalHeightOffsetRight = 2,
            .compatibleFlags = TrackTraitFlags::steepSlope | TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x114,
            .flags = CommonTraitFlags::sBendCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 41,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 34,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x114,
            .flags = CommonTraitFlags::sBendCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 40,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 34,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x114,
            .flags = CommonTraitFlags::sBendCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 39,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 34,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x114,
            .flags = CommonTraitFlags::sBendCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 38,
            .reverseRotation = 2,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 34,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x25B,
            .flags = CommonTraitFlags::verySmallCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 43,
            .reverseRotation = 0,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 26,
            .sparkDirection = false },
        // unused
        TrackMiscData{
            .costFactor = 0x25B,
            .flags = CommonTraitFlags::verySmallCurve | CommonTraitFlags::oneSided,
            .reverseTrackId = 42,
            .reverseRotation = 0,
            .signalHeightOffsetLeft = 0,
            .signalHeightOffsetRight = 0,
            .compatibleFlags = TrackTraitFlags::oneSided,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 26,
            .sparkDirection = false },
    };

    const TrackMiscData& getTrackMiscData(size_t trackId)
    {
        return _miscData[trackId];
    }

    constexpr std::array<RoadMiscData, 10> _roadMiscData = {
        // straight
        RoadMiscData{
            .costFactor = 0x100,
            .flags = CommonTraitFlags::none,
            .reverseRoadId = 0,
            .reverseRotation = 2,
            .reverseLane = 1,
            .compatibleFlags = RoadTraitFlags::none,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 32,
        },
        // leftCurveVerySmall
        RoadMiscData{
            .costFactor = 0x0C9,
            .flags = CommonTraitFlags::verySmallCurve,
            .reverseRoadId = 2,
            .reverseRotation = 1,
            .reverseLane = 1,
            .compatibleFlags = RoadTraitFlags::verySmallCurve,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 25,
        },
        // rightCurveVerySmall
        RoadMiscData{
            .costFactor = 0x0C9,
            .flags = CommonTraitFlags::verySmallCurve,
            .reverseRoadId = 1,
            .reverseRotation = 3,
            .reverseLane = 1,
            .compatibleFlags = RoadTraitFlags::verySmallCurve,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 25,
        },
        // leftCurveSmall
        RoadMiscData{
            .costFactor = 0x25B,
            .flags = CommonTraitFlags::smallCurve,
            .reverseRoadId = 4,
            .reverseRotation = 1,
            .reverseLane = 4,
            .compatibleFlags = RoadTraitFlags::smallCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 75,
        },
        // rightCurveSmall
        RoadMiscData{
            .costFactor = 0x25B,
            .flags = CommonTraitFlags::smallCurve,
            .reverseRoadId = 3,
            .reverseRotation = 3,
            .reverseLane = 4,
            .compatibleFlags = RoadTraitFlags::smallCurve,
            .curveSpeedFraction = 0x199A,
            .unkWeighting = 75,
        },
        // straightSlopeUp
        RoadMiscData{
            .costFactor = 0x220,
            .flags = CommonTraitFlags::slope,
            .reverseRoadId = 6,
            .reverseRotation = 2,
            .reverseLane = 2,
            .compatibleFlags = RoadTraitFlags::slope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 66,
        },
        // straightSlopeDown
        RoadMiscData{
            .costFactor = 0x220,
            .flags = CommonTraitFlags::slope,
            .reverseRoadId = 5,
            .reverseRotation = 2,
            .reverseLane = 2,
            .compatibleFlags = RoadTraitFlags::slope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 66,
        },
        // straightSteepSlopeUp
        RoadMiscData{
            .costFactor = 0x138,
            .flags = CommonTraitFlags::steepSlope,
            .reverseRoadId = 8,
            .reverseRotation = 2,
            .reverseLane = 1,
            .compatibleFlags = RoadTraitFlags::steepSlope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
        },
        // straightSteepSlopeDown
        RoadMiscData{
            .costFactor = 0x138,
            .flags = CommonTraitFlags::steepSlope,
            .reverseRoadId = 7,
            .reverseRotation = 2,
            .reverseLane = 1,
            .compatibleFlags = RoadTraitFlags::steepSlope,
            .curveSpeedFraction = 0xFFFF,
            .unkWeighting = 36,
        },
        // turnaround
        RoadMiscData{
            .costFactor = 0x25B,
            .flags = CommonTraitFlags::none,
            .reverseRoadId = 9,
            .reverseRotation = 0,
            .reverseLane = 1,
            .compatibleFlags = RoadTraitFlags::turnaround,
            .curveSpeedFraction = 0x0CCD,
            .unkWeighting = 26,
        },
    };

    const RoadMiscData& getRoadMiscData(size_t roadId)
    {
        return _roadMiscData[roadId];
    }

    // TODO: The following i think can be further deduplicated/generated

    static constexpr std::array<RoadUnkNextTo, 2> kStraightUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, -32, 0 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { 0, 32, 0 },
            .rotation = 3,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 2> kLeftCurveVerySmallUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, 32, 0 },
            .rotation = 3,
        },
        RoadUnkNextTo{
            .pos = { -32, 0, 0 },
            .rotation = 2,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 2> kRightCurveVerySmallUnkNextTo = {
        RoadUnkNextTo{
            .pos = { -32, 0, 0 },
            .rotation = 2,
        },
        RoadUnkNextTo{
            .pos = { 0, -32, 0 },
            .rotation = 1,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 4> kLeftCurveSmallLUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, 32, 0 },
            .rotation = 3,
        },
        RoadUnkNextTo{
            .pos = { -32, 32, 0 },
            .rotation = 3,
        },
        RoadUnkNextTo{
            .pos = { -64, 0, 0 },
            .rotation = 2,
        },
        RoadUnkNextTo{
            .pos = { -64, -32, 0 },
            .rotation = 2,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 4> kLeftCurveSmallRUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 32, 64, 0 },
            .rotation = 3,
        },
        RoadUnkNextTo{
            .pos = { 0, 64, 0 },
            .rotation = 3,
        },
        RoadUnkNextTo{
            .pos = { -32, 0, 0 },
            .rotation = 2,
        },
        RoadUnkNextTo{
            .pos = { -32, 32, 0 },
            .rotation = 2,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 4> kRightCurveSmallLUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, -32, 0 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { -32, -32, 0 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { -64, 0, 0 },
            .rotation = 2,
        },
        RoadUnkNextTo{
            .pos = { -64, 32, 0 },
            .rotation = 2,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 4> kRightCurveSmallRUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, -64, 0 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { 32, -64, 0 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { -32, 0, 0 },
            .rotation = 2,
        },
        RoadUnkNextTo{
            .pos = { -32, -32, 0 },
            .rotation = 2,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 4> kStraightSlopeUpLUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, -32, 0 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { 0, 32, 0 },
            .rotation = 3,
        },
        RoadUnkNextTo{
            .pos = { -32, -32, 16 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { -32, 32, 16 },
            .rotation = 3,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 4> kStraightSlopeUpRUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, -32, 16 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { 0, 32, 16 },
            .rotation = 3,
        },
        RoadUnkNextTo{
            .pos = { -32, -32, 0 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { -32, 32, 0 },
            .rotation = 3,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 4> kStraightSlopeDownLUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, -32, 16 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { 0, 32, 16 },
            .rotation = 3,
        },
        RoadUnkNextTo{
            .pos = { -32, -32, 0 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { -32, 32, 0 },
            .rotation = 3,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 4> kStraightSlopeDownRUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, -32, 0 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { 0, 32, 0 },
            .rotation = 3,
        },
        RoadUnkNextTo{
            .pos = { -32, -32, 16 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { -32, 32, 16 },
            .rotation = 3,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 2> kStraightSteepSlopeUpLUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, -32, 16 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { 0, 32, 16 },
            .rotation = 3,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 2> kStraightSteepSlopeUpRUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, -32, 0 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { 0, 32, 0 },
            .rotation = 3,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 2> kStraightSteepSlopeDownLUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, -32, 0 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { 0, 32, 0 },
            .rotation = 3,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 2> kStraightSteepSlopeDownRUnkNextTo = {
        RoadUnkNextTo{
            .pos = { 0, -32, 16 },
            .rotation = 1,
        },
        RoadUnkNextTo{
            .pos = { 0, 32, 16 },
            .rotation = 3,
        },
    };

    static constexpr std::array<RoadUnkNextTo, 2> kTurnaroundUnkNextTo = kStraightUnkNextTo;

    // 0x004F6E02
    static constexpr std::array<std::span<const RoadUnkNextTo>, 20> kRoadUnkNextTo = {
        kStraightUnkNextTo,
        kStraightUnkNextTo,
        kLeftCurveVerySmallUnkNextTo,
        kLeftCurveVerySmallUnkNextTo,
        kRightCurveVerySmallUnkNextTo,
        kRightCurveVerySmallUnkNextTo,
        kLeftCurveSmallLUnkNextTo,
        kLeftCurveSmallRUnkNextTo,
        kRightCurveSmallLUnkNextTo,
        kRightCurveSmallRUnkNextTo,
        kStraightSlopeUpLUnkNextTo,
        kStraightSlopeUpRUnkNextTo,
        kStraightSlopeDownLUnkNextTo,
        kStraightSlopeDownRUnkNextTo,
        kStraightSteepSlopeUpLUnkNextTo,
        kStraightSteepSlopeUpRUnkNextTo,
        kStraightSteepSlopeDownLUnkNextTo,
        kStraightSteepSlopeDownRUnkNextTo,
        kTurnaroundUnkNextTo,
        kTurnaroundUnkNextTo,
    };

    std::span<const RoadUnkNextTo> getRoadUnkNextTo(uint16_t trackAndDirection)
    {
        assert(static_cast<size_t>(trackAndDirection / 4) < kRoadUnkNextTo.size());
        return kRoadUnkNextTo[trackAndDirection / 4];
    }

    // 0x004F7358
    // Lane occupation mask lookup table
    // Indexed by (trackAndDirection._data >> 2), which encodes:
    // - roadId (track piece identifier)
    // - isBackwards (direction of travel)
    // - isOvertakeLane (which lane the vehicle is in)
    // - isChangingLane (whether the vehicle is in the process of changing lanes)
    // Upper nibble (>> 4) contains 2-bit lane occupation mask
    static constexpr std::array<uint8_t, 128> kRoadOccupationMasks = { {
        // clang-format off
        0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20,
        0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20,
        0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10,
        0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10,
        0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10,
        0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10,
        0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
        // clang-format on
    } };

    uint8_t getRoadOccupationMask(uint8_t index)
    {
        assert(index < kRoadOccupationMasks.size());
        return kRoadOccupationMasks[index];
    }
}
