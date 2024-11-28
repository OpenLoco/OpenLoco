#pragma once

#include <OpenLoco/Core/FileSystem.hpp>

namespace OpenLoco::Environment
{
    enum class PathId : uint8_t
    {
        g1,
        plugin1,
        plugin2,
        css1, // Sound effects
        css2, // Wind (mountains)
        css3, // Ocean
        css4, // Jungle
        css5, // Title music
        gamecfg,
        kanji,
        music_chuggin_along,
        music_long_dusty_road,
        music_flying_high,
        music_gettin_on_the_gas,
        music_jumpin_the_rails,
        music_smooth_running,
        music_traffic_jam,
        music_never_stop_til_you_get_there,
        music_soaring_away,
        music_techno_torture,
        music_everlasting_high_rise,
        music_solace,
        music_chrysanthemum,
        music_eugenia,
        music_the_ragtime_dance,
        music_easy_winners,
        music_setting_off,
        music_a_travellers_serenade,
        music_latino_trip,
        music_a_good_head_of_steam,
        music_hop_to_the_bop,
        music_the_city_lights,
        music_steamin_down_town,
        music_bright_expectations,
        music_mo_station,
        music_far_out,
        music_running_on_time,
        music_get_me_to_gladstone_bay,
        music_sandy_track_blues,
        title,
        scores,
        boulderBreakers,
        tut1024_1,
        tut1024_2,
        tut1024_3,
        tut800_1,
        tut800_2,
        tut800_3,
        openlocoYML,
        languageFiles,
        save,
        autosave,
        landscape,
        _1tmp,
        vanillaObjects,
        scenarios,
        heightmap,
        customObjects,
        objects,
        screenshots,
    };

    void autoCreateDirectory(const fs::path& path);
    fs::path getPath(PathId id);
    fs::path getPathNoWarning(PathId id);
    void resolvePaths();
    void setLocale();
}
