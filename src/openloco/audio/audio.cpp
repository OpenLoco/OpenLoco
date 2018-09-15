#include "audio.h"
#include "../config.h"
#include "../console.h"
#include "../environment.h"
#include "../interop/interop.hpp"
#include "../map/tilemgr.h"
#include "../objects/objectmgr.h"
#include "../objects/sound_object.h"
#include "../things/thingmgr.h"
#include "../things/vehicle.h"
#include "../utility/stream.hpp"
#include "../windowmgr.h"
#include "channel.h"
#include "music_channel.h"
#include "vehicle_channel.h"
#include <SDL2/SDL_mixer.h>
#include <array>
#include <cassert>
#include <fstream>
#include <unordered_map>

using namespace openloco::environment;
using namespace openloco::interop;
using namespace openloco::ui;
using namespace openloco::utility;

namespace openloco::audio
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

    struct sound_object_data
    {
        int32_t var_00;
        int32_t offset;
        uint32_t length;
        WAVEFORMATEX pcm_header;

        const void* pcm()
        {
            return (void*)((uintptr_t)this + sizeof(sound_object_data));
        }
    };
#pragma pack(pop)

    struct audio_format
    {
        int32_t frequency{};
        int32_t format{};
        int32_t channels{};
    };

    [[maybe_unused]] constexpr int32_t play_at_centre = 0x8000;
    constexpr int32_t play_at_location = 0x8001;
    constexpr int32_t num_sound_channels = 16;

    static loco_global<uint32_t, 0x0050D1EC> _audio_initialised;

    static audio_format _outputFormat;
    static std::array<channel, 4> _channels;
    static std::array<vehicle_channel, 10> _vehicle_channels;
    static music_channel _music_channel;
    static channel_id _music_current_channel = channel_id::bgm;

    static std::vector<sample> _samples;
    static std::unordered_map<uint16_t, sample> _object_samples;

    static void play_sound(sound_id id, loc16 loc, int32_t volume, int32_t pan, int32_t frequency);
    static void mix_sound(sound_id id, bool loop, int32_t volume, int32_t pan, int32_t freq);

    static constexpr bool is_music_channel(channel_id id)
    {
        return (id == channel_id::bgm || id == channel_id::title);
    }

    int32_t volume_loco_to_sdl(int32_t loco)
    {
        return (int)(SDL_MIX_MAXVOLUME * (SDL_pow(10, (float)loco / 2000)));
    }

    static channel* get_channel(channel_id id)
    {
        auto index = (size_t)id;
        if (index < _channels.size())
        {
            return &_channels[index];
        }
        return nullptr;
    }

    static sample load_sound_from_wave_memory(const WAVEFORMATEX& format, const void* pcm, size_t pcmLen)
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
            console::error("Error during SDL_BuildAudioCVT: %s", SDL_GetError());
            return {};
        }
        else if (cr == 0)
        {
            // No conversion necessary
            sample s;
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
                console::error("Error during SDL_ConvertAudio: %s", SDL_GetError());
                return {};
            }

            // Trim converted buffer
            cvt.buf = (uint8_t*)std::realloc(cvt.buf, cvt.len_cvt);
            if (cvt.buf == nullptr)
            {
                throw std::bad_alloc();
            }

            sample s;
            s.pcm = cvt.buf;
            s.len = cvt.len_cvt;
            s.chunk = Mix_QuickLoad_RAW(cvt.buf, cvt.len_cvt);
            return s;
        }
    }

    static std::vector<sample> load_sounds_from_css(const fs::path& path)
    {
        console::log("load_sounds_from_css(%s)", path.string().c_str());
        std::vector<sample> results;
#ifdef _OPENLOCO_USE_BOOST_FS_
        std::ifstream fs(path.string(), std::ios::in | std::ios::binary);
#else
        std::ifstream fs(path, std::ios::in | std::ios::binary);
#endif

        if (fs.is_open())
        {
            auto numSounds = read_value<uint32_t>(fs);

            std::vector<uint32_t> offsets(numSounds, 0);
            read_data(fs, offsets.data(), numSounds);

            std::vector<std::byte> pcm;
            for (uint32_t i = 0; i < numSounds; i++)
            {
                // Navigate to beginning of wave data
                fs.seekg(offsets[i]);

                // Read length of wave data and load it into the pcm buffer
                auto pcmLen = read_value<uint32_t>(fs);
                auto format = read_value<WAVEFORMATEX>(fs);

                pcm.resize(pcmLen);
                read_data(fs, pcm.data(), pcmLen);

                auto s = load_sound_from_wave_memory(format, pcm.data(), pcmLen);
                results.push_back(s);
            }
        }
        return results;
    }

    static void dispose_samples()
    {
        for (size_t i = 0; i < _samples.size(); i++)
        {
            Mix_FreeChunk(_samples[i].chunk);
            std::free(_samples[i].pcm);
        }
        _samples = {};
    }

    static void dispose_channels()
    {
        std::generate(_channels.begin(), _channels.end(), []() { return channel(); });
        std::generate(_vehicle_channels.begin(), _vehicle_channels.end(), []() { return vehicle_channel(); });
    }

    // 0x00404E53
    void initialise_dsound()
    {
        auto& format = _outputFormat;
        format.frequency = MIX_DEFAULT_FREQUENCY;
        format.format = MIX_DEFAULT_FORMAT;
        format.channels = MIX_DEFAULT_CHANNELS;
        if (Mix_OpenAudio(format.frequency, format.format, format.channels, 1024) != 0)
        {
            console::error("Mix_OpenAudio failed: %s", Mix_GetError());
            return;
        }
        Mix_AllocateChannels(num_reserved_channels + num_sound_channels);
        Mix_ReserveChannels(num_reserved_channels);

        for (size_t i = 0; i < _channels.size(); i++)
        {
            _channels[i] = channel(i);
        }
        for (size_t i = 0; i < _vehicle_channels.size(); i++)
        {
            _vehicle_channels[i] = vehicle_channel(channel(4 + i));
        }
    }

    // 0x00404E58
    void dispose_dsound()
    {
        dispose_samples();
        dispose_channels();
        _music_channel = {};
        Mix_CloseAudio();
    }

    // 0x004899E4
    void initialise()
    {
        auto css1path = environment::get_path(environment::path_id::css1);
        _samples = load_sounds_from_css(css1path);
        _audio_initialised = 1;
    }

    // 0x00489C34
    void pause_sound()
    {
        call(0x00489C34);
    }

    // 0x00489C58
    void unpause_sound()
    {
        call(0x00489C58);
    }

    static sound_object* get_sound_object(sound_id id)
    {
        auto idx = (int32_t)id & ~0x8000;
        return objectmgr::get<sound_object>(idx);
    }

    static viewport* find_best_viewport_for_sound(viewport_pos vpos)
    {
        auto w = windowmgr::find(window_type::main, 0);
        if (w != nullptr)
        {
            auto viewport = w->viewports[0];
            if (viewport != nullptr && viewport->contains(vpos))
            {
                return viewport;
            }
        }

        for (auto i = (int32_t)windowmgr::num_windows(); i >= 0; i--)
        {
            w = windowmgr::get(i);
            if (w != nullptr && w->type != window_type::main && w->type != window_type::unk_36)
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

    static int32_t get_volume_for_sound_id(sound_id id)
    {
        if (is_object_sound_id(id))
        {
            auto obj = get_sound_object(id);
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

    static int32_t calculate_volume_from_viewport(sound_id id, const map::map_pos3& mpos, const viewport& viewport)
    {
        auto volume = 0;
        auto zVol = 0;
        auto tile = map::tilemgr::get(mpos);
        if (!tile.is_null())
        {
            auto surface = tile.surface();
            if (surface != nullptr)
            {
                if ((surface->base_z() * 4) - 5 > mpos.z)
                {
                    zVol = 8;
                }
            }
            volume = ((-1024 * viewport.zoom - 1) << zVol) + 1;
        }
        return volume;
    }

    void play_sound(sound_id id, loc16 loc)
    {
        play_sound(id, loc, play_at_location);
    }

    void play_sound(sound_id id, int32_t pan)
    {
        play_sound(id, {}, play_at_location);
    }

    static vehicle_channel* get_free_vehicle_channel()
    {
        for (auto& vc : _vehicle_channels)
        {
            if (vc.is_free())
            {
                return &vc;
            }
        }
        return nullptr;
    }

    bool should_sound_loop(sound_id id)
    {
        loco_global<uint8_t[64], 0x0050D514> unk_50D514;
        if (is_object_sound_id(id))
        {
            auto obj = get_sound_object(id);
            return obj->var_06 != 0;
        }
        else
        {
            return unk_50D514[(int32_t)id * 2] != 0;
        }
    }

    // 0x0048A4BF
    void play_sound(vehicle* v)
    {
        if (v->var_4A & 1)
        {
            console::log("play_sound(vehicle #%d)", v->object_id);
            auto vc = get_free_vehicle_channel();
            if (vc != nullptr)
            {
                vc->begin(v->id);
            }
        }
    }

    // 0x00489F1B
    void play_sound(sound_id id, loc16 loc, int32_t volume, int32_t frequency)
    {
        play_sound(id, loc, volume, play_at_location, frequency);
    }

    // 0x00489CB5
    void play_sound(sound_id id, loc16 loc, int32_t pan)
    {
        play_sound(id, loc, 0, pan, 0);
    }

    // 0x00489CB5 / 0x00489F1B
    // pan is in UI pixels or known constant
    void play_sound(sound_id id, loc16 loc, int32_t volume, int32_t pan, int32_t frequency)
    {
        loco_global<uint8_t, 0x0050D555> unk_50D555;
        loco_global<int32_t, 0x00e3f0b8> current_rotation;

        if (unk_50D555 & 1)
        {
            volume += get_volume_for_sound_id(id);
            if (pan == play_at_location)
            {
                auto vpos = viewport::map_from_3d(loc, current_rotation);
                auto viewport = find_best_viewport_for_sound(vpos);
                if (viewport == nullptr)
                {
                    return;
                }

                volume += calculate_volume_from_viewport(id, { loc.x, loc.y }, *viewport);
                pan = viewport->map_to_ui(vpos).x;
                if (volume < -10000)
                {
                    return;
                }
            }
            else if (pan == play_at_centre)
            {
                pan = 0;
            }

            const auto& cfg = config::get();
            if (cfg.var_1E == 0)
            {
                pan = 0;
            }
            else if (pan != 0)
            {
                auto uiWidth = std::max(64, ui::width());
                pan = (((pan << 16) / uiWidth) - 0x8000) >> 4;
            }

            mix_sound(id, 0, volume, pan, frequency);
        }
    }

    sample* get_sound_sample(sound_id id)
    {
        if (is_object_sound_id(id))
        {
            // TODO use a LRU queue for object samples
            auto sr = _object_samples.find((uint16_t)id);
            if (sr == _object_samples.end())
            {
                auto obj = get_sound_object(id);
                if (obj != nullptr)
                {
                    auto data = (sound_object_data*)obj->data;
                    assert(data->offset == 8);
                    auto sample = load_sound_from_wave_memory(data->pcm_header, data->pcm(), data->length);
                    _object_samples[(int16_t)id] = sample;
                    return &_object_samples[(int16_t)id];
                }
            }
            else
            {
                return &sr->second;
            }
        }
        else if ((int32_t)id >= 0 && (int32_t)id < (int32_t)_samples.size())
        {
            return &_samples[(int32_t)id];
        }
        return nullptr;
    }

    static void mix_sound(sound_id id, bool loop, int32_t volume, int32_t pan, int32_t freq)
    {
        console::log("mix_sound(%d, %s, %d, %d, %d)", (int32_t)id, loop ? "true" : "false", volume, pan, freq);
        auto sample = get_sound_sample(id);
        if (sample != nullptr && sample->chunk != nullptr)
        {
            auto loops = loop == 0 ? 0 : -1;
            auto channel = Mix_PlayChannel(-1, sample->chunk, loops);
            auto sdlv = volume_loco_to_sdl(volume);
            Mix_Volume(channel, sdlv);

            // clang-format off
            auto [left, right] = pan_loco_to_sdl(pan);
            // clang-format on
            Mix_SetPanning(channel, left, right);
        }
    }

    // 0x0048A18C
    void update_sounds()
    {
    }

    // 0x0040194E
    bool load_channel(channel_id id, const char* path, int32_t c)
    {
        console::log("load_channel(%d, %s, %d)", id, path, c);
        if (is_music_channel(id))
        {
            if (_music_channel.load(path))
            {
                _music_current_channel = id;
                return true;
            }
        }
        else
        {
            auto channel = get_channel(id);
            if (channel != nullptr)
            {
                channel->load(path);
                return true;
            }
        }
        return false;
    }

    // 0x00401999
    bool play_channel(channel_id id, int32_t loop, int32_t volume, int32_t d, int32_t freq)
    {
        console::log("play_channel(%d, %d, %d, %d, %d)", id, loop, volume, d, freq);
        if (is_music_channel(id))
        {
            if (_music_channel.play(loop != 0))
            {
                _music_channel.set_volume(volume);
            }
        }
        else
        {
            auto channel = get_channel(id);
            if (channel != nullptr)
            {
                channel->play(loop != 0);
                channel->set_volume(volume);
                return true;
            }
        }
        return false;
    }

    // 0x00401A05
    void stop_channel(channel_id id)
    {
        console::log("stop_channel(%d)", id);
        if (is_music_channel(id))
        {
            if (_music_current_channel == id)
            {
                _music_channel.stop();
            }
        }
        else
        {
            auto channel = get_channel(id);
            if (channel != nullptr)
            {
                channel->stop();
            }
        }
    }

    // 0x00401AD3
    void set_channel_volume(channel_id id, int32_t volume)
    {
        console::log("set_channel_volume(%d, %d)", id, volume);
        if (is_music_channel(id))
        {
            if (_music_current_channel == id)
            {
                _music_channel.set_volume(volume);
            }
        }
        else
        {
            auto channel = get_channel(id);
            if (channel != nullptr)
            {
                channel->set_volume(volume);
            }
        }
    }

    // 0x00401B10
    bool is_channel_playing(channel_id id)
    {
        if (is_music_channel(id))
        {
            return _music_current_channel == id && _music_channel.is_playing();
        }
        else
        {
            auto channel = get_channel(id);
            if (channel != nullptr)
            {
                return channel->is_playing();
            }
        }
        return false;
    }

    static void sub_48A274(vehicle* v)
    {
        registers regs;
        regs.esi = (int32_t)v;
        call(0x0048A274, regs);
    }

    static void off_4FEB58(vehicle* v, int32_t x)
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
                play_sound(v);
                break;
        }
    }

    static void sub_48A1FA(int32_t x)
    {
        if (x == 0)
        {
            addr<0x0112C666, uint8_t>() = 0;
        }

        auto v = thingmgr::first<vehicle>();
        while (v != nullptr)
        {
            auto next = v->next_vehicle();
            auto v2 = v->next_car()->next_car();
            off_4FEB58(v2, x);
            do
            {
                v2 = v2->next_car();
            } while (v2->type != thing_type::vehicle_6);
            off_4FEB58(v2, x);
            v = next;
        }
    }

    // 0x48A73B
    void update_vehicle_noise()
    {
        if (addr<0x00525E28, uint32_t>() & 1)
        {
            if (addr<0x0050D554, uint8_t>() == 0 && (addr<0x0050D555, uint8_t>() & 1))
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
    void stop_vehicle_noise()
    {
        for (auto& vc : _vehicle_channels)
        {
            vc.stop();
        }
    }

    // 0x0048ACFD
    void update_ambient_noise()
    {
        call(0x0048ACFD);
    }

    // 0x0048A78D
    void play_background_music()
    {
        call(0x0048A78D);
    }

    // 0x0048AAE8
    void stop_background_music()
    {
        call(0x0048AAE8);
    }

    // 0x0048AC66
    void play_title_screen_music()
    {
        call(0x0048AC66);
    }
}
