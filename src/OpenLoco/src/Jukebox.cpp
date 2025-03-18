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
    static MusicId currentTrack;  // 0x0050D434
    static MusicId previousTrack; // 0x0050D435

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

    StringId getCurrentTrackTitleId()
    {
        if (currentTrack == kNoSong)
        {
            return StringIds::music_none;
        }
        return kMusicInfo[currentTrack].titleId;
    }

    // If the player has manually requested a track, returns the MusicId of that track.
    // Otherwise, returns kNoSong.
    MusicId getRequestedTrack()
    {
        if (previousTrack == currentTrack)
        {
            return currentTrack;
        }
        return kNoSong;
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

    static MusicId chooseNextMusicTrack(MusicId lastSong)
    {
        auto playlist = makeSelectedPlaylist();

        const auto& cfg = Config::get().old;

        if (playlist.empty() && cfg.musicPlaylist != Config::MusicPlaylistType::currentEra)
        {
            playlist = makeCurrentEraPlaylist();
        }

        if (playlist.empty())
        {
            playlist = makeAllMusicPlaylist();
        }

        // Remove lastSong if it is present and not the only song, so that you do not get the same song twice.
        // Assumes there is no more than one occurence of that song in the playlist.
        if (playlist.size() > 1)
        {
            auto position = std::find(playlist.begin(), playlist.end(), lastSong);
            if (position != playlist.end())
            {
                playlist.erase(position);
            }
        }

        auto r = std::rand() % playlist.size();
        auto track = playlist[r];
        return track;
    }

    // Changes currentTrack to the next music track if applicable, updates previousTrack,
    // and returns its MusicInfo.
    const MusicInfo& changeTrack()
    {
        // If currentTrack == previousTrack, that is our signal that a track has been manually requested.
        bool trackRequested = previousTrack != kNoSong && previousTrack == currentTrack;

        // Choose a track to play, unless we have requested one track in particular.
        if (currentTrack == kNoSong || !trackRequested)
        {
            previousTrack = currentTrack;
            currentTrack = chooseNextMusicTrack(previousTrack);
        }
        else
        {
            // Make sure it doesn't think it is manually requested again for next time.
            previousTrack = kNoSong;
        }

        // Return the information for this track so that Audio.cpp can get its PathId to play.
        return kMusicInfo[currentTrack];
    }

    // The player manually selects a song that they want to be played immediately.
    // Note: assumes music is already being played.
    bool changeTrackTo(MusicId toTrack)
    {
        if (toTrack == currentTrack)
        {
            return false;
        }

        Audio::stopMusic();

        currentTrack = toTrack;
        previousTrack = toTrack; // Setting these to the same track is how Audio::playBackgroundMusic() detects that it has been requested manually

        // Previously 0x0050D430 '_songProgress' would be set to 0 here, but this loco global was not used for anything anymore.

        return true;
    }

    // Prematurely stop the current song so another can be played.
    bool skipCurrentTrack()
    {
        if (Config::get().old.musicPlaying == 0)
        {
            return false;
        }

        // By stopping the music, the next time OpenLoco::tick() calls Audio::playBackgroundMusic(),
        // it will detect that the channel is not playing anything and it will play the next music track.
        Audio::stopMusic();

        return true;
    }

    // Disables music altogether.
    bool stopMusic()
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

    void resetJukebox()
    {
        currentTrack = kNoSong;
        previousTrack = kNoSong;
    }
}
