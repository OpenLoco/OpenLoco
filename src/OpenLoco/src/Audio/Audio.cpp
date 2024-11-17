#include "Audio.h"
#include "Channel.h"
#include "Config.h"
#include "Date.h"
#include "Entities/EntityManager.h"
#include "Environment.h"
#include "Game.h"
#include "GameStateFlags.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Map/SurfaceElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/SoundObject.h"
#include "Objects/TreeObject.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "VehicleChannel.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Core/FileStream.h>
#include <OpenLoco/Interop/Interop.hpp>
#include <array>
#include <cassert>
#include <numeric>
#include <unordered_map>

#ifdef _WIN32
#define __HAS_DEFAULT_DEVICE__
#endif

using namespace OpenLoco::Environment;
using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Utility;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Audio
{
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
    // 0x0050D5B0
    static std::optional<PathId> _chosenAmbientNoisePathId = std::nullopt;

    static uint8_t _numActiveVehicleSounds; // 0x0112C666
    static std::vector<std::string> _devices;
    static std::vector<Channel> _channels;
    static std::vector<VehicleChannel> _vehicleChannels;
    static std::vector<Channel> _soundFX;

    static std::vector<uint32_t> _samples;
    static std::unordered_map<uint16_t, uint32_t> _objectSamples;
    static std::unordered_map<PathId, uint32_t> _musicSamples;

    static OpenAL::Device _device;
    static OpenAL::SourceManager _sourceManager;
    static OpenAL::BufferManager _bufferManager;

    static void playSound(SoundId id, const World::Pos3& loc, int32_t volume, int32_t pan, int32_t frequency);
    static void mixSound(SoundId id, bool loop, int32_t volume, int32_t pan, int32_t freq);

    // 0x004FE910
    static constexpr MusicInfo kMusicInfo[] = {
        { PathId::music_chuggin_along, StringIds::music_chuggin_along, 1925, 1933 },
        { PathId::music_long_dusty_road, StringIds::music_long_dusty_road, 1927, 1935 },
        { PathId::music_flying_high, StringIds::music_flying_high, 1932, 1940 },
        { PathId::music_gettin_on_the_gas, StringIds::music_gettin_on_the_gas, 1956, 1964 },
        { PathId::music_jumpin_the_rails, StringIds::music_jumpin_the_rails, 1953, 1961 },
        { PathId::music_smooth_running, StringIds::music_smooth_running, 1976, 1984 },
        { PathId::music_traffic_jam, StringIds::music_traffic_jam, 1973, 1981 },
        { PathId::music_never_stop_til_you_get_there, StringIds::music_never_stop_til_you_get_there, 1970, 1978 },
        { PathId::music_soaring_away, StringIds::music_soaring_away, 1990, 9999 },
        { PathId::music_techno_torture, StringIds::music_techno_torture, 1993, 9999 },
        { PathId::music_everlasting_high_rise, StringIds::music_everlasting_high_rise, 1996, 9999 },
        { PathId::music_solace, StringIds::music_solace, 1912, 1920 },
        { PathId::music_chrysanthemum, StringIds::music_chrysanthemum, 0, 1911 },
        { PathId::music_eugenia, StringIds::music_eugenia, 0, 1908 },
        { PathId::music_the_ragtime_dance, StringIds::music_the_ragtime_dance, 1909, 1917 },
        { PathId::music_easy_winners, StringIds::music_easy_winners, 0, 1914 },
        { PathId::music_setting_off, StringIds::music_setting_off, 1929, 1937 },
        { PathId::music_a_travellers_serenade, StringIds::music_a_travellers_serenade, 1940, 1948 },
        { PathId::music_latino_trip, StringIds::music_latino_trip, 1943, 1951 },
        { PathId::music_a_good_head_of_steam, StringIds::music_a_good_head_of_steam, 1950, 1958 },
        { PathId::music_hop_to_the_bop, StringIds::music_hop_to_the_bop, 1946, 1954 },
        { PathId::music_the_city_lights, StringIds::music_the_city_lights, 1980, 1988 },
        { PathId::music_steamin_down_town, StringIds::music_steamin_down_town, 1960, 1968 },
        { PathId::music_bright_expectations, StringIds::music_bright_expectations, 1983, 1991 },
        { PathId::music_mo_station, StringIds::music_mo_station, 1963, 1971 },
        { PathId::music_far_out, StringIds::music_far_out, 1966, 1974 },
        { PathId::music_running_on_time, StringIds::music_running_on_time, 1986, 1994 },
        { PathId::music_get_me_to_gladstone_bay, StringIds::music_get_me_to_gladstone_bay, 1918, 1926 },
        { PathId::music_sandy_track_blues, StringIds::music_sandy_track_blues, 1921, 1929 }
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
        const auto id = _bufferManager.allocate(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(pcm), pcmLen), format.nSamplesPerSec, format.nChannels == 2, format.wBitsPerSample);
        return id;
    }

    static std::vector<uint32_t> loadSoundsFromCSS(const fs::path& path)
    {
        Logging::verbose("loadSoundsFromCSS({})", path.string());

        try
        {
            std::vector<uint32_t> results;

            FileStream fs(path, StreamMode::read);
            auto numSounds = fs.readValue<uint32_t>();

            std::vector<uint32_t> soundOffsets(numSounds, 0);
            fs.read(soundOffsets.data(), numSounds * sizeof(uint32_t));

            std::vector<std::byte> pcm;
            for (uint32_t i = 0; i < numSounds; i++)
            {
                // Navigate to beginning of wave data
                fs.setPosition(soundOffsets[i]);

                // Read length of wave data and load it into the pcm buffer
                auto pcmLen = fs.readValue<uint32_t>();
                auto format = fs.readValue<WAVEFORMATEX>();

                pcm.resize(pcmLen);
                fs.read(pcm.data(), pcmLen);

                results.push_back(loadSoundFromWaveMemory(format, pcm.data(), pcmLen));
            }

            return results;
        }
        catch (const std::exception& ex)
        {
            Logging::error("loadSoundsFromCSS({}) failed: {}", path.string(), ex.what());
            return {};
        }
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
        const auto& cfg = Config::get();
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
            {
                getDevices();
            }

            if (!devices.empty())
            {
                return devices[0].c_str();
            }
#endif
        }

        const auto& cfg = Config::get();
        return cfg.audio.device.c_str();
    }

    size_t getCurrentDevice()
    {
        const auto& cfg = Config::get();
        return getDeviceIndex(cfg.audio.device);
    }

    void setDevice(size_t index)
    {
        if (index < _devices.size())
        {
            auto& cfg = Config::get();
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
            Config::write();
            reinitialise();
        }
    }

    // 0x00489C0A
    void toggleSound()
    {
        _audioIsEnabled = !_audioIsEnabled;
        if (_audioIsEnabled)
        {
            return;
        }

        stopVehicleNoise();
        stopAmbientNoise();
        stopMusic();
        Config::write();
    }

    // 0x00489C34
    void pauseSound()
    {
        if (_audioIsPaused)
        {
            return;
        }

        _audioIsPaused = true;
        stopVehicleNoise();
        stopAmbientNoise();
        stopMusic();
    }

    // 0x00489C58
    void unpauseSound()
    {
        _audioIsPaused = false;
    }

    static const SoundObject* getSoundObject(SoundId id)
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
            loco_global<int32_t[32], 0x004FEAB8> _unk_4FEAB8;
            return _unk_4FEAB8[(int32_t)id];
        }
    }

    static int32_t calculateVolumeFromViewport([[maybe_unused]] SoundId id, const World::Pos3& mpos, const Viewport& viewport)
    {
        auto volume = 0;
        auto zVol = 0;
        auto tile = World::TileManager::get(mpos);
        if (!tile.isNull())
        {
            auto surface = tile.surface();
            if (surface != nullptr)
            {
                if ((surface->baseHeight()) - 5 > mpos.z)
                {
                    zVol = 8;
                }
            }
            volume = ((-1024 * viewport.zoom - 1) << zVol) + 1;
        }
        return volume;
    }

    void playSound(SoundId id, const World::Pos3& loc)
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
        loco_global<uint8_t[64], 0x0050D514> _unk_50D514;
        if (isObjectSoundId(id))
        {
            auto obj = getSoundObject(id);
            return obj->var_06 != 0;
        }
        else
        {
            return _unk_50D514[(int32_t)id * 2] != 0;
        }
    }

    // 0x0048A4BF
    void playSound(Vehicles::Vehicle2or6* v)
    {
        if ((v->soundFlags & Vehicles::SoundFlags::flag0) != Vehicles::SoundFlags::none)
        {
            Logging::verbose("playSound(vehicle #{})", enumValue(v->id));
            auto vc = getFreeVehicleChannel();
            if (vc != nullptr)
            {
                vc->begin(v->id);
            }
        }
    }

    // 0x00489F1B
    void playSound(SoundId id, const World::Pos3& loc, int32_t volume, int32_t frequency)
    {
        playSound(id, loc, volume, kPlayAtLocation, frequency);
    }

    // 0x00489CB5
    void playSound(SoundId id, const World::Pos3& loc, int32_t pan)
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
    void playSound(SoundId id, const World::Pos3& loc, int32_t volume, int32_t pan, int32_t frequency)
    {
        if (_audioIsEnabled)
        {
            volume += getVolumeForSoundId(id);
            if (pan == kPlayAtLocation)
            {
                auto vpos = World::gameToScreen(loc, WindowManager::getCurrentRotation());
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

            const auto& cfg = Config::get().old;
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
                    auto* data = obj->data;
                    assert(data->offset == 8);
                    _objectSamples[static_cast<size_t>(id)] = loadSoundFromWaveMemory(data->pcmHeader, data->pcm(), data->length);
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
        Logging::verbose("mixSound({}, {}, {}, {}, {})", enumValue(id), loop ? "true" : "false", volume, pan, freq);
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
        try
        {
            FileStream fs(path, StreamMode::read);

            const auto sig = fs.readValue<uint32_t>();
            if (sig != 0x46464952) // RIFF
            {
                throw Exception::RuntimeError("Invalid signature.");
            }

            fs.readValue<uint32_t>(); // size

            const auto riffType = fs.readValue<uint32_t>();
            if (riffType != 0x45564157) // WAVE
            {
                throw Exception::RuntimeError("Invalid format.");
            }

            const auto fmtMarker = fs.readValue<uint32_t>();
            // This can be 'fmt\0' or 'fmt '
            if (fmtMarker != 0x20746d66 && fmtMarker != 0x00746d66)
            {
                throw Exception::RuntimeError("Invalid format marker.");
            }

            fs.readValue<uint32_t>(); // header size

            const auto typeFormat = fs.readValue<uint16_t>();
            if (typeFormat != 1)
            {
                throw Exception::RuntimeError("Invalid format type, expected PCM.");
            }

            const auto channels = fs.readValue<uint16_t>();
            const auto sampleRate = fs.readValue<uint32_t>();

            fs.readValue<uint32_t>();
            fs.readValue<uint16_t>();

            const auto bits = fs.readValue<uint16_t>();

            const auto dataMarker = fs.readValue<uint32_t>();
            if (dataMarker != 0x61746164) // data
            {
                throw Exception::RuntimeError("Invalid data marker.");
            }

            const auto pcmLen = fs.readValue<uint32_t>();

            std::vector<std::uint8_t> pcm(pcmLen);
            fs.read(pcm.data(), pcmLen);

            const auto id = _bufferManager.allocate(std::span<const uint8_t>(pcm.data(), pcmLen), sampleRate, channels == 2, bits);
            _musicSamples[asset] = id;

            return id;
        }
        catch (const std::exception& ex)
        {
            Logging::error("Unable to load music sample '{}': {}", path, ex.what());
            return std::nullopt;
        }
    }

    // 0x00401A05
    static void stopChannel(ChannelId id)
    {
        Logging::verbose("stopChannel({})", static_cast<int>(id));

        auto channel = getChannel(id);
        if (channel != nullptr)
        {
            channel->stop();
        }
    }

    static void sub_48A274(Vehicles::Vehicle2or6* v)
    {
        if (v == nullptr)
        {
            return;
        }

        if (v->drivingSoundId == SoundObjectId::null)
        {
            return;
        }

        // TODO: left or top?
        if (v->spriteLeft == Location::null)
        {
            return;
        }

        if (_numActiveVehicleSounds >= Config::get().old.maxVehicleSounds)
        {
            return;
        }

        auto spritePosition = viewport_pos(v->spriteLeft, v->spriteTop);

        auto main = WindowManager::getMainWindow();
        if (main != nullptr && main->viewports[0] != nullptr)
        {
            auto viewport = main->viewports[0];
            ViewportRect extendedViewport = {};

            auto quarterWidth = viewport->viewWidth / 4;
            auto quarterHeight = viewport->viewHeight / 4;
            extendedViewport.left = viewport->viewX - quarterWidth;
            extendedViewport.top = viewport->viewY - quarterHeight;
            extendedViewport.right = viewport->viewX + viewport->viewWidth + quarterWidth;
            extendedViewport.bottom = viewport->viewY + viewport->viewHeight + quarterHeight;

            if (extendedViewport.contains(spritePosition))
            {
                // jump + return
                _numActiveVehicleSounds += 1;
                v->soundFlags |= Vehicles::SoundFlags::flag0;
                v->soundWindowType = main->type;
                v->soundWindowNumber = main->number;
                return;
            }
        }

        if (WindowManager::count() == 0)
        {
            return;
        }

        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            auto w = WindowManager::get(i);

            if (w->type == WindowType::main)
            {
                continue;
            }

            if (w->type == WindowType::news)
            {
                continue;
            }

            auto viewport = w->viewports[0];
            if (viewport == nullptr)
            {
                continue;
            }

            if (viewport->contains(spritePosition))
            {
                _numActiveVehicleSounds += 1;
                v->soundFlags |= Vehicles::SoundFlags::flag0;
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
                v->soundFlags &= ~Vehicles::SoundFlags::flag0;
                break;
            case 1:
                if ((v->soundFlags & Vehicles::SoundFlags::flag1) == Vehicles::SoundFlags::none)
                {
                    sub_48A274(v);
                }
                break;
            case 2:
                if ((v->soundFlags & Vehicles::SoundFlags::flag1) != Vehicles::SoundFlags::none)
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

        for (auto* v : VehicleManager::VehicleList())
        {
            Vehicles::Vehicle train(*v);
            off_4FEB58(reinterpret_cast<Vehicles::Vehicle2or6*>(train.veh2), x);
            off_4FEB58(reinterpret_cast<Vehicles::Vehicle2or6*>(train.tail), x);
        }
    }

    // 0x48A73B
    void updateVehicleNoise()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded))
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

    void stopVehicleNoise(EntityId head)
    {
        Vehicles::Vehicle train(head);
        for (auto& vc : _vehicleChannels)
        {
            if (vc.getId() == train.veh2->id
                || vc.getId() == train.tail->id)
            {
                vc.stop();
            }
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
        {
            return;
        }

        auto* mainViewport = WindowManager::getMainViewport();
        std::optional<PathId> newAmbientSound = std::nullopt;
        int32_t maxVolume = kAmbientMinVolume;

        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && mainViewport != nullptr)
        {
            maxVolume = getAmbientMaxVolume(mainViewport->zoom);
            const auto centre = mainViewport->getCentreMapPosition();
            const auto topLeft = World::toTileSpace(centre) - World::TilePos2{ 5, 5 };
            const auto bottomRight = topLeft + World::TilePos2{ 11, 11 };
            const auto searchRange = World::getClampedRange(topLeft, bottomRight);

            size_t waterCount = 0;      // bl
            size_t wildernessCount = 0; // bh
            size_t treeCount = 0;       // cx
            for (auto& tilePos : searchRange)
            {
                const auto tile = World::TileManager::get(tilePos);
                bool passedSurface = false;
                for (const auto& el : tile)
                {
                    auto* elSurface = el.as<World::SurfaceElement>();
                    if (elSurface != nullptr)
                    {
                        passedSurface = true;
                        if (elSurface->water() != 0)
                        {
                            waterCount++;
                            break;
                        }
                        else if (elSurface->snowCoverage() && elSurface->isLast())
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
                    auto* elTree = el.as<World::TreeElement>();
                    if (passedSurface && elTree != nullptr)
                    {
                        const auto* treeObj = ObjectManager::get<TreeObject>(elTree->treeObjectId());
                        if (!treeObj->hasFlags(TreeObjectFlags::droughtResistant))
                        {
                            treeCount++;
                        }
                    }
                }
            }

            if (waterCount > kAmbientNumWaterTilesForOcean)
            {
                newAmbientSound = PathId::css3;
            }
            else if (wildernessCount > kAmbientNumMountainTilesForWilderness)
            {
                newAmbientSound = PathId::css2;
            }
            else if (treeCount > kAmbientNumTreeTilesForForest)
            {
                newAmbientSound = PathId::css4;
            }
        }
        auto* channel = getChannel(ChannelId::ambient);
        if (channel == nullptr)
        {
            return;
        }

        // TODO: Consider changing this so that we ask if the channel is playing a certain
        // buffer instead of storing what buffer is playing indirectly through the global
        // variable _chosenAmbientNoisePathId

        // In these situations quieten until channel stopped
        if (!newAmbientSound.has_value() || (channel->isPlaying() && _chosenAmbientNoisePathId != *newAmbientSound))
        {
            const auto newVolume = channel->getAttributes().volume - kAmbientVolumeChangePerTick;
            if (newVolume < kAmbientMinVolume)
            {
                _chosenAmbientNoisePathId = std::nullopt;
                channel->stop();
            }
            else
            {
                channel->setVolume(newVolume);
            }
            return;
        }

        if (_chosenAmbientNoisePathId != *newAmbientSound)
        {
            auto musicBuffer = loadMusicSample(*newAmbientSound);
            if (musicBuffer.has_value())
            {
                channel->load(*musicBuffer);
                channel->setVolume(kAmbientMinVolume);
                channel->play(true);
                _chosenAmbientNoisePathId = *newAmbientSound;
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
        const auto& cfg = Config::get().old;

        if (_currentSong == kNoSong)
        {
            return;
        }

        bool trackStillApplies = true;
        switch (cfg.musicPlaylist)
        {
            case MusicPlaylistType::currentEra:
            {
                auto currentYear = getCurrentYear();
                auto info = kMusicInfo[_currentSong];
                if (currentYear < info.startYear || currentYear > info.endYear)
                {
                    trackStillApplies = false;
                }
                break;
            }

            case MusicPlaylistType::all:
                return;

            case MusicPlaylistType::custom:
                if (!cfg.enabledMusic[_currentSong])
                {
                    trackStillApplies = false;
                }
                break;
        }

        if (!trackStillApplies)
        {
            stopMusic();
            _currentSong = kNoSong;
            _lastSong = kNoSong;
        }
    }

    static std::vector<uint8_t> makeAllMusicPlaylist()
    {
        std::vector<uint8_t> playlist(kNumMusicTracks);
        std::iota(playlist.begin(), playlist.end(), 0);
        return playlist;
    }

    static std::vector<uint8_t> makeCurrentEraPlaylist()
    {
        auto playlist = std::vector<uint8_t>();
        auto currentYear = getCurrentYear();

        for (auto i = 0; i < kNumMusicTracks; i++)
        {
            const auto& mi = kMusicInfo[i];
            if (currentYear >= mi.startYear && currentYear <= mi.endYear)
            {
                playlist.push_back(i);
            }
        }

        return playlist;
    }

    static std::vector<uint8_t> makeCustomSelectionPlaylist()
    {
        auto playlist = std::vector<uint8_t>();

        const auto& cfg = Config::get().old;
        for (auto i = 0; i < kNumMusicTracks; i++)
        {
            if (cfg.enabledMusic[i] & 1)
            {
                playlist.push_back(i);
            }
        }

        return playlist;
    }

    std::vector<uint8_t> makeSelectedPlaylist()
    {
        using MusicPlaylistType = Config::MusicPlaylistType;

        switch (Config::get().old.musicPlaylist)
        {
            case MusicPlaylistType::currentEra:
                return makeCurrentEraPlaylist();

            case MusicPlaylistType::all:
                return makeAllMusicPlaylist();

            case MusicPlaylistType::custom:
                return makeCustomSelectionPlaylist();
        }

        throw Exception::RuntimeError("Invalid MusicPlaylistType");
    }

    static int32_t chooseNextMusicTrack()
    {
        using MusicPlaylistType = Config::MusicPlaylistType;

        auto playlist = makeSelectedPlaylist();

        const auto& cfg = Config::get().old;

        if (playlist.empty() && cfg.musicPlaylist != MusicPlaylistType::currentEra)
        {
            playlist = makeCurrentEraPlaylist();
        }

        if (playlist.empty())
        {
            playlist = makeAllMusicPlaylist();
        }

        // Remove _lastSong if it is present and not the only song, so that you do not get the same song twice.
        // Assumes there is no more than one occurence of that song in the playlist.
        if (playlist.size() > 1)
        {
            auto position = std::find(playlist.begin(), playlist.end(), *_lastSong);
            if (position != playlist.end())
            {
                playlist.erase(position);
            }
        }

        auto r = std::rand() % playlist.size();
        auto track = playlist[r];
        return track;
    }

    // 0x0048A78D
    void playBackgroundMusic()
    {
        auto& cfg = Config::get().old;
        if (cfg.musicPlaying == 0 || isTitleMode() || isEditorMode() || isPaused())
        {
            return;
        }

        auto* channel = getChannel(ChannelId::music);
        if (channel == nullptr)
        {
            return;
        }

        if (!channel->isPlaying())
        {
            // Not playing, but the 'current song' is last song? It's been requested manually!
            bool requestedSong = _lastSong != kNoSong && _lastSong == _currentSong;

            // Choose a track to play, unless we have requested one track in particular.
            if (_currentSong == kNoSong || !requestedSong)
            {
                _lastSong = _currentSong;
                _currentSong = chooseNextMusicTrack();
            }
            else
            {
                // We're choosing this one, but the next one should be decided automatically again.
                _lastSong = kNoSong;
            }

            // Load info on the song to play.
            const auto& mi = kMusicInfo[_currentSong];
            playMusic(mi.pathId, cfg.volume, false);

            WindowManager::invalidate(WindowType::options);
        }
    }

    static PathId currentTrackPathId;

    // 0x0048AC66
    // previously called void playTitleScreenMusic()
    void playMusic(PathId sample, int32_t volume, bool loop)
    {
        auto* channel = getChannel(ChannelId::music);
        if (!_audioInitialised || _audioIsPaused || !_audioIsEnabled || channel == nullptr)
        {
            return;
        }

        if (currentTrackPathId == sample)
        {
            return;
        }

        currentTrackPathId = sample;
        channel->stop();

        auto musicSample = loadMusicSample(sample);
        if (channel->load(*musicSample))
        {
            channel->setVolume(volume);
            channel->play(loop);
        }
    }

    // 0x0048AAD2
    void resetMusic()
    {
        stopMusic();
        _currentSong = kNoSong;
        _lastSong = kNoSong;
    }

    // 0x0048AAE8
    // void stopBackgroundMusic()
    // merged into Audio::stopMusic

    // 0x0048AC2B
    // previously called void stopTitleMusic()
    void stopMusic()
    {
        auto* channel = getChannel(ChannelId::music);
        if (_audioInitialised && channel != nullptr && channel->isPlaying())
        {
            channel->stop();
            currentTrackPathId = PathId::g1; // there is no 'null' or 'none' so just set it to something that isn't a music track
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
        if (Config::get().old.volume == volume)
        {
            return;
        }

        auto& cfg = Config::get().old;
        cfg.volume = volume;
        Config::write();

        auto* channel = getChannel(ChannelId::music);
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
