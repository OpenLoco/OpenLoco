#pragma once

#include "../types.hpp"
#include <tuple>

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

    enum class channel_id
    {
        bgm,
        unk_1,
        ambient,
        title,
    };
    constexpr int32_t num_reserved_channels = 4;

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

    bool load_channel(channel_id id, const char* path, int32_t c);
    bool play_channel(channel_id id, int32_t loop, int32_t volume, int32_t d, int32_t freq);
    void stop_channel(channel_id id);
    void set_channel_volume(channel_id id, int32_t volume);
    bool is_channel_playing(channel_id id);

    void update_ambient_noise();
    void play_background_music();
    void play_title_screen_music();

    /**
     * Converts a Locomotion volume range to SDL2.
     * @remarks Not constexpr as it requires an SDL2 macro and we avoid
     *          library header includes in our own headers.
     */
    int32_t volume_loco_to_sdl(int32_t loco);

    /**
     * Converts a Locomotion pan range to a left and right value for SDL2 mixer.
     */
    constexpr std::tuple<int32_t, int32_t> pan_loco_to_sdl(int32_t pan)
    {
        constexpr auto range = 2048.0f;
        if (pan == 0)
        {
            return std::make_tuple(0, 0);
        }
        else if (pan < 0)
        {
            auto r = (int32_t)(255 - ((pan / -range) * 255));
            return std::make_tuple(255, r);
        }
        else
        {
            auto r = (int32_t)(255 - ((pan / range) * 255));
            return std::make_tuple(r, 255);
        }
    }
}
