#include "Jukebox.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Date.h"
#include "Environment.h"
#include "Localisation/StringIds.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <numeric>

using namespace OpenLoco::Environment;
using namespace OpenLoco::Interop;

namespace OpenLoco::Jukebox
{
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
    
    // Queue that stores all the songs in the current playlist
    static std::queue<MusicID> playlist;

    static std::vector<MusicId> findAllSongs()
    {
        std::vector<MusicId> songs(kNumMusicTracks);
        std::iota(songs.begin(), songs.end(), 0);
        return songs;
    }

    static std::vector<MusicId> findCurrentEraSongs()
    {
        std::vector<MusicId> songs;
        auto currentYear = getCurrentYear();

        for (auto i = 0; i < kNumMusicTracks; i++)
        {
            const auto& mi = kMusicInfo[i];
            if (currentYear >= mi.startYear && currentYear <= mi.endYear)
            {
                songs.push_back(i);
            }
        }

        return songs;
    }

    static std::vector<MusicId> findCustomSelectionSongs()
    {
        std::vector<MusicId> songs;

        const auto& cfg = Config::get().old;
        for (auto i = 0; i < kNumMusicTracks; i++)
        {
            if (cfg.enabledMusic[i] & 1)
            {
                songs.push_back(i);
            }
        }

        return songs;
    }

    static void generatePlaylist()
    {
        using Config::MusicPlaylistType;
        
        // Find selected songs
        std::vector<MusicID> selectedSongs;
        switch (Config::get().old.musicPlaylist)
        {
            case MusicPlaylistType::currentEra:
                selectedSongs = findCurrentEraSongs();
                break;
            case MusicPlaylistType::all:
                selectedSongs = findAllSongs();
                break;
            case MusicPlaylistType::custom:
                selectedSongs = findCustomSelectionSongs();
                break;
            default:
                throw Exception::RuntimeError("Invalid MusicPlaylistType");
        }

        // Scramble selected songs and populate playlist
        std::random_shuffle(selectedSongs.begin(), selectedSongs.end());
        for (MusicID song : selectedSongs) {
            playlist.push(song);
        }
    }

    static MusicId chooseNextMusicTrack(MusicId lastSong)
    {
        if (playlist.empty()) {
            generatePlaylist();
        }
        MusicID track = playlist.pop();
        playlist.push(track);
        return track;
    }

    static const MusicInfo& getMusicInfo(MusicId track)
    {
        return kMusicInfo[track];
    }
}
