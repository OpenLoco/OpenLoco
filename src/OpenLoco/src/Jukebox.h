#pragma once

#include "Types.hpp"
#include <vector>

namespace OpenLoco::Environment
{
    enum class PathId : uint8_t;
}

namespace OpenLoco::Jukebox
{
    using MusicId = uint8_t;

    constexpr int32_t kNumMusicTracks = 29;
    static constexpr MusicId kNoSong = 0xFF;

    struct MusicInfo
    {
        Environment::PathId pathId;
        StringId titleId;
        uint16_t startYear;
        uint16_t endYear;
    };

    enum class MusicSortMode : uint8_t
    {
        original, // vanilla ordered tracks by interal ID
        name,
        nameReverse,
        year,
        yearReverse,
    };

    const MusicInfo& getMusicInfo(MusicId track);
    bool isMusicPlaying();
    MusicId getCurrentTrack();
    StringId getSelectedTrackTitleId();
    std::vector<MusicId> makeAllMusicPlaylist(MusicSortMode ordering = MusicSortMode::original);
    std::vector<MusicId> makeSelectedPlaylist();

    const MusicInfo& changeTrack();
    bool requestTrack(MusicId track);
    bool skipCurrentTrack();
    bool disableMusic();
    bool enableMusic();
    void resetJukebox();
}
