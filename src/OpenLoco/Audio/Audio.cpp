#include "Audio.h"
#include "../Config.h"
#include "../Console.h"
#include "../Date.h"
#include "../Entities/EntityManager.h"
#include "../Environment.h"
#include "../Game.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/TileLoop.hpp"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/SoundObject.h"
#include "../Objects/TreeObject.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Stream.hpp"
#include "../Vehicles/Vehicle.h"
#include "Channel.h"
#include "VehicleChannel.h"
#include <array>
#include <cassert>
#include <fstream>
#include <unordered_map>

#ifdef _WIN32
#define __HAS_DEFAULT_DEVICE__
#endif

using namespace OpenLoco::Environment;
using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Utility;

namespace OpenLoco::Audio
{
#pragma pack(push, 1)
    struct WAVEFORMATEX
    {
        int16_t wFormatTag;
        int16_t nChannels;
        int32_t nSamplesPerSec;
        int32_t nAvgBytesPerSec;
        int16_t nBlockAlign;
        int16_t wBitsPerSample;
        int16_t cbSize;
    };

    struct SoundObjectData
    {
        int32_t var_00;
        int32_t offset;
        uint32_t length;
        WAVEFORMATEX pcm_header;

        const void* pcm()
        {
            return (void*)((uintptr_t)this + sizeof(SoundObjectData));
        }
    };
#pragma pack(pop)

    struct AudioFormat
    {
        int32_t frequency{};
        int32_t format{};
        int32_t channels{};
    };

    [[maybe_unused]] constexpr int32_t kPlayAtCentre = 0x8000;
    [[maybe_unused]] constexpr int32_t kPlayAtLocation = 0x8001;
    [[maybe_unused]] constexpr int32_t kNumSoundChannels = 16;

    static constexpr uint8_t kNoSong = 0xFF;

    static loco_global<uint32_t, 0x0050D1EC> _audioInitialised;
    static loco_global<uint8_t, 0x0050D434> _currentSong;
    static loco_global<uint8_t, 0x0050D435> _lastSong;
    static loco_global<bool, 0x0050D554> _audioIsPaused;
    static loco_global<bool, 0x0050D555> _audioIsEnabled;
    static loco_global<PathId, 0x0050D5B0> _chosenAmbientNoisePathId;

    static uint8_t _numActiveVehicleSounds; // 0x0112C666
    static std::vector<std::string> _devices;
    static std::vector<Channel> _channels;
    static std::vector<VehicleChannel> _vehicleChannels;
    static std::vector<Channel> _soundFX;
    static ChannelId _musicCurrentChannel = ChannelId::bgm;

    static std::vector<uint32_t> _samples;
    static std::unordered_map<uint16_t, uint32_t> _objectSamples;
    static std::unordered_map<PathId, uint32_t> _musicSamples;

    static OpenAL::Device _device;
    static OpenAL::SourceManager _sourceManager;
    static OpenAL::BufferManager _bufferManager;

    static void playSound(SoundId id, const Map::Pos3& loc, int32_t volume, int32_t pan, int32_t frequency);
    static void mixSound(SoundId id, bool loop, int32_t volume, int32_t pan, int32_t freq);

