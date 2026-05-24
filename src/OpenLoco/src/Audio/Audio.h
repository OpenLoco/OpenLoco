#pragma once

#include "Location.hpp"
#include "Types.hpp"
#include <OpenLoco/Audio/AudioEngine.h>
#include <OpenLoco/Engine/World.hpp>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

namespace OpenLoco::Environment
{
    enum class PathId : uint8_t;
}

namespace OpenLoco::Audio
{
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
        applause1 = 13,
        error = 14,
        multiplayerConnected = 15,
        multiplayerDisconnected = 16,
        demolishTree = 17,
        demolishBuilding = 18,
        vehiclePlace = 19,
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

    constexpr bool isObjectSoundId(SoundId id)
    {
        return static_cast<int32_t>(id) & 0x8000;
    }

    constexpr SoundId makeObjectSoundId(SoundObjectId_t id)
    {
        return static_cast<SoundId>((static_cast<int32_t>(id) | 0x8000));
    }

    void initialiseDSound();
    void disposeDSound();
    void close();

    const std::vector<std::string>& getDevices();
    const char* getCurrentDeviceName();
    size_t getCurrentDevice();
    void setDevice(size_t index);

    void toggleSound();
    void pauseSound();
    void unpauseSound();
    bool isAudioEnabled();

    void playSound(SoundId id, ChannelId channel, const World::Pos3& loc);
    void playSound(SoundId id, ChannelId channel, const World::Pos3& loc, int32_t pan);
    void playSound(SoundId id, ChannelId channel, int32_t pan);
    void playSound(SoundId id, ChannelId channel, const World::Pos3& loc, int32_t volume, int32_t frequency);

    std::optional<BufferId> getSoundBuffer(SoundId id);
    bool shouldSoundLoop(SoundId id);
    AudioHandle play(SoundId id, ChannelId channel, const AudioAttributes& attribs = {});

    void update();

    void setBgmVolume(int32_t volume);

    void stopVehicleNoise();
    void stopVehicleNoise(EntityId head);

    void stopAmbientNoise();

    void revalidateCurrentTrack();
    void resetMusic();
    void playBackgroundMusic();
    void stopMusic();
    void pauseMusic();
    void unpauseMusic();
    void playMusic(Environment::PathId sample, int32_t volume, bool loop);

    void resetSoundObjects();

    std::optional<BufferId> loadMusicSample(Environment::PathId asset);

    int32_t calculatePan(const coord_t coord, const int32_t screenSize);

    constexpr int32_t kVolumeDbMin = -6000;

    inline int32_t percentToDb(int32_t percent)
    {
        if (percent <= 0)
        {
            return kVolumeDbMin;
        }
        if (percent >= 100)
        {
            return 0;
        }
        return static_cast<int32_t>(2000.0f * std::log10(static_cast<float>(percent) / 100.0f));
    }

    inline int32_t dbToPercent(int32_t db)
    {
        if (db <= kVolumeDbMin)
        {
            return 0;
        }
        if (db >= 0)
        {
            return 100;
        }
        return static_cast<int32_t>(100.0f * std::pow(10.0f, static_cast<float>(db) / 2000.0f));
    }
}
