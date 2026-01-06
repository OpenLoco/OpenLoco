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
    static constexpr uint16_t kNoStartYear = 0;
    static constexpr uint16_t kNoEndYear = 9999;

    struct MusicInfo
    {
        Environment::PathId pathId;
        StringId titleId;
        uint16_t startYear;
        uint16_t endYear;
    };

    enum class MusicSortMode : uint8_t
    {
        original, // In vanilla, music tracks were displayed in internal ID order.
        titleAscending,
        titleDescending,
        yearsAscending,
        yearsDescending,
    };

    const MusicInfo& getMusicInfo(MusicId track);
    bool isMusicPlaying();
    MusicId getCurrentTrack();
    StringId getSelectedTrackTitleId();
    std::vector<MusicId> makeAllMusicPlaylist(MusicSortMode ordering = MusicSortMode::original);
    std::vector<MusicId> makeSelectedPlaylist();

    const MusicInfo& consumeTrack();
    bool requestTrack(MusicId track);
    bool skipCurrentTrack();
    bool disableMusic();
    bool enableMusic();
    void resetJukebox();
}
