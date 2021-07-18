#include "Audio.h"
#include "../Config.h"
#include "../Console.h"
#include "../Date.h"
#include "../Entities/EntityManager.h"
#include "../Environment.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/SoundObject.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Stream.hpp"
#include "../Vehicles/Vehicle.h"
#include "Channel.h"
#include "MusicChannel.h"
#include "VehicleChannel.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
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

    [[maybe_unused]] constexpr int32_t play_at_centre = 0x8000;
    [[maybe_unused]] constexpr int32_t play_at_location = 0x8001;
    [[maybe_unused]] constexpr int32_t num_sound_channels = 16;

    static constexpr uint8_t no_song = 0xFF;

    static loco_global<uint32_t, 0x0050D1EC> _audio_initialised;
    static loco_global<uint8_t, 0x0050D434> _currentSong;
    static loco_global<uint8_t, 0x0050D435> _lastSong;
    static loco_global<bool, 0x0050D554> _audioIsPaused;
    static loco_global<bool, 0x0050D555> _audioIsEnabled;

    static uint8_t _numActiveVehicleSounds; // 0x0112C666
    static std::vector<std::string> _devices;
    static AudioFormat _outputFormat;
    static std::array<Channel, 4> _channels;
    static std::array<VehicleChannel, 10> _vehicle_channels;
    static MusicChannel _music_channel;
    static ChannelId _music_current_channel = ChannelId::bgm;

    static std::vector<Sample> _samples;
    static std::unordered_map<uint16_t, Sample> _object_samples;

    static void playSound(SoundId id, const Map::Pos3& loc, int32_t volume, int32_t pan, int32_t frequency);
    static void mixSound(SoundId id, bool loop, int32_t volume, int32_t pan, int32_t freq);

    // 0x004FE910
    static const MusicInfo _musicInfo[] = {
        { path_id::music_20s1, StringIds::music_chuggin_along, 1925, 1933 },
        { path_id::music_20s2, StringIds::music_long_dusty_road, 1927, 1935 },
        { path_id::music_20s4, StringIds::music_flying_high, 1932, 1940 },
        { path_id::music_50s1, StringIds::music_gettin_on_the_gas, 1956, 1964 },
        { path_id::music_50s2, StringIds::music_jumpin_the_rails, 1953, 1961 },
        { path_id::music_70s1, StringIds::music_smooth_running, 1976, 1984 },
        { path_id::music_70s2, StringIds::music_traffic_jam, 1973, 1981 },
        { path_id::music_70s3, StringIds::music_never_stop_til_you_get_there, 1970, 1978 },
        { path_id::music_80s1, StringIds::music_soaring_away, 1990, 9999 },
        { path_id::music_90s1, StringIds::music_techno_torture, 1993, 9999 },
        { path_id::music_90s2, StringIds::music_everlasting_high_rise, 1996, 9999 },
        { path_id::music_rag3, StringIds::music_solace, 1912, 1920 },
        { path_id::music_chrysanthemum, StringIds::music_chrysanthemum, 0, 1911 },
        { path_id::music_eugenia, StringIds::music_eugenia, 0, 1908 },
        { path_id::music_rag2, StringIds::music_the_ragtime_dance, 1909, 1917 },
        { path_id::music_rag1, StringIds::music_easy_winners, 0, 1914 },
        { path_id::music_20s3, StringIds::music_setting_off, 1929, 1937 },
        { path_id::music_40s1, StringIds::music_a_travellers_seranade, 1940, 1948 },
        { path_id::music_40s2, StringIds::music_latino_trip, 1943, 1951 },
        { path_id::music_50s3, StringIds::music_a_good_head_of_steam, 1950, 1958 },
        { path_id::music_40s3, StringIds::music_hop_to_the_bop, 1946, 1954 },
        { path_id::music_80s2, StringIds::music_the_city_lights, 1980, 1988 },
        { path_id::music_60s1, StringIds::music_steamin_down_town, 1960, 1968 },
        { path_id::music_80s3, StringIds::music_bright_expectations, 1983, 1991 },
        { path_id::music_60s2, StringIds::music_mo_station, 1963, 1971 },
        { path_id::music_60s3, StringIds::music_far_out, 1966, 1974 },
        { path_id::music_80s4, StringIds::music_running_on_time, 1986, 1994 },
        { path_id::music_20s5, StringIds::music_get_me_to_gladstone_bay, 1918, 1926 },
        { path_id::music_20s6, StringIds::music_sandy_track_blues, 1921, 1929 }
    };

    static constexpr bool isMusicChannel(ChannelId id)
    {
        return (id == ChannelId::bgm || id == ChannelId::title);
    }

    int32_t volumeLocoToSDL(int32_t loco)
    {
        return (int)(SDL_MIX_MAXVOLUME * (SDL_pow(10, (float)loco / 2000)));
    }

    static Channel* getChannel(ChannelId id)
    {
        auto index = (size_t)id;
        if (index < _channels.size())
        {
            return &_channels[index];
        }
        return nullptr;
    }

    static Sample loadSoundFromWaveMemory(const WAVEFORMATEX& format, const void* pcm, size_t pcmLen)
    {
        // Build a CVT to convert the audio
        const auto& dstFormat = _outputFormat;
        SDL_AudioCVT cvt{};
        auto cr = SDL_BuildAudioCVT(
            &cvt,
            AUDIO_S16LSB,
            format.nChannels,
            format.nSamplesPerSec,
            dstFormat.format,
            dstFormat.channels,
            dstFormat.frequency);

        if (cr == -1)
        {
            Console::error("Error during SDL_BuildAudioCVT: %s", SDL_GetError());
            return {};
        }
        else if (cr == 0)
        {
            // No conversion necessary
            Sample s;
            s.pcm = std::malloc(pcmLen);
            if (s.pcm == nullptr)
            {
                throw std::bad_alloc();
            }
            s.len = pcmLen;
            std::memcpy(s.pcm, pcm, s.len);
            s.chunk = Mix_QuickLoad_RAW((uint8_t*)s.pcm, pcmLen);
            return s;
        }
        else
        {
            // Allocate a new buffer with src data for conversion
            cvt.len = pcmLen;
            cvt.buf = (uint8_t*)std::malloc(cvt.len * cvt.len_mult);
            if (cvt.buf == nullptr)
            {
                throw std::bad_alloc();
            }
            std::memcpy(cvt.buf, pcm, pcmLen);
            if (SDL_ConvertAudio(&cvt) != 0)
            {
                Console::error("Error during SDL_ConvertAudio: %s", SDL_GetError());
                return {};
            }

            // Trim converted buffer
            cvt.buf = (uint8_t*)std::realloc(cvt.buf, cvt.len_cvt);
            if (cvt.buf == nullptr)
            {
                throw std::bad_alloc();
            }

            Sample s;
            s.pcm = cvt.buf;
            s.len = cvt.len_cvt;
            s.chunk = Mix_QuickLoad_RAW(cvt.buf, cvt.len_cvt);
            return s;
        }
    }

    static std::vector<Sample> loadSoundsFromCSS(const fs::path& path)
    {
        Console::logVerbose("loadSoundsFromCSS(%s)", path.string().c_str());
        std::vector<Sample> results;
        std::ifstream fs(path, std::ios::in | std::ios::binary);

        if (fs.is_open())
        {
            auto numSounds = readValue<uint32_t>(fs);

            std::vector<uint32_t> offsets(numSounds, 0);
            readData(fs, offsets.data(), numSounds);

            std::vector<std::byte> pcm;
            for (uint32_t i = 0; i < numSounds; i++)
            {
                // Navigate to beginning of wave data
                fs.seekg(offsets[i]);

                // Read length of wave data and load it into the pcm buffer
                auto pcmLen = readValue<uint32_t>(fs);
                auto format = readValue<WAVEFORMATEX>(fs);

                pcm.resize(pcmLen);
                readData(fs, pcm.data(), pcmLen);

                auto s = loadSoundFromWaveMemory(format, pcm.data(), pcmLen);
                results.push_back(s);
            }
        }
        return results;
    }

    static void disposeSamples()
    {
        for (size_t i = 0; i < _samples.size(); i++)
        {
            Mix_FreeChunk(_samples[i].chunk);
            std::free(_samples[i].pcm);
        }
        _samples = {};
    }

    static void disposeChannels()
    {
        std::generate(_channels.begin(), _channels.end(), []() { return Channel(); });
        std::generate(_vehicle_channels.begin(), _vehicle_channels.end(), []() { return VehicleChannel(); });
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
            // If devices is empty we need to load the default audio device to initliase the audio system
            // this will then allow populating the devices
            auto& format = _outputFormat;
            format.frequency = MIX_DEFAULT_FREQUENCY;
            format.format = MIX_DEFAULT_FORMAT;
            format.channels = MIX_DEFAULT_CHANNELS;
            if (Mix_OpenAudioDevice(format.frequency, format.format, format.channels, 1024, nullptr, 0) != 0)
            {
                Console::error("Mix_OpenAudio failed: %s", Mix_GetError());
                return;
            }
            Mix_CloseAudio();
        }
        const char* deviceName = nullptr;
        const auto& cfg = Config::getNew();
        if (!cfg.audio.device.empty())
        {
            deviceName = cfg.audio.device.c_str();

            // Use default device if config device could not be found
            if (getDeviceIndex(deviceName) == std::numeric_limits<size_t>().max())
            {
                deviceName = nullptr;
            }
        }

        auto& format = _outputFormat;
        format.frequency = MIX_DEFAULT_FREQUENCY;
        format.format = MIX_DEFAULT_FORMAT;
        format.channels = MIX_DEFAULT_CHANNELS;
        if (Mix_OpenAudioDevice(format.frequency, format.format, format.channels, 1024, deviceName, 0) != 0)
        {
            Console::error("Mix_OpenAudio failed: %s", Mix_GetError());
            return;
        }
        Mix_AllocateChannels(num_reserved_channels + num_sound_channels);
        Mix_ReserveChannels(num_reserved_channels);

        for (size_t i = 0; i < _channels.size(); i++)
        {
            _channels[i] = Channel(i);
        }
        for (size_t i = 0; i < _vehicle_channels.size(); i++)
        {
            _vehicle_channels[i] = VehicleChannel(Channel(4 + i));
        }

        auto css1path = Environment::getPath(Environment::path_id::css1);
        _samples = loadSoundsFromCSS(css1path);
        _audio_initialised = 1;
    }

    // 0x00404E58
    void disposeDSound()
    {
        disposeSamples();
        disposeChannels();
        _music_channel = {};
        _music_current_channel = (ChannelId)-1;
        Mix_CloseAudio();
        _audio_initialised = 0;
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
        auto defaultDevice = getDefaultDeviceName();
        if (defaultDevice != nullptr)
        {
            _devices.push_back(getDefaultDeviceName());
        }
#endif
        auto count = SDL_GetNumAudioDevices(0);
        for (auto i = 0; i < count; i++)
        {
            auto name = SDL_GetAudioDeviceName(i, 0);
            if (name != nullptr)
            {
                _devices.push_back(name);
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
        playSound(id, loc, play_at_location);
    }

    void playSound(SoundId id, int32_t pan)
    {
        playSound(id, {}, pan);
    }

    static VehicleChannel* getFreeVehicleChannel()
    {
        for (auto& vc : _vehicle_channels)
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
        playSound(id, loc, volume, play_at_location, frequency);
    }

    // 0x00489CB5
    void playSound(SoundId id, const Map::Pos3& loc, int32_t pan)
    {
        playSound(id, loc, 0, pan, 0);
    }

    // 0x00489CB5 / 0x00489F1B
    // pan is in UI pixels or known constant
    void playSound(SoundId id, const Map::Pos3& loc, int32_t volume, int32_t pan, int32_t frequency)
    {
        loco_global<int32_t, 0x00e3f0b8> current_rotation;

        if (_audioIsEnabled)
        {
            volume += getVolumeForSoundId(id);
            if (pan == play_at_location)
            {
                auto vpos = Viewport::mapFrom3d(loc, current_rotation);
                auto viewport = findBestViewportForSound(vpos);
                if (viewport == nullptr)
                {
                    return;
                }

                volume += calculateVolumeFromViewport(id, loc, *viewport);
                pan = viewport->mapToUi(vpos).x;
                if (volume < -10000)
                {
                    return;
                }
            }
            else if (pan == play_at_centre)
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
                auto uiWidth = std::max(64, Ui::width());
                pan = (((pan << 16) / uiWidth) - 0x8000) >> 4;
            }

            mixSound(id, 0, volume, pan, frequency);
        }
    }

    Sample* getSoundSample(SoundId id)
    {
        if (isObjectSoundId(id))
        {
            // TODO use a LRU queue for object samples
            auto sr = _object_samples.find((uint16_t)id);
            if (sr == _object_samples.end())
            {
                auto obj = getSoundObject(id);
                if (obj != nullptr)
                {
                    auto data = (SoundObjectData*)obj->data;
                    assert(data->offset == 8);
                    auto sample = loadSoundFromWaveMemory(data->pcm_header, data->pcm(), data->length);
                    _object_samples[static_cast<size_t>(id)] = sample;
                    return &_object_samples[static_cast<size_t>(id)];
                }
            }
            else
            {
                return &sr->second;
            }
        }
        else if (static_cast<size_t>(id) < _samples.size())
        {
            return &_samples[static_cast<size_t>(id)];
        }
        return nullptr;
    }

    static void mixSound(SoundId id, bool loop, int32_t volume, int32_t pan, int32_t freq)
    {
        Console::logVerbose("mixSound(%d, %s, %d, %d, %d)", (int32_t)id, loop ? "true" : "false", volume, pan, freq);
        auto sample = getSoundSample(id);
        if (sample != nullptr && sample->chunk != nullptr)
        {
            auto loops = loop == 0 ? 0 : -1;
            auto channel = Mix_PlayChannel(-1, sample->chunk, loops);
            auto sdlv = volumeLocoToSDL(volume);
            Mix_Volume(channel, sdlv);

            auto [left, right] = panLocoToSDL(pan);
            Mix_SetPanning(channel, left, right);
        }
    }

    // 0x0048A18C
    void updateSounds()
    {
    }

    static bool loadChannel(ChannelId id, const fs::path& path, int32_t c)
    {
        Console::logVerbose("loadChannel(%d, %s, %d)", id, path.string().c_str(), c);
        if (isMusicChannel(id))
        {
            if (_music_channel.load(path))
            {
                _music_current_channel = id;
                return true;
            }
        }
        else
        {
            auto channel = getChannel(id);
            if (channel != nullptr)
            {
                channel->load(path);
                return true;
            }
        }
        return false;
    }

    // 0x0040194E
    bool loadChannel(ChannelId id, const char* path, int32_t c)
    {
        return loadChannel(id, fs::u8path(path), c);
    }

    // 0x00401999
    bool playChannel(ChannelId id, int32_t loop, int32_t volume, int32_t d, int32_t freq)
    {
        Console::logVerbose("playChannel(%d, %d, %d, %d, %d)", id, loop, volume, d, freq);
        if (isMusicChannel(id))
        {
            if (_music_channel.play(loop != 0))
            {
                _music_channel.setVolume(volume);
            }
        }
        else
        {
            auto channel = getChannel(id);
            if (channel != nullptr)
            {
                channel->play(loop != 0);
                channel->setVolume(volume);
                return true;
            }
        }
        return false;
    }

    // 0x00401A05
    void stopChannel(ChannelId id)
    {
        Console::logVerbose("stopChannel(%d)", id);
        if (isMusicChannel(id))
        {
            if (_music_current_channel == id)
            {
                _music_channel.stop();
            }
        }
        else
        {
            auto channel = getChannel(id);
            if (channel != nullptr)
            {
                channel->stop();
            }
        }
    }

    // 0x00401AD3
    void setChannelVolume(ChannelId id, int32_t volume)
    {
        Console::logVerbose("setChannelVolume(%d, %d)", id, volume);
        if (isMusicChannel(id))
        {
            if (_music_current_channel == id)
            {
                _music_channel.setVolume(volume);
            }
        }
        else
        {
            auto channel = getChannel(id);
            if (channel != nullptr)
            {
                channel->setVolume(volume);
            }
        }
    }

    // 0x00401B10
    bool isChannelPlaying(ChannelId id)
    {
        if (isMusicChannel(id))
        {
            return _music_current_channel == id && _music_channel.isPlaying();
        }
        else
        {
            auto channel = getChannel(id);
            if (channel != nullptr)
            {
                return channel->isPlaying();
            }
        }
        return false;
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

        if (_numActiveVehicleSounds >= Config::get().max_vehicle_sounds)
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
                v->sound_window_type = main->type;
                v->sound_window_number = main->number;
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
                v->sound_window_type = w->type;
                v->sound_window_number = w->number;
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

    static void sub_48A1FA(int32_t x)
    {
        if (x == 0)
        {
            _numActiveVehicleSounds = 0;
        }

        for (auto v : EntityManager::VehicleList())
        {
            Vehicles::Vehicle train(v);
            off_4FEB58(reinterpret_cast<Vehicles::Vehicle2or6*>(train.veh2), x);
            off_4FEB58(reinterpret_cast<Vehicles::Vehicle2or6*>(train.tail), x);
        }
    }

    // 0x48A73B
    void updateVehicleNoise()
    {
        if (addr<0x00525E28, uint32_t>() & 1)
        {
            if (!_audioIsPaused && _audioIsEnabled)
            {
                sub_48A1FA(0);
                sub_48A1FA(1);
                sub_48A1FA(2);
                for (auto& vc : _vehicle_channels)
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
        for (auto& vc : _vehicle_channels)
        {
            vc.stop();
        }
    }

    // 0x0048ACFD
    void updateAmbientNoise()
    {
        if (!_audio_initialised || _audioIsPaused || !_audioIsEnabled)
            return;

        call(0x0048AD25);
    }

    // 0x0048ABE3
    void stopAmbientNoise()
    {
        loco_global<uint32_t, 0x0050D5AC> _50D5AC;
        if (_audio_initialised && _50D5AC != 1)
        {
            stopChannel(ChannelId::ambient);
            _50D5AC = 1;
        }
    }

    // 0x0048AA0C
    void revalidateCurrentTrack()
    {
        using MusicPlaylistType = Config::MusicPlaylistType;
        auto cfg = Config::get();

        if (_currentSong == no_song)
            return;

        bool trackStillApplies = true;
        switch (cfg.music_playlist)
        {
            case MusicPlaylistType::currentEra:
            {
                auto currentYear = getCurrentYear();
                auto info = _musicInfo[_currentSong];
                if (currentYear < info.start_year || currentYear > info.end_year)
                    trackStillApplies = false;
                break;
            }

            case MusicPlaylistType::all:
                return;

            case MusicPlaylistType::custom:
                if (!cfg.enabled_music[_currentSong])
                    trackStillApplies = false;
                break;
        }

        if (!trackStillApplies)
        {
            stopBackgroundMusic();
            _currentSong = no_song;
            _lastSong = no_song;
        }
    }

    static int32_t chooseNextMusicTrack(int32_t excludeTrack)
    {
        using MusicPlaylistType = Config::MusicPlaylistType;

        static std::vector<uint8_t> playlist;
        playlist.clear();

        auto cfg = Config::get();
        switch (cfg.music_playlist)
        {
            case MusicPlaylistType::currentEra:
            {
                auto currentYear = getCurrentYear();
                for (auto i = 0; i < num_music_tracks; i++)
                {
                    const auto& mi = _musicInfo[i];
                    if (currentYear >= mi.start_year && currentYear <= mi.end_year)
                    {
                        if (i != excludeTrack)
                        {
                            playlist.push_back(i);
                        }
                    }
                }
                break;
            }
            case MusicPlaylistType::all:
                for (auto i = 0; i < num_music_tracks; i++)
                {
                    if (i != excludeTrack)
                    {
                        playlist.push_back(i);
                    }
                }
                break;
            case MusicPlaylistType::custom:
                for (auto i = 0; i < num_music_tracks; i++)
                {
                    if (i != excludeTrack && (cfg.enabled_music[i] & 1))
                    {
                        playlist.push_back(i);
                    }
                }
                break;
        }

        if (playlist.size() == 0)
        {
            if (excludeTrack == no_song)
            {
                for (auto i = 0; i < num_music_tracks; i++)
                {
                    if (i != excludeTrack)
                    {
                        playlist.push_back(i);
                    }
                }
            }
            else
            {
                const auto& mi = _musicInfo[excludeTrack];
                auto currentYear = getCurrentYear();
                if (currentYear >= mi.start_year && currentYear <= mi.end_year)
                {
                    playlist.push_back(excludeTrack);
                }
            }
        }

        auto r = std::rand() % playlist.size();
        auto track = playlist[r];
        return track;
    }

    // 0x0048A78D
    void playBackgroundMusic()
    {
        if (!_audio_initialised || _audioIsPaused || !_audioIsEnabled)
        {
            return;
        }

        auto cfg = Config::get();
        if (cfg.music_playing == 0 || isTitleMode() || isEditorMode())
        {
            return;
        }

        if (!_music_channel.isPlaying())
        {
            // Not playing, but the 'current song' is last song? It's been requested manually!
            bool requestedSong = _lastSong != no_song && _lastSong == _currentSong;

            // Choose a track to play, unless we have requested one track in particular.
            if (_currentSong == no_song || !requestedSong)
            {
                auto trackToExclude = _lastSong;
                _lastSong = _currentSong;
                _currentSong = chooseNextMusicTrack(trackToExclude);
            }
            else
            {
                // We're choosing this one, but the next one should be decided automatically again.
                _lastSong = no_song;
            }

            // Load info on the song to play.
            const auto& mi = _musicInfo[_currentSong];
            auto path = Environment::getPath((path_id)mi.path_id);
            if (_music_channel.load(path))
            {
                _music_current_channel = ChannelId::bgm;
                if (!_music_channel.play(false))
                {
                    cfg.music_playing = 0;
                }
            }
            else
            {
                cfg.music_playing = 0;
            }

            WindowManager::invalidate(WindowType::options);
        }
    }

    // 0x0048AAD2
    void resetMusic()
    {
        stopBackgroundMusic();
        _currentSong = no_song;
        _lastSong = no_song;
    }

    // 0x0048AAE8
    void stopBackgroundMusic()
    {
        if (_audio_initialised && _music_channel.isPlaying())
        {
            _music_channel.stop();
        }
    }

    // 0x0048AC66
    void playTitleScreenMusic()
    {
        if (isTitleMode() && _audio_initialised && _audioIsEnabled && Config::getNew().audio.play_title_music)
        {
            if (!isChannelPlaying(ChannelId::title))
            {
                auto path = Environment::getPath(path_id::css5);
                if (loadChannel(ChannelId::title, path, 0))
                {
                    playChannel(ChannelId::title, 1, -500, 0, 0);
                }
            }
        }
        else
        {
            if (isChannelPlaying(ChannelId::title))
            {
                stopChannel(ChannelId::title);
            }
        }
    }

    // 0x0048AC2B
    void stopTitleMusic()
    {
        if (isChannelPlaying(ChannelId::title))
        {
            stopChannel(ChannelId::title);
        }
    }

    bool isAudioEnabled()
    {
        return _audioIsEnabled;
    }

    const MusicInfo* getMusicInfo(MusicId track)
    {
        return &_musicInfo[track];
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

        if (_audio_initialised && _currentSong != no_song && isChannelPlaying(ChannelId::bgm))
        {
            setChannelVolume(ChannelId::bgm, volume);
        }
    }
}