    // 0x004FE910
    static const MusicInfo kMusicInfo[] = {
        { PathId::music_20s1, StringIds::music_chuggin_along, 1925, 1933 },
        { PathId::music_20s2, StringIds::music_long_dusty_road, 1927, 1935 },
        { PathId::music_20s4, StringIds::music_flying_high, 1932, 1940 },
        { PathId::music_50s1, StringIds::music_gettin_on_the_gas, 1956, 1964 },
        { PathId::music_50s2, StringIds::music_jumpin_the_rails, 1953, 1961 },
        { PathId::music_70s1, StringIds::music_smooth_running, 1976, 1984 },
        { PathId::music_70s2, StringIds::music_traffic_jam, 1973, 1981 },
        { PathId::music_70s3, StringIds::music_never_stop_til_you_get_there, 1970, 1978 },
        { PathId::music_80s1, StringIds::music_soaring_away, 1990, 9999 },
        { PathId::music_90s1, StringIds::music_techno_torture, 1993, 9999 },
        { PathId::music_90s2, StringIds::music_everlasting_high_rise, 1996, 9999 },
        { PathId::music_rag3, StringIds::music_solace, 1912, 1920 },
        { PathId::music_chrysanthemum, StringIds::music_chrysanthemum, 0, 1911 },
        { PathId::music_eugenia, StringIds::music_eugenia, 0, 1908 },
        { PathId::music_rag2, StringIds::music_the_ragtime_dance, 1909, 1917 },
        { PathId::music_rag1, StringIds::music_easy_winners, 0, 1914 },
        { PathId::music_20s3, StringIds::music_setting_off, 1929, 1937 },
        { PathId::music_40s1, StringIds::music_a_travellers_seranade, 1940, 1948 },
        { PathId::music_40s2, StringIds::music_latino_trip, 1943, 1951 },
        { PathId::music_50s3, StringIds::music_a_good_head_of_steam, 1950, 1958 },
        { PathId::music_40s3, StringIds::music_hop_to_the_bop, 1946, 1954 },
        { PathId::music_80s2, StringIds::music_the_city_lights, 1980, 1988 },
        { PathId::music_60s1, StringIds::music_steamin_down_town, 1960, 1968 },
        { PathId::music_80s3, StringIds::music_bright_expectations, 1983, 1991 },
        { PathId::music_60s2, StringIds::music_mo_station, 1963, 1971 },
        { PathId::music_60s3, StringIds::music_far_out, 1966, 1974 },
        { PathId::music_80s4, StringIds::music_running_on_time, 1986, 1994 },
        { PathId::music_20s5, StringIds::music_get_me_to_gladstone_bay, 1918, 1926 },
        { PathId::music_20s6, StringIds::music_sandy_track_blues, 1921, 1929 }
    };

    static Channel* getChannel(ChannelId id)
    {
        auto index = static_cast<size_t>(id);
        if (index < _channels.size())
        {
            return &_channels[index];
        }
        return nullptr;
    }

    static uint32_t loadSoundFromWaveMemory(const WAVEFORMATEX& format, const void* pcm, size_t pcmLen)
    {
        const auto id = _bufferManager.allocate(stdx::span<const uint8_t>(reinterpret_cast<const uint8_t*>(pcm), pcmLen), format.nSamplesPerSec, format.nChannels == 2, format.wBitsPerSample);
        return id;
    }

    static std::vector<uint32_t> loadSoundsFromCSS(const fs::path& path)
    {
        Console::logVerbose("loadSoundsFromCSS(%s)", path.string().c_str());
        std::vector<uint32_t> results;
        std::ifstream fs(path, std::ios::in | std::ios::binary);

        if (fs.is_open())
        {
            auto numSounds = readValue<uint32_t>(fs);

            std::vector<uint32_t> soundOffsets(numSounds, 0);
            readData(fs, soundOffsets.data(), numSounds);

            std::vector<std::byte> pcm;
            for (uint32_t i = 0; i < numSounds; i++)
            {
                // Navigate to beginning of wave data
                fs.seekg(soundOffsets[i]);

                // Read length of wave data and load it into the pcm buffer
                auto pcmLen = readValue<uint32_t>(fs);
                auto format = readValue<WAVEFORMATEX>(fs);

                pcm.resize(pcmLen);
                readData(fs, pcm.data(), pcmLen);

                results.push_back(loadSoundFromWaveMemory(format, pcm.data(), pcmLen));
            }
        }
        return results;
    }

    static void disposeSamples()
    {
        _samples.clear();
        _objectSamples.clear();
        _musicSamples.clear();
    }

    static void disposeChannels()
    {
        _channels.clear();
        _vehicleChannels.clear();
        _soundFX.clear();
    }

    static void reinitialise()
    {
        disposeDSound();
        initialiseDSound();
    }

    static size_t getDeviceIndex(const std::string_view& deviceName)
    {
        if (deviceName.empty())
        {
            return 0;
        }

        const auto& devices = _devices;
        if (devices.empty())
        {
            getDevices();
        }

        auto fr = std::find(devices.begin(), devices.end(), deviceName);
        if (fr != devices.end())
        {
            return fr - devices.begin();
        }
        return std::numeric_limits<size_t>().max();
    }

