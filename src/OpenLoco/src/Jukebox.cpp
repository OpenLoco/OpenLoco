#include "Jukebox.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Date.h"
#include "Environment.h"
#include "Localisation/StringIds.h"
#include <numeric>

using namespace OpenLoco::Environment;
using namespace OpenLoco::Interop;

namespace OpenLoco::Jukebox
{
    static MusicId currentTrack;       // 0x0050D434
    static MusicId requestedNextTrack; // Song manually chosen to play next. kNoSong means nothing has been requested.

    // 0x004FE910
    static constexpr MusicInfo kMusicInfo[] = {
        { PathId::music_chuggin_along, StringIds::music_chuggin_along, 1925, 1933 },
        { PathId::music_long_dusty_road, StringIds::music_long_dusty_road, 1927, 1935 },
        { PathId::music_flying_high, StringIds::music_flying_high, 1932, 1940 },
        { PathId::music_gettin_on_the_gas, StringIds::music_gettin_on_the_gas, 1956, 1964 },
        { PathId::music_jumpin_the_rails, StringIds::music_jumpin_the_rails, 1953, 1961 },
        { PathId::music_smooth_running, StringIds::music_smooth_running, 1976, 1984 },
        { PathId::music_traffic_jam, StringIds::music_traffic_jam, 1973, 1981 },
        { PathId::music_never_stop_til_you_get_there, StringIds::music_never_stop_til_you_get_there, 1970, 1978 },
        { PathId::music_soaring_away, StringIds::music_soaring_away, 1990, 9999 },
        { PathId::music_techno_torture, StringIds::music_techno_torture, 1993, 9999 },
        { PathId::music_everlasting_high_rise, StringIds::music_everlasting_high_rise, 1996, 9999 },
        { PathId::music_solace, StringIds::music_solace, 1912, 1920 },
        { PathId::music_chrysanthemum, StringIds::music_chrysanthemum, 0, 1911 },
        { PathId::music_eugenia, StringIds::music_eugenia, 0, 1908 },
        { PathId::music_the_ragtime_dance, StringIds::music_the_ragtime_dance, 1909, 1917 },
        { PathId::music_easy_winners, StringIds::music_easy_winners, 0, 1914 },
        { PathId::music_setting_off, StringIds::music_setting_off, 1929, 1937 },
        { PathId::music_a_travellers_serenade, StringIds::music_a_travellers_serenade, 1940, 1948 },
        { PathId::music_latino_trip, StringIds::music_latino_trip, 1943, 1951 },
        { PathId::music_a_good_head_of_steam, StringIds::music_a_good_head_of_steam, 1950, 1958 },
        { PathId::music_hop_to_the_bop, StringIds::music_hop_to_the_bop, 1946, 1954 },
        { PathId::music_the_city_lights, StringIds::music_the_city_lights, 1980, 1988 },
        { PathId::music_steamin_down_town, StringIds::music_steamin_down_town, 1960, 1968 },
        { PathId::music_bright_expectations, StringIds::music_bright_expectations, 1983, 1991 },
        { PathId::music_mo_station, StringIds::music_mo_station, 1963, 1971 },
        { PathId::music_far_out, StringIds::music_far_out, 1966, 1974 },
        { PathId::music_running_on_time, StringIds::music_running_on_time, 1986, 1994 },
        { PathId::music_get_me_to_gladstone_bay, StringIds::music_get_me_to_gladstone_bay, 1918, 1926 },
        { PathId::music_sandy_track_blues, StringIds::music_sandy_track_blues, 1921, 1929 }
    };
    static_assert(sizeof(kMusicInfo) / sizeof(kMusicInfo[0]) == kNumMusicTracks);

    const MusicInfo& getMusicInfo(MusicId track)
    {
        return kMusicInfo[track];
    }

    bool isMusicPlaying()
    {
        return (currentTrack != kNoSong && Config::get().old.musicPlaying);
    }

    MusicId getCurrentTrack()
    {
        return currentTrack;
    }

    StringId getSelectedTrackTitleId()
    {
        if (requestedNextTrack != kNoSong)
        {
            return kMusicInfo[requestedNextTrack].titleId;
        }
        if (currentTrack != kNoSong)
        {
            return kMusicInfo[currentTrack].titleId;
        }
        return StringIds::music_none;
    }

