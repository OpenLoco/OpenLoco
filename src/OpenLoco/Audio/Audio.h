#pragma once

#include "../Environment.h"
#include "../Location.hpp"
#include "../Map/Map.hpp"
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
    struct Sample
    {
        void* pcm{};
        size_t len{};
        Mix_Chunk* chunk{};
    };

    // TODO: This should only be a byte needs to be split off from sound object
    enum class SoundId : uint16_t
    {
        clickDown = 0,
        clickUp = 1,
        clickPress = 2,
        construct = 3,
        demolish = 4,
        income = 5,
        crash = 6,
        water = 7,
        splash1 = 8,
        splash2 = 9,
        waypoint = 10,
        notification = 11,
        openWindow = 12,
        applause_1 = 13,
        error = 14,
        unk_15 = 15,
        unk_16 = 16,
        demolishTree = 17,
        demolishBuilding = 18,
        unk_19 = 19,
        vehiclePickup = 20,
        constructShip = 21,
        ticker = 22,
        applause2 = 23,
        newsOooh = 24,
        newsAwww = 25,
        breakdown1 = 26,
        breakdown2 = 27,
        breakdown3 = 28,
        breakdown4 = 29,
        breakdown5 = 30,
        breakdown6 = 31,

        null = 0xFF
    };

    enum class ChannelId
    {
        bgm,
        unk_1,
        ambient,
        title,
        vehicle_0, // * 10
    };
    constexpr int32_t kNumReservedChannels = 4 + 10;

    using MusicId = uint8_t;

    struct MusicInfo
    {
        Environment::path_id path_id;
        string_id title_id;
        uint16_t start_year;
        uint16_t end_year;
    };

    void initialiseDSound();
    void disposeDSound();
    void close();

    const std::vector<std::string>& getDevices();
    const char* getCurrentDeviceName();
    size_t getCurrentDevice();
    void setDevice(size_t index);

    Sample* getSoundSample(SoundId id);
    bool shouldSoundLoop(SoundId id);

    void toggleSound();
    void pauseSound();
    void unpauseSound();
    void playSound(Vehicles::Vehicle2or6* t);
    void playSound(SoundId id, const Map::Pos3& loc);
    void playSound(SoundId id, const Map::Pos3& loc, int32_t pan);
    void playSound(SoundId id, int32_t pan);
    void playSound(SoundId id, const Map::Pos3& loc, int32_t volume, int32_t frequency);
    void updateSounds();

    bool loadChannel(ChannelId id, const char* path, int32_t c);
    bool playChannel(ChannelId id, int32_t loop, int32_t volume, int32_t d, int32_t freq);
    void stopChannel(ChannelId id);
    void setChannelVolume(ChannelId id, int32_t volume);
    bool isChannelPlaying(ChannelId id);
    void setBgmVolume(int32_t volume);

    void updateVehicleNoise();
    void stopVehicleNoise();

    void updateAmbientNoise();
    void stopAmbientNoise();

    void revalidateCurrentTrack();

    void resetMusic();
    void playBackgroundMusic();
    void stopBackgroundMusic();
    void playTitleScreenMusic();
    void stopTitleMusic();

    bool isAudioEnabled();

    const MusicInfo* getMusicInfo(MusicId track);
    constexpr int32_t kNumMusicTracks = 29;

    /**
     * Converts a Locomotion volume range to SDL2.
     * @remarks Not constexpr as it requires an SDL2 macro and we avoid
     *          library header includes in our own headers.
     */
    int32_t volumeLocoToSDL(int32_t loco);

    constexpr bool isObjectSoundId(SoundId id)
    {
        return ((int32_t)id & 0x8000);
    }

    constexpr SoundId makeObjectSoundId(SoundObjectId_t id)
    {
        return (SoundId)((int32_t)id | 0x8000);
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
