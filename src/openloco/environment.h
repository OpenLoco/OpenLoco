#pragma once

#ifdef _OPENLOCO_USE_BOOST_FS_
#include <boost/filesystem.hpp>
#else
#include <experimental/filesystem>
#endif

namespace openloco::environment
{
#ifdef _OPENLOCO_USE_BOOST_FS_
    namespace fs = boost::filesystem;
#else
    namespace fs = std::experimental::filesystem;
#endif

    enum class path_id
    {
        g1,
        plugin1,
        plugin2,
        css1,
        css2,
        css3,
        css4,
        css5,
        gamecfg,
        kanji,
        music_20s1,
        music_20s2,
        music_20s4,
        music_50s1,
        music_50s2,
        music_70s1,
        music_70s2,
        music_70s3,
        music_80s1,
        music_90s1,
        music_90s2,
        music_rag3,
        music_chrysanthemum,
        music_eugenia,
        music_rag2,
        music_rag1,
        music_20s3,
        music_40s1,
        music_40s2,
        music_50s3,
        music_40s3,
        music_80s2,
        music_60s1,
        music_80s3,
        music_60s2,
        music_60s3,
        music_80s4,
        music_20s5,
        music_20s6,
        title,
        scores,
        boulder_breakers,
        tut1024_1,
        tut1024_2,
        tut1024_3,
        tut800_1,
        tut800_2,
        tut800_3,
        openloco_yml,
    };

    fs::path get_path(path_id id);
    void resolve_paths();
}
