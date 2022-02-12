#pragma once

#include "Core/FileSystem.hpp"

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
        _1tmp,
        objects,
        scenarios,
    };

    void autoCreateDirectory(const fs::path& path);
    fs::path getPath(PathId id);
    fs::path getPathNoWarning(PathId id);
    void resolvePaths();
}