    // 0x00404E53
    void initialiseDSound()
    {
        if (_devices.empty())
        {
            _devices = getDevices();
        }
        std::string deviceName{};
        const auto& cfg = Config::getNew();
        if (!cfg.audio.device.empty())
        {
            if (std::find(std::begin(_devices), std::end(_devices), cfg.audio.device) != std::end(_devices))
            {
                deviceName = cfg.audio.device;
            }
        }

        _device.open(deviceName);
        _channels.clear();
        for (auto i = 0; i < 4; ++i)
        {
            const auto sourceId = _sourceManager.allocate();
            _channels.push_back(Channel(sourceId));
        }
        _vehicleChannels.clear();
        for (auto i = 0; i < 10; ++i)
        {
            const auto sourceId = _sourceManager.allocate();
            _vehicleChannels.push_back(Channel(sourceId));
        }

        auto css1path = Environment::getPath(Environment::PathId::css1);
        _samples = loadSoundsFromCSS(css1path);
        _audioInitialised = 1;
    }

    // 0x00404E58
    void disposeDSound()
    {
        disposeChannels();
        disposeSamples();
        _sourceManager.dispose();
        _bufferManager.dispose();
        _device.close();
        _musicCurrentChannel = (ChannelId)-1;
        _audioInitialised = 0;
    }

    // 0x00489BA1
    void close()
    {
        call(0x00489BA1);
    }

#ifdef __HAS_DEFAULT_DEVICE__
    static const char* getDefaultDeviceName()
    {
        return StringManager::getString(StringIds::default_audio_device_name);
    }
#endif

    const std::vector<std::string>& getDevices()
    {
        _devices.clear();
#ifdef __HAS_DEFAULT_DEVICE__
        auto* ddChar = getDefaultDeviceName();
        std::string defaultDevice = ddChar == nullptr ? "" : std::string(ddChar);
        if (!defaultDevice.empty())
        {
            _devices.push_back(getDefaultDeviceName());
        }
#endif
        auto newDevices = _device.getAvailableDeviceNames();
        for (auto& device : newDevices)
        {
#ifdef __HAS_DEFAULT_DEVICE__
            if (defaultDevice.empty() || device != defaultDevice)
#endif
            {
                _devices.push_back(device);
            }
        }
        return _devices;
    }

    const char* getCurrentDeviceName()
    {
        auto index = getCurrentDevice();
        if (index == 0)
        {
#ifdef __HAS_DEFAULT_DEVICE__
            return getDefaultDeviceName();
#else
            const auto& devices = _devices;
            if (devices.empty())
                getDevices();

            if (!devices.empty())
                return devices[0].c_str();
#endif
        }

        const auto& cfg = Config::getNew();
        return cfg.audio.device.c_str();
    }

    size_t getCurrentDevice()
    {
        const auto& cfg = Config::getNew();
        return getDeviceIndex(cfg.audio.device);
    }

    void setDevice(size_t index)
    {
        if (index < _devices.size())
        {
            auto& cfg = Config::getNew();
#ifdef __HAS_DEFAULT_DEVICE__
            if (index == 0)
            {
                cfg.audio.device = {};
            }
            else
#endif
            {
                cfg.audio.device = _devices[index];
            }
            Config::writeNewConfig();
            reinitialise();
        }
    }

    // 0x00489C0A
    void toggleSound()
    {
        _audioIsEnabled = !_audioIsEnabled;
        if (_audioIsEnabled)
            return;

        stopVehicleNoise();
        stopBackgroundMusic();
        stopAmbientNoise();
        stopTitleMusic();
        Config::write();
    }

    // 0x00489C34
    void pauseSound()
    {
        if (_audioIsPaused)
            return;

        _audioIsPaused = true;
        stopVehicleNoise();
        stopBackgroundMusic();
        stopAmbientNoise();
    }

    // 0x00489C58
    void unpauseSound()
    {
        _audioIsPaused = false;
    }

    static SoundObject* getSoundObject(SoundId id)
    {
        auto idx = (int32_t)id & ~0x8000;
        return ObjectManager::get<SoundObject>(idx);
    }

    static Viewport* findBestViewportForSound(viewport_pos vpos)
    {
        auto w = WindowManager::find(WindowType::main, 0);
        if (w != nullptr)
        {
            auto viewport = w->viewports[0];
            if (viewport != nullptr && viewport->contains(vpos))
            {
                return viewport;
            }
        }

        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            w = WindowManager::get(i);
            if (w != nullptr && w->type != WindowType::main && w->type != WindowType::news)
            {
                auto viewport = w->viewports[0];
                if (viewport != nullptr && viewport->contains(vpos))
                {
                    return viewport;
                }
            }
        }

