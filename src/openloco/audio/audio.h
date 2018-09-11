#pragma once

#include "../types.hpp"

namespace openloco::audio
{
    struct sound_instance;

    enum class sound_id
    {
        click_down = 0,
        click_up = 1,
        click_press = 2,
        construct = 3,
        demolish = 4,
        income = 5,
        crash = 6,
        water = 7,
        splash_1 = 8,
        splash_2 = 9,
        waypoint = 10,
        notification = 11,
        open_window = 12,
        applause_1 = 13,
        error = 14,
        unk_15 = 15,
        unk_16 = 16,
        demolish_tree = 17,
        demolish_building = 18,
        unk_19 = 19,
        unk_20 = 20,
        construct_ship = 21,
        ticker = 22,
        applause_2 = 23,
        news_oooh = 24,
        news_awww = 25,
        breakdown_1 = 26,
        breakdown_2 = 27,
        breakdown_3 = 28,
        breakdown_4 = 29,
        breakdown_5 = 30,
        breakdown_6 = 31,
    };

    enum class music_channel
    {
        bgm,
        unk_1,
        ocean,
        title,
    };

    void initialise_dsound();
    void dispose_dsound();
    void initialise();

    void pause_sound();
    void unpause_sound();
    void play_sound(sound_id id, loc16 loc);
    void play_sound(sound_id id, loc16 loc, int32_t pan);
    void play_sound(sound_id id, int32_t pan);
    void play_sound(sound_id id, loc16 loc, int32_t volume, int32_t frequency, bool obj_sound);

    bool prepare_sound(sound_id soundId, sound_instance* sound, int32_t channels, int32_t software);
    void mix_sound(sound_instance* sound, int32_t b, int32_t volume, int32_t pan, int32_t freq);

    bool load_music(int32_t id, const char* path, int32_t c);
    bool play_music(int32_t id, int32_t loop, int32_t volume, int32_t d, int32_t freq);
    void stop_music(int32_t id);
    void set_music_volume(int32_t id, int32_t volume);
    bool is_music_playing(int32_t id);

    void update_ambient_noise();
    void play_background_music();
    void play_title_screen_music();
}
