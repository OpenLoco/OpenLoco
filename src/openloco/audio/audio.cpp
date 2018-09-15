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
#include "vehicle_channel.h"
#include <SDL2/SDL_mixer.h>
#include <array>
#include <cassert>
#include <fstream>
#include <unordered_map>

#define __USE_NEW_MIXER__

using namespace openloco::environment;
using namespace openloco::interop;
using namespace openloco::ui;
using namespace openloco::utility;

namespace openloco::audio
{
    constexpr int16_t sound_entry_null = -1;

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

    struct sound_instance
    {
        uint16_t id;
        uint8_t pad_02[20 - 2];
    };
    static_assert(sizeof(sound_instance) == 20);

    struct sound_entry
    {
        int16_t id;
        sound_instance sound;

        void free() { id = sound_entry_null; }
        constexpr bool is_free() { return id == sound_entry_null; }
    };
    static_assert(sizeof(sound_entry) == 22);

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
    static loco_global<sound_entry[10], 0x0050D438> _sound_entries;

    static audio_format _outputFormat;
    static std::array<channel, 4> _channels;
    static std::array<vehicle_channel, 10> _vehicle_channels;
    static std::vector<sample> _samples;
    static std::unordered_map<uint16_t, sample> _object_samples;
    static Mix_Music* _music_track;
    static int32_t _current_music = -1;

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

    static void dispose_music()
    {
        Mix_FreeMusic(_music_track);
        _music_track = nullptr;
        _current_music = -1;
    }

    // 0x00404E53
    void initialise_dsound()
    {
#ifdef __USE_OLD_CODE__
        call(0x00404E53);
#endif

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
#ifdef __USE_OLD_CODE__
        call(0x00404E58);
#endif

        dispose_samples();
        dispose_channels();
        dispose_music();
        Mix_CloseAudio();
    }

    // 0x004899E4
    void initialise()
    {
#ifdef __USE_OLD_CODE__
        call(0x004899E4);
#endif

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

#ifndef __USE_OLD_CODE__
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
#endif

#ifndef __USE_NEW_MIXER__
    static sound_entry* get_free_sound_entry()
    {
        const auto& cfg = config::get();
        const auto max_sounds = std::min<size_t>(_sound_entries.size(), cfg.max_sound_instances);
        for (size_t i = 0; i < max_sounds; i++)
        {
            auto& entry = _sound_entries[i];
            if (entry.is_free())
            {
                return &entry;
            }
        }
        return nullptr;
    }
#endif

    void play_sound(sound_id id, loc16 loc)
    {
        play_sound(id, loc, play_at_location);
    }

    void play_sound(sound_id id, int32_t pan)
    {
        play_sound(id, {}, play_at_location);
    }

    static vehicle_channel* get_free_vehicle_sound_instance()
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
            auto vc = get_free_vehicle_sound_instance();
            if (vc != nullptr)
            {
                vc->begin(v->id);
            }
        }
    }

    // 0x00489F1B
    void play_sound(sound_id id, loc16 loc, int32_t volume, int32_t frequency)
    {
#ifdef __USE_OLD_CODE__
        registers regs;
        regs.eax = (int32_t)id;
        regs.ecx = loc.x;
        regs.edx = loc.y;
        regs.ebp = loc.z;
        regs.ebx = frequency;
        regs.edi = volume;
        call(0x00489F1B, regs);
#else
        play_sound(id, loc, volume, play_at_location, frequency);
#endif
    }

    // 0x00489CB5
    void play_sound(sound_id id, loc16 loc, int32_t pan)
    {
#ifdef __USE_OLD_CODE__
        registers regs;
        regs.eax = (int32_t)id;
        regs.cx = loc.x;
        regs.dx = loc.y;
        regs.bp = loc.z;
        regs.ebx = pan;
        call(0x00489CB5, regs);
#else
        play_sound(id, loc, 0, pan, 0);
#endif
    }

#ifndef __USE_OLD_CODE__
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

#ifdef __USE_NEW_MIXER__
            mix_sound(id, 0, volume, pan, frequency);
#else
            auto entry = get_free_sound_entry();
            if (entry != nullptr)
            {
                entry->id = (int16_t)id;
                prepare_sound(id, &entry->sound, 1, cfg.force_software_audio_mixer);
                mix_sound(&entry->sound, 0, volume, pan, frequency);
            }
#endif
        }
    }
#endif

    // 0x00404B68
    bool prepare_sound(sound_id id, sound_instance* sound, int32_t channels, int32_t software)
    {
        console::log("prepare_sound(%d, 0x%X, %d, %d)", id, (int32_t)sound, channels, software);
        sound->id = (uint16_t)id;
        return false;
    }

    // 0x00404D7A
    void mix_sound(sound_instance* sound, int32_t b, int32_t volume, int32_t pan, int32_t freq)
    {
        mix_sound((sound_id)sound->id, b, volume, pan, freq);
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

    // 0x00404CD3
    void stop_sound(sound_instance* sound)
    {
    }

    // 0x0040194E
    bool load_channel(channel_id id, const char* path, int32_t c)
    {
        console::log("load_channel(%d, %s, %d)", id, path, c);
        if (is_music_channel(id))
        {
            auto music = Mix_LoadMUS(path);
            if (music != nullptr)
            {
                dispose_music();
                _music_track = music;
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
            if (_music_track != nullptr)
            {
                auto loops = loop ? -1 : 1;
                if (Mix_PlayMusic(_music_track, loops) == 0)
                {
                    set_channel_volume(id, volume);
                    _current_music = (int32_t)id;
                    return true;
                }
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
            if (_current_music == (int32_t)id)
            {
                Mix_HaltMusic();
                _current_music = -1;
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
            Mix_VolumeMusic(volume_loco_to_sdl(volume));
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
            if (_current_music == (int32_t)id)
            {
                return Mix_PlayingMusic() != 0;
            }
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

    static void off_4FEB58(vehicle* v, int32_t x)
    {
        registers regs;
        regs.esi = (int32_t)v;
        switch (x)
        {
            case 0:
                call(0x0048A262, regs);
                break;
            case 1:
                call(0x0048A268, regs);
                break;
            case 2:
                call(0x0048A395, regs);
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

    // 0x0048A3A2
    static void update_vehicle_sounds()
    {
        for (auto& vc : _vehicle_channels)
        {
            vc.update();
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
                update_vehicle_sounds();
                sub_48A1FA(3);
            }
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