        return nullptr;
    }

    static int32_t getVolumeForSoundId(SoundId id)
    {
        if (isObjectSoundId(id))
        {
            auto obj = getSoundObject(id);
            if (obj != nullptr)
            {
                return obj->volume;
            }
            return 0;
        }
        else
        {
            loco_global<int32_t[32], 0x004FEAB8> unk_4FEAB8;
            return unk_4FEAB8[(int32_t)id];
        }
    }

    static int32_t calculateVolumeFromViewport(SoundId id, const Map::Pos3& mpos, const Viewport& viewport)
    {
        auto volume = 0;
        auto zVol = 0;
        auto tile = Map::TileManager::get(mpos);
        if (!tile.isNull())
        {
            auto surface = tile.surface();
            if (surface != nullptr)
            {
                if ((surface->baseZ() * 4) - 5 > mpos.z)
                {
                    zVol = 8;
                }
            }
            volume = ((-1024 * viewport.zoom - 1) << zVol) + 1;
        }
        return volume;
    }

    void playSound(SoundId id, const Map::Pos3& loc)
    {
        playSound(id, loc, kPlayAtLocation);
    }

    void playSound(SoundId id, int32_t pan)
    {
        playSound(id, {}, pan);
    }

    static VehicleChannel* getFreeVehicleChannel()
    {
        for (auto& vc : _vehicleChannels)
        {
            if (vc.isFree())
            {
                return &vc;
            }
        }
        return nullptr;
    }

    bool shouldSoundLoop(SoundId id)
    {
        loco_global<uint8_t[64], 0x0050D514> unk_50D514;
        if (isObjectSoundId(id))
        {
            auto obj = getSoundObject(id);
            return obj->var_06 != 0;
        }
        else
        {
            return unk_50D514[(int32_t)id * 2] != 0;
        }
    }

    // 0x0048A4BF
    void playSound(Vehicles::Vehicle2or6* v)
    {
        if (v->var_4A & 1)
        {
            Console::logVerbose("playSound(vehicle #%d)", v->id);
            auto vc = getFreeVehicleChannel();
            if (vc != nullptr)
            {
                vc->begin(v->id);
            }
        }
    }

    // 0x00489F1B
    void playSound(SoundId id, const Map::Pos3& loc, int32_t volume, int32_t frequency)
    {
        playSound(id, loc, volume, kPlayAtLocation, frequency);
    }

    // 0x00489CB5
    void playSound(SoundId id, const Map::Pos3& loc, int32_t pan)
    {
        playSound(id, loc, 0, pan, 22050);
    }

    constexpr int32_t kVpSizeMin = 64; // Note check if defined in viewport.hpp

    int32_t calculatePan(const coord_t coord, const int32_t screenSize)
    {
        const auto relativePosition = (coord << 16) / std::max(screenSize, kVpSizeMin);
        return (relativePosition - (1 << 15)) / 16;
    }

    // 0x00489CB5 / 0x00489F1B
    // pan is in UI pixels or known constant
    void playSound(SoundId id, const Map::Pos3& loc, int32_t volume, int32_t pan, int32_t frequency)
    {
        if (_audioIsEnabled)
        {
            volume += getVolumeForSoundId(id);
            if (pan == kPlayAtLocation)
            {
                auto vpos = Map::gameToScreen(loc, WindowManager::getCurrentRotation());
                auto viewport = findBestViewportForSound(vpos);
                if (viewport == nullptr)
                {
                    return;
                }

                volume += calculateVolumeFromViewport(id, loc, *viewport);
                pan = viewport->viewportToScreen(vpos).x;
                if (volume < -10000)
                {
                    return;
                }
            }
            else if (pan == kPlayAtCentre)
            {
                pan = 0;
            }

            const auto& cfg = Config::get();
            if (cfg.var_1E == 0)
            {
                pan = 0;
            }
            else if (pan != 0)
            {
                pan = calculatePan(pan, Ui::width());
            }

            mixSound(id, 0, volume, pan, frequency);
        }
    }

    std::optional<uint32_t> getSoundSample(SoundId id)
    {
        if (isObjectSoundId(id))
        {
            // TODO this is not getting deallocated when sound objects unloaded
            // TODO use a LRU queue for object samples
            auto sr = _objectSamples.find((uint16_t)id);
            if (sr == _objectSamples.end())
            {
                auto obj = getSoundObject(id);
                if (obj != nullptr)
                {
                    auto data = (SoundObjectData*)obj->data;
                    assert(data->offset == 8);
                    _objectSamples[static_cast<size_t>(id)] = loadSoundFromWaveMemory(data->pcm_header, data->pcm(), data->length);
                    return _objectSamples[static_cast<size_t>(id)];
                }
            }
            else
            {
                return sr->second;
            }
        }
        else if (static_cast<size_t>(id) < _samples.size())
        {
            return _samples[static_cast<size_t>(id)];
        }
        return std::nullopt;
    }

    static void mixSound(SoundId id, bool loop, int32_t volume, int32_t pan, int32_t freq)
    {
        Console::logVerbose("mixSound(%d, %s, %d, %d, %d)", (int32_t)id, loop ? "true" : "false", volume, pan, freq);
        auto sample = getSoundSample(id);
        if (sample)
        {
            auto freeChannel = std::find_if(std::begin(_soundFX), std::end(_soundFX), [](auto& channel) { return !channel.isPlaying(); });
            Channel* channel = nullptr;
            if (freeChannel == std::end(_soundFX))
            {
                _soundFX.push_back(Channel(_sourceManager.allocate()));
                channel = &_soundFX.back();
            }
            else
            {
                channel = &*freeChannel;
            }
            channel->load(*sample);
            channel->setVolume(volume);
            channel->setFrequency(freq);
            channel->setPan(pan);
            channel->play(loop);
        }
    }

    // 0x0048A18C
    void updateSounds()
    {
        if (_soundFX.empty())
        {
            return;
        }
        for (auto& channel : _soundFX)
        {
            if (!channel.isPlaying())
            {
                channel.stop(); // This forces deallocation of buffer
            }
        }
    }

    static std::optional<uint32_t> loadMusicSample(PathId asset)
    {
        auto res = _musicSamples.find(asset);
        if (res != std::end(_musicSamples))
        {
            return res->second;
        }
        const auto path = Environment::getPath(asset);
        std::ifstream fs(path, std::ios::in | std::ios::binary);

        if (fs.is_open())
        {
            char buffer[5]{};
            // Read length of wave data and load it into the pcm buffer
            fs.read(buffer, 4);      // RIFF
            readValue<uint32_t>(fs); // size
            fs.read(buffer, 4);      // WAVE
            fs.read(buffer, 4);      // fmt
            readValue<uint32_t>(fs); // headersize
            readValue<uint16_t>(fs); // PCM
            const auto channels = readValue<uint16_t>(fs);
            const auto sampleRate = readValue<uint32_t>(fs);
            readValue<uint32_t>(fs);
            readValue<uint16_t>(fs);
            const auto bits = readValue<uint16_t>(fs);
            fs.read(buffer, 4); // data
            auto pcmLen = readValue<uint32_t>(fs);
            std::vector<std::uint8_t> pcm;
            pcm.resize(pcmLen);
            readData(fs, pcm.data(), pcmLen);
            const auto id = _bufferManager.allocate(stdx::span<const uint8_t>(pcm.data(), pcmLen), sampleRate, channels == 2, bits);
            _musicSamples[asset] = id;
            return id;
        }
        return std::nullopt;
    }

    // 0x00401A05
    static void stopChannel(ChannelId id)
    {
        Console::logVerbose("stopChannel(%d)", id);

        auto channel = getChannel(id);
        if (channel != nullptr)
        {
            channel->stop();
        }
    }

    static void sub_48A274(Vehicles::Vehicle2or6* v)
    {
        if (v == nullptr)
            return;

        if (v->drivingSoundId == SoundObjectId::null)
            return;

        // TODO: left or top?
        if (v->sprite_left == Location::null)
            return;

        if (_numActiveVehicleSounds >= Config::get().maxVehicleSounds)
            return;

        auto spritePosition = viewport_pos(v->sprite_left, v->sprite_top);

        auto main = WindowManager::getMainWindow();
        if (main != nullptr && main->viewports[0] != nullptr)
        {
            auto viewport = main->viewports[0];
            ViewportRect extendedViewport = {};

            auto quarterWidth = viewport->view_width / 4;
            auto quarterHeight = viewport->view_height / 4;
            extendedViewport.left = viewport->view_x - quarterWidth;
            extendedViewport.top = viewport->view_y - quarterHeight;
            extendedViewport.right = viewport->view_x + viewport->view_width + quarterWidth;
            extendedViewport.bottom = viewport->view_y + viewport->view_height + quarterHeight;

            if (extendedViewport.contains(spritePosition))
            {
                // jump + return
                _numActiveVehicleSounds += 1;
                v->var_4A |= 1;
                v->soundWindowType = main->type;
                v->soundWindowNumber = main->number;
                return;
            }
        }

        if (WindowManager::count() == 0)
            return;

        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            auto w = WindowManager::get(i);

            if (w->type == WindowType::main)
                continue;

            if (w->type == WindowType::news)
                continue;

            auto viewport = w->viewports[0];
            if (viewport == nullptr)
                continue;

            if (viewport->contains(spritePosition))
            {
                _numActiveVehicleSounds += 1;
                v->var_4A |= 1;
                v->soundWindowType = w->type;
                v->soundWindowNumber = w->number;
                return;
            }
        }
    }

    static void off_4FEB58(Vehicles::Vehicle2or6* v, int32_t x)
    {
        switch (x)
        {
            case 0:
                v->var_4A &= ~1;
                break;
            case 1:
                if (!(v->var_4A & 2))
                {
                    sub_48A274(v);
                }
                break;
            case 2:
                if (v->var_4A & 2)
                {
                    sub_48A274(v);
                }
                break;
            case 3:
                playSound(v);
                break;
        }
    }

    // 0x0048A1FA
    static void sub_48A1FA(int32_t x)
    {
        if (x == 0)
        {
            _numActiveVehicleSounds = 0;
        }

        for (auto v : EntityManager::VehicleList())
        {
            Vehicles::Vehicle train(*v);
            off_4FEB58(reinterpret_cast<Vehicles::Vehicle2or6*>(train.veh2), x);
            off_4FEB58(reinterpret_cast<Vehicles::Vehicle2or6*>(train.tail), x);
        }
    }

    // 0x48A73B
    void updateVehicleNoise()
    {
        if (Game::hasFlags(1u << 0))
        {
            if (!_audioIsPaused && _audioIsEnabled)
            {
                sub_48A1FA(0);
                sub_48A1FA(1);
                sub_48A1FA(2);
                for (auto& vc : _vehicleChannels)
                {
                    vc.update();
                }
                sub_48A1FA(3);
            }
        }
    }

    // 0x00489C6A
    void stopVehicleNoise()
    {
        for (auto& vc : _vehicleChannels)
        {
            vc.stop();
        }
    }

    static constexpr auto kAmbientMinVolume = -3500;
    static constexpr auto kAmbientVolumeChangePerTick = 100;
    static constexpr auto kAmbientNumWaterTilesForOcean = 60;
    static constexpr auto kAmbientNumTreeTilesForForest = 30;
    static constexpr auto kAmbientNumMountainTilesForWilderness = 60;

    static constexpr int32_t getAmbientMaxVolume(uint8_t zoom)
    {
        constexpr int32_t _volumes[]{ -1200, -2000, -3000, -3000 };
        return _volumes[zoom];
    }

    // 0x0048ACFD
    void updateAmbientNoise()
    {
        if (!_audioInitialised || _audioIsPaused || !_audioIsEnabled)
            return;

        auto* mainViewport = WindowManager::getMainViewport();
        std::optional<PathId> ambientSound = std::nullopt;
        int32_t maxVolume = kAmbientMinVolume;

        if (!Game::hasFlags((1u << 0)) && mainViewport != nullptr)
        {
            maxVolume = getAmbientMaxVolume(mainViewport->zoom);
            const auto centre = mainViewport->getCentreMapPosition();
            const auto topLeft = Map::TilePos2{ centre } - Map::TilePos2{ 5, 5 };
            const auto bottomRight = topLeft + Map::TilePos2{ 11, 11 };
            Map::TilePosRangeView searchRange(topLeft, bottomRight);
            size_t waterCount = 0;      // bl
            size_t wildernessCount = 0; // bh
            size_t treeCount = 0;       // cx
            for (auto& tilePos : searchRange)
            {
                if (!Map::validCoords(tilePos))
                {
                    continue;
                }
                const auto tile = Map::TileManager::get(tilePos);
                bool passedSurface = false;
                for (const auto& el : tile)
                {
                    auto* elSurface = el.as<Map::SurfaceElement>();
                    if (elSurface != nullptr)
                    {
                        passedSurface = true;
                        if (elSurface->water() != 0)
                        {
                            waterCount++;
                            break;
                        }
                        else if (elSurface->var_4_E0() && elSurface->isLast())
                        {
                            wildernessCount++;
                            break;
                        }
                        else if (elSurface->baseZ() >= 64 && elSurface->isLast())
                        {
                            wildernessCount++;
                            break;
                        }
                        continue;
                    }
                    auto* elTree = el.as<Map::TreeElement>();
                    if (passedSurface && elTree != nullptr)
                    {
                        const auto* treeObj = ObjectManager::get<TreeObject>(elTree->treeObjectId());
                        if (!(treeObj->flags & TreeObjectFlags::droughtResistant))
                        {
                            treeCount++;
                        }
                    }
                }
            }

            if (waterCount > kAmbientNumWaterTilesForOcean)
            {
                ambientSound = PathId::css3;
            }
            else if (wildernessCount > kAmbientNumMountainTilesForWilderness)
            {
                ambientSound = PathId::css2;
            }
            else if (treeCount > kAmbientNumTreeTilesForForest)
            {
                ambientSound = PathId::css4;
            }
        }
        auto* channel = getChannel(ChannelId::ambient);
        if (channel == nullptr)
        {
            return;
        }
        // In these situations quieten until channel stopped
        if (!ambientSound.has_value() || (channel->isPlaying() && _chosenAmbientNoisePathId != *ambientSound))
        {
            const auto newVolume = channel->getAttributes().volume - kAmbientVolumeChangePerTick;
            if (newVolume < kAmbientMinVolume)
            {
                channel->stop();
            }
            else
            {
                channel->setVolume(newVolume);
            }
            return;
        }

        if (_chosenAmbientNoisePathId != *ambientSound)
        {
            auto musicBuffer = loadMusicSample(*ambientSound);
            if (musicBuffer.has_value())
            {
                channel->load(*musicBuffer);
                channel->setVolume(kAmbientMinVolume);
                channel->play(true);
            }
        }
        else
        {
            auto newVolume = std::min(channel->getAttributes().volume + kAmbientVolumeChangePerTick, maxVolume);
            channel->setVolume(newVolume);
        }
    }

    // 0x0048ABE3
    void stopAmbientNoise()
    {
        loco_global<uint32_t, 0x0050D5AC> _50D5AC;
        if (_audioInitialised && _50D5AC != 1)
        {
            stopChannel(ChannelId::ambient);
            _50D5AC = 1;
        }
    }

    // 0x0048AA0C
    void revalidateCurrentTrack()
    {
        using MusicPlaylistType = Config::MusicPlaylistType;
        const auto& cfg = Config::get();

        if (_currentSong == kNoSong)
            return;

        bool trackStillApplies = true;
        switch (cfg.musicPlaylist)
        {
            case MusicPlaylistType::currentEra:
            {
                auto currentYear = getCurrentYear();
                auto info = kMusicInfo[_currentSong];
                if (currentYear < info.startYear || currentYear > info.endYear)
                    trackStillApplies = false;
                break;
            }

            case MusicPlaylistType::all:
                return;

            case MusicPlaylistType::custom:
                if (!cfg.enabledMusic[_currentSong])
                    trackStillApplies = false;
                break;
        }

        if (!trackStillApplies)
        {
            stopBackgroundMusic();
            _currentSong = kNoSong;
            _lastSong = kNoSong;
        }
    }

    static void addAllSongsToPlaylist(int32_t excludeTrack, std::vector<uint8_t>& playlist)
    {
        for (auto i = 0; i < kNumMusicTracks; i++)
        {
            if (i != excludeTrack)
            {
                playlist.push_back(i);
            }
        }
    }

    static void addCurrentEraSongsToPlaylist(int32_t excludeTrack, std::vector<uint8_t>& playlist)
    {
        auto currentYear = getCurrentYear();
        for (auto i = 0; i < kNumMusicTracks; i++)
        {
            const auto& mi = kMusicInfo[i];
            if (currentYear >= mi.startYear && currentYear <= mi.endYear)
            {
                if (i != excludeTrack)
                {
                    playlist.push_back(i);
                }
            }
        }
    }

    static int32_t chooseNextMusicTrack(int32_t excludeTrack)
    {
        using MusicPlaylistType = Config::MusicPlaylistType;

        static std::vector<uint8_t> playlist;
        playlist.clear();

        const auto& cfg = Config::get();
        switch (cfg.musicPlaylist)
        {
            case MusicPlaylistType::currentEra:
                addCurrentEraSongsToPlaylist(excludeTrack, playlist);
                break;
            case MusicPlaylistType::all:
                addAllSongsToPlaylist(excludeTrack, playlist);
                break;
            case MusicPlaylistType::custom:
                for (auto i = 0; i < kNumMusicTracks; i++)
                {
                    if (i != excludeTrack && (cfg.enabledMusic[i] & 1))
                    {
                        playlist.push_back(i);
                    }
                }
                break;
        }

        if (playlist.empty() && cfg.musicPlaylist != MusicPlaylistType::currentEra)
        {
            addCurrentEraSongsToPlaylist(excludeTrack, playlist);
        }

        if (playlist.empty())
        {
            addAllSongsToPlaylist(excludeTrack, playlist);
        }

        auto r = std::rand() % playlist.size();
        auto track = playlist[r];
        return track;
    }

    // 0x0048A78D
    void playBackgroundMusic()
    {
        if (!_audioInitialised || _audioIsPaused || !_audioIsEnabled)
        {
            return;
        }

        auto cfg = Config::get();
        if (cfg.musicPlaying == 0 || isTitleMode() || isEditorMode())
        {
            return;
        }

        if (_musicCurrentChannel != ChannelId::bgm || !_channels[enumValue(_musicCurrentChannel)].isPlaying())
        {
            // Not playing, but the 'current song' is last song? It's been requested manually!
            bool requestedSong = _lastSong != kNoSong && _lastSong == _currentSong;

            // Choose a track to play, unless we have requested one track in particular.
            if (_currentSong == kNoSong || !requestedSong)
            {
                auto trackToExclude = _lastSong;
                _lastSong = _currentSong;
                _currentSong = chooseNextMusicTrack(trackToExclude);
            }
            else
            {
                // We're choosing this one, but the next one should be decided automatically again.
                _lastSong = kNoSong;
            }

            // Load info on the song to play.
            const auto& mi = kMusicInfo[_currentSong];
            auto buffer = loadMusicSample(mi.pathId);
            if (_channels[enumValue(ChannelId::bgm)].load(*buffer))
            {
                _musicCurrentChannel = ChannelId::bgm;
                if (!_channels[enumValue(_musicCurrentChannel)].play(false))
                {
                    cfg.musicPlaying = 0;
                }
            }
            else
            {
                cfg.musicPlaying = 0;
            }

            WindowManager::invalidate(WindowType::options);
        }
    }

    // 0x0048AAD2
    void resetMusic()
    {
        stopBackgroundMusic();
        _currentSong = kNoSong;
        _lastSong = kNoSong;
    }

    // 0x0048AAE8
    void stopBackgroundMusic()
    {
        if (_audioInitialised && _musicCurrentChannel != static_cast<ChannelId>(-1) && _channels[enumValue(_musicCurrentChannel)].isPlaying())
        {
            _channels[enumValue(_musicCurrentChannel)].stop();
        }
    }

    // 0x0048AC66
    void playTitleScreenMusic()
    {
        auto* channel = getChannel(ChannelId::title);
        if (channel == nullptr)
        {
            return;
        }
        if (isTitleMode() && _audioInitialised && _audioIsEnabled && Config::getNew().audio.playTitleMusic)
        {
            if (!channel->isPlaying())
            {
                auto musicSample = loadMusicSample(PathId::css5);
                if (channel->load(*musicSample))
                {
                    channel->setVolume(-500);
                    channel->play(true);
                }
            }
        }
        else
        {
            if (channel->isPlaying())
            {
                channel->stop();
            }
        }
    }

    // 0x0048AC2B
    void stopTitleMusic()
    {
        auto* channel = getChannel(ChannelId::title);
        if (channel == nullptr)
        {
            return;
        }
        if (channel->isPlaying())
        {
            channel->stop();
        }
    }

    void resetSoundObjects()
    {
        for (auto& sample : _objectSamples)
        {
            _bufferManager.deAllocate(sample.second);
        }
        _objectSamples.clear();
    }

    bool isAudioEnabled()
    {
        return _audioIsEnabled;
    }

    const MusicInfo* getMusicInfo(MusicId track)
    {
        return &kMusicInfo[track];
    }

    // 0x0048AA67
    void setBgmVolume(int32_t volume)
    {
        if (Config::get().volume == volume)
        {
            return;
        }

        auto& cfg = Config::get();
        cfg.volume = volume;
        Config::write();

        auto* channel = getChannel(ChannelId::bgm);
        if (channel == nullptr)
        {
            return;
        }
        if (_audioInitialised && _currentSong != kNoSong)
        {
            channel->setVolume(volume);
        }
    }
}
