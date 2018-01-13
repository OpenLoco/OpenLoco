#include "environment.h"
#include "interop/interop.hpp"
#include "utility/collection.hpp"

namespace openloco::environment
{
    loco_global_array<char, 260, 0x009D118E> _path_buffer;
    loco_global_array<char, 257, 0x0050B0CE> _path_install;
    loco_global_array<char, 257, 0x0050B1CF> _path_saves_single_player;
    loco_global_array<char, 257, 0x0050B2EC> _path_saves_two_player;
    loco_global_array<char, 257, 0x0050B406> _path_scenarios;
    loco_global_array<char, 257, 0x0050B518> _path_landscapes;
    loco_global_array<char, 257, 0x0050B635> _path_objects;

    static fs::path get_sub_path(path_id id);

    fs::path get_loco_install_path()
    {
        return "C:/Program Files (x86)/Atari/Locomotion";
    }

    // 0x004416B5
    fs::path get_path(path_id id)
    {
        auto basePath = get_loco_install_path();
        auto subPath = get_sub_path(id);
        auto result = basePath / subPath;
        return result;
    }

    static void set_directory(char * buffer, fs::path path)
    {
        std::strcpy(buffer, path.make_preferred().u8string().c_str());
    }

    // 0x004412CE
    void resolve_paths()
    {
        auto basePath = get_loco_install_path();
        set_directory(_path_install, basePath);
        set_directory(_path_saves_single_player, basePath / "Single Player Saved Games/");
        set_directory(_path_saves_two_player, basePath / "Two Player Saved Games/");
        set_directory(_path_scenarios, basePath / "Scenarios/*.SC5");
        set_directory(_path_landscapes, basePath / "Scenarios/Landscapes/*.SC5");
        set_directory(_path_objects, basePath / "ObjData/*.DAT");
    }

    static fs::path get_sub_path(path_id id)
    {
        static constexpr const char * paths[] =
        {
            "Data/G1.DAT",
            "Data/PLUGIN.DAT",
            "Data/PLUGIN2.DAT",
            "Data/CSS1.DAT",
            "Data/CSS2.DAT",
            "Data/CSS3.DAT",
            "Data/CSS4.DAT",
            "Data/CSS5.DAT",
            "Data/GAME.CFG",
            "Data/KANJI.DAT",
            "Data/20s1.DAT",
            "Data/20s2.DAT",
            "Data/20s4.DAT",
            "Data/50s1.DAT",
            "Data/50s2.DAT",
            "Data/70s1.DAT",
            "Data/70s2.DAT",
            "Data/70s3.DAT",
            "Data/80s1.DAT",
            "Data/90s1.DAT",
            "Data/90s2.DAT",
            "Data/rag3.DAT",
            "Data/Chrysanthemum.DAT",
            "Data/Eugenia.DAT",
            "Data/Rag2.DAT",
            "Data/Rag1.DAT",
            "Data/20s3.DAT",
            "Data/40s1.DAT",
            "Data/40s2.DAT",
            "Data/50s3.DAT",
            "Data/40s3.DAT",
            "Data/80s2.DAT",
            "Data/60s1.DAT",
            "Data/80s3.DAT",
            "Data/60s2.DAT",
            "Data/60s3.DAT",
            "Data/80s4.DAT",
            "Data/20s5.DAT",
            "Data/20s6.DAT",
            "Data/title.dat",
            "Data/Scores.DAT",
            "Scenarios/Boulder Breakers.SC5",
            "Data/TUT1024_1.DAT",
            "Data/TUT1024_2.DAT",
            "Data/TUT1024_3.DAT",
            "Data/TUT800_1.DAT",
            "Data/TUT800_2.DAT",
            "Data/TUT800_3.DAT"
        };

        size_t index = (size_t)id;
        if (index >= utility::length(paths))
        {
            throw std::runtime_error("Invalid path_id: " + std::to_string((int32_t)id));
        }
        return paths[(size_t)id];
    }
}
