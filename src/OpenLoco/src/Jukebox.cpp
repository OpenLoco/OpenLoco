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
    static MusicId selectedTrack; // 0x0050D434
    static bool selectedTrackNotPlayedYet;

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
    static_assert(std::size(kMusicInfo) == kNumMusicTracks);

    const MusicInfo& getMusicInfo(MusicId track)
    {
        return kMusicInfo[track];
    }

    // Note: this counts paused as playing
    bool isMusicPlaying()
    {
        return (selectedTrack != kNoSong && Config::get().audio.playJukeboxMusic);
    }

    MusicId getCurrentTrack()
    {
        return selectedTrack;
    }

    StringId getSelectedTrackTitleId()
    {
        if (selectedTrack == kNoSong)
        {
            return StringIds::music_none;
        }
        return kMusicInfo[selectedTrack].titleId;
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

        const auto& cfg = Config::get().audio;
        for (auto i = 0; i < kNumMusicTracks; i++)
        {
            if (cfg.customJukebox[i] & 1)
            {
                playlist.push_back(i);
            }
        }

        return playlist;
    }

    std::vector<MusicId> makeSelectedPlaylist()
    {
        using Config::MusicPlaylistType;

        switch (Config::get().audio.playlist)
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

    // Change selectedTrack to a random track from the selected playlist
    static void chooseNextTrack()
    {
        auto playlist = makeSelectedPlaylist();

        // Fallback playlist (TODO: does this even make sense?)
        const auto& cfg = Config::get();
        if (playlist.empty() && cfg.audio.playlist != Config::MusicPlaylistType::currentEra)
        {
            playlist = makeCurrentEraPlaylist();

            if (playlist.empty())
            {
                playlist = makeAllMusicPlaylist();
            }
        }

        // Unless it is the only track, prevent the same song from playing twice in a row.
        // Assumes there is no more than one occurence of this track in the playlist.
        if (playlist.size() > 1)
        {
            auto position = std::find(playlist.begin(), playlist.end(), selectedTrack);
            if (position != playlist.end())
            {
                playlist.erase(position);
            }
        }

        // And pick a song!
        auto r = std::rand() % playlist.size();
        selectedTrack = playlist[r];

        selectedTrackNotPlayedYet = true;
    }

    // If the next track has already been selected, mark it as played. Otherwise, choose the next track (and mark it as played).
    // Returns the information for this track so that Audio.cpp can get its PathId to play.
    const MusicInfo& consumeTrack()
    {
        // Track requested? If so, selectedTrack has already been set for us.
        if (!selectedTrackNotPlayedYet)
        {
            chooseNextTrack();
        }
        selectedTrackNotPlayedYet = false;

        return kMusicInfo[selectedTrack];
    }

    // The player manually selects a song from the drop-down in the music options. consumeTrack() is expected to be called shortly after this.
    bool requestTrack(MusicId track)
    {
        assert(track < kNumMusicTracks); // Also catches kNoSong ("[None]"), which is impossible to request.

        if (track == selectedTrack)
        {
            return false;
        }

        selectedTrack = track;
        selectedTrackNotPlayedYet = true;

        // If a song is playing, stop it prematurely.
        Audio::stopMusic();

        // Previously 0x0050D430 '_songProgress' would be set to 0 here, but that loco global was no longer used for anything.

        return true;
    }

    // Prematurely stops the current song so that another will be played.
    bool skipCurrentTrack()
    {
        if (Config::get().audio.playJukeboxMusic == 0)
        {
            return false;
        }

        // Changing the track here makes the skip button responsive even when paused.
        chooseNextTrack();

        // Audio::playBackgroundMusic() will call Jukebox::consumeTrack() if the music is stopped.
        Audio::stopMusic();

        return true;
    }

    // When the player disables "Play Music" from the top toolbar, or clicks the stop button in the music options.
    bool disableMusic()
    {
        auto& cfg = Config::get().audio;

        if (cfg.playJukeboxMusic == 0)
        {
            return false;
        }

        cfg.playJukeboxMusic = 0;
        Config::write();

        Audio::stopMusic();
        selectedTrack = kNoSong;

        return true;
    }

    bool enableMusic()
    {
        auto& cfg = Config::get().audio;

        if (cfg.playJukeboxMusic != 0)
        {
            return false;
        }

        cfg.playJukeboxMusic = 1;
        Config::write();

        return true;
    }

    void resetJukebox()
    {
        selectedTrack = kNoSong;
        selectedTrackNotPlayedYet = false;
    }
}
