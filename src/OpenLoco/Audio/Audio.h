#pragma once

#include "../Environment.h"
#include "../Types.hpp"
#include <string>
#include <tuple>
#include <vector>

struct Mix_Chunk;

namespace OpenLoco::Vehicles
{
    struct Vehicle2or6;
}

namespace OpenLoco::Audio
{
    struct sample
    {
        void* pcm{};
        size_t len{};
        Mix_Chunk* chunk{};
    };

    // TODO: This should only be a byte needs to be split off from sound object
    enum class sound_id : uint16_t
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

        null = 0xFF
    };

    enum class channel_id
    {
        bgm,
        unk_1,
        ambient,
        title,
        vehicle_0, // * 10
    };
    constexpr int32_t num_reserved_channels = 4 + 10;

    using music_id = uint8_t;

    struct music_info
    {
        Environment::path_id path_id;
        string_id title_id;
        uint16_t start_year;
        uint16_t end_year;
    };

    void initialiseDSound();
    void disposeDSound();

    const std::vector<std::string>& getDevices();
    const char* getCurrentDeviceName();
    size_t getCurrentDevice();
    void setDevice(size_t index);

    sample* getSoundSample(sound_id id);
    bool shouldSoundLoop(sound_id id);

    void toggleSound();
    void pauseSound();
    void unpauseSound();
    void playSound(Vehicles::Vehicle2or6* t);
    void playSound(sound_id id, loc16 loc);
    void playSound(sound_id id, loc16 loc, int32_t pan);
    void playSound(sound_id id, int32_t pan);
    void playSound(sound_id id, loc16 loc, int32_t volume, int32_t frequency);
    void updateSounds();

    bool loadChannel(channel_id id, const char* path, int32_t c);
    bool playChannel(channel_id id, int32_t loop, int32_t volume, int32_t d, int32_t freq);
    void stopChannel(channel_id id);
    void setChannelVolume(channel_id id, int32_t volume);
    bool isChannelPlaying(channel_id id);
    void setBgmVolume(int32_t volume);

    void updateVehicleNoise();
    void stopVehicleNoise();

    void updateAmbientNoise();
    void stopAmbientNoise();

    void revalidateCurrentTrack();

    void playBackgroundMusic();
    void stopBackgroundMusic();
    void playTitleScreenMusic();
    void stopTitleMusic();

    bool isAudioEnabled();

    const music_info* getMusicInfo(music_id track);
    constexpr int32_t num_music_tracks = 29;

    /**
     * Converts a Locomotion volume range to SDL2.
     * @remarks Not constexpr as it requires an SDL2 macro and we avoid
     *          library header includes in our own headers.
     */
    int32_t volumeLocoToSDL(int32_t loco);

    constexpr bool isObjectSoundId(sound_id id)
    {
        return ((int32_t)id & 0x8000);
    }

    constexpr sound_id makeObjectSoundId(sound_object_id_t id)
    {
        return (sound_id)((int32_t)id | 0x8000);
    }

    /**
     * Converts a Locomotion pan range to a left and right value for SDL2 mixer.
     */
    constexpr std::pair<int32_t, int32_t> panLocoToSDL(int32_t pan)
    {
        constexpr auto range = 2048.0f;
        if (pan == 0)
        {
            return { 0, 0 };
        }
        else if (pan < 0)
        {
            auto r = (int32_t)(255 - ((pan / -range) * 255));
            return { 255, r };
        }
        else
        {
            auto r = (int32_t)(255 - ((pan / range) * 255));
            return { r, 255 };
        }
    }
}
