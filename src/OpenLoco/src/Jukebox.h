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

    struct MusicInfo
    {
        Environment::PathId pathId;
        StringId titleId;
        uint16_t startYear;
        uint16_t endYear;
    };

    std::vector<MusicId> makeSelectedPlaylist();
    MusicId chooseNextMusicTrack(MusicId lastSong);
    const MusicInfo& getMusicInfo(MusicId track);
}
