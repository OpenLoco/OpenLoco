#pragma once

#include "../Environment.h"
#include "../Location.hpp"
#include "../Map/Map.hpp"
#include "../Types.hpp"
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace OpenLoco::Vehicles
{
    struct Vehicle2or6;
}

namespace OpenLoco::Audio
{
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
        Environment::PathId pathId;
        string_id titleId;
        uint16_t startYear;
        uint16_t endYear;
    };

    void initialiseDSound();
    void disposeDSound();
    void close();

    const std::vector<std::string>& getDevices();
    const char* getCurrentDeviceName();
    size_t getCurrentDevice();
    void setDevice(size_t index);

    std::optional<uint32_t> getSoundSample(SoundId id);
    bool shouldSoundLoop(SoundId id);

    void toggleSound();
    void pauseSound();
    void unpauseSound();
    void playSound(Vehicles::Vehicle2or6* t);
    void playSound(SoundId id, const Map::Pos3& loc);

    // FOR HOOKS ONLY DO NOT USE THIS FUNCTION FOR OPENLOCO CODE
    // INSTEAD USE playSound(SoundId id, const Map::Pos3& loc) OR playSound(SoundId id, int32_t pan)
    void playSound(SoundId id, const Map::Pos3& loc, int32_t pan);

    void playSound(SoundId id, int32_t pan);
    void playSound(SoundId id, const Map::Pos3& loc, int32_t volume, int32_t frequency);
    void updateSounds();

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

    void resetSoundObjects();

    bool isAudioEnabled();

    const MusicInfo* getMusicInfo(MusicId track);
    constexpr int32_t kNumMusicTracks = 29;

    constexpr bool isObjectSoundId(SoundId id)
    {
        return static_cast<int32_t>(id) & 0x8000;
    }

    constexpr SoundId makeObjectSoundId(SoundObjectId_t id)
    {
        return static_cast<SoundId>((static_cast<int32_t>(id) | 0x8000));
    }

    int32_t calculatePan(const coord_t coord, const int32_t screenSize);
}