    static std::vector<MusicId> makeAllMusicPlaylist()
    {
        std::vector<MusicId> playlist(kNumMusicTracks);
        std::iota(playlist.begin(), playlist.end(), 0);
        return playlist;
    }

    static std::vector<MusicId> makeCurrentEraPlaylist()
    {
        auto playlist = std::vector<MusicId>();
        auto currentYear = getCurrentYear();

        for (auto i = 0; i < kNumMusicTracks; i++)
        {
            const auto& mi = kMusicInfo[i];
            if (currentYear >= mi.startYear && currentYear <= mi.endYear)
            {
                playlist.push_back(i);
            }
        }

        return playlist;
    }

    static std::vector<MusicId> makeCustomSelectionPlaylist()
    {
        auto playlist = std::vector<MusicId>();

        const auto& cfg = Config::get().old;
        for (auto i = 0; i < kNumMusicTracks; i++)
        {
            if (cfg.enabledMusic[i] & 1)
            {
                playlist.push_back(i);
            }
        }

        return playlist;
    }

    std::vector<MusicId> makeSelectedPlaylist()
    {
        using Config::MusicPlaylistType;

        switch (Config::get().old.musicPlaylist)
        {
            case MusicPlaylistType::currentEra:
                return makeCurrentEraPlaylist();

            case MusicPlaylistType::all:
                return makeAllMusicPlaylist();

            case MusicPlaylistType::custom:
                return makeCustomSelectionPlaylist();
        }

        throw Exception::RuntimeError("Invalid MusicPlaylistType");
    }

    // Changes the value of `currentTrack`, and returns the information of that track (so that Audio.cpp can get its PathId to play).
    const MusicInfo& changeTrack()
    {
        // Was a specific track requested?
        if (requestedNextTrack != kNoSong)
        {
            currentTrack = requestedNextTrack;
            requestedNextTrack = kNoSong;

            return kMusicInfo[currentTrack];
        }

        // No requests? Get a playlist that we can choose a track from!
        auto playlist = makeSelectedPlaylist();

        const auto& cfg = Config::get().old;
        if (playlist.empty() && cfg.musicPlaylist != Config::MusicPlaylistType::currentEra)
        {
            playlist = makeCurrentEraPlaylist();

            if (playlist.empty())
            {
                playlist = makeAllMusicPlaylist();
            }
        }

        // Remove currentTrack to prevent the same song from playing twice in a row, unless it is the only track.
        // Assumes there is no more than one occurence of this track in the playlist.
        if (playlist.size() > 1)
        {
            auto position = std::find(playlist.begin(), playlist.end(), currentTrack);
            if (position != playlist.end())
            {
                playlist.erase(position);
            }
        }

        // And pick one
        auto r = std::rand() % playlist.size();
        currentTrack = playlist[r];
        return kMusicInfo[currentTrack];
    }

    // The player manually selects a song from the drop-down in the music options.
    bool requestTrack(MusicId track)
    {
        assert(track < kNumMusicTracks); // Will also catch kNoSong ("[None]"), which isn't in the drop-down and would be weird to request

        if (track == currentTrack)
        {
            return false;
        }

        requestedNextTrack = track;

        // Stop the currently playing song prematurely
        Audio::stopMusic();

        // Previously 0x0050D430 '_songProgress' would be set to 0 here, but that loco global was no longer used for anything.

        return true;
    }

    // Prematurely stops the current song so that another can be played.
    bool skipCurrentTrack()
    {
        if (Config::get().old.musicPlaying == 0)
        {
            return false;
        }

        // By stopping the music, the next time OpenLoco::tick() calls Audio::playBackgroundMusic(),
        // it will detect that the channel is not playing anything, and so it will play the next music track.
        Audio::stopMusic();

        return true;
    }

    // When the player disables "Play Music" from the top toolbar, or clicks the stop button in the music options.
    bool disableMusic()
    {
        auto& cfg = Config::get().old;

        if (cfg.musicPlaying == 0)
        {
            return false;
        }

        cfg.musicPlaying = 0;
        Config::write();

        Audio::stopMusic();
        currentTrack = kNoSong;

        return true;
    }

    bool enableMusic()
    {
        auto& cfg = Config::get().old;

        if (cfg.musicPlaying != 0)
        {
            return false;
        }

        cfg.musicPlaying = 1;
        Config::write();

        return true;
    }

    void resetJukebox()
    {
        currentTrack = kNoSong;
        requestedNextTrack = kNoSong;
    }
}
