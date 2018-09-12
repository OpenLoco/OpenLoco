#include "audio.h"
#include "../console.h"
#include "../environment.h"
#include "../interop/interop.hpp"
#include "../utility/stream.hpp"
#include "channel.h"
#include <SDL2/SDL_mixer.h>
#include <fstream>

using namespace openloco::environment;
using namespace openloco::interop;
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

    struct sound_instance
    {
        void* dsbuffer;
        uint16_t id;
        uint16_t var_8;
        int32_t has_caps;
        int32_t var_0C;
        sound_instance* next;
    };
#pragma pack(pop)

    struct audio_format
    {
        int32_t frequency{};
        int32_t format{};
        int32_t channels{};
    };

    struct sample
    {
        void* pcm{};
        size_t len{};
        Mix_Chunk* chunk{};
    };

    [[maybe_unused]] constexpr int32_t play_at_centre = 0x8000;
    constexpr int32_t play_at_location = 0x8001;
    constexpr int32_t num_sound_channels = 16;

    static loco_global<uint32_t, 0x0050D1EC> _audio_initialised;

    static audio_format _outputFormat;
    static std::vector<channel> _channels;
    static std::vector<sample> _samples;
    static Mix_Music* _music_track;
    static int32_t _current_music = -1;

    static constexpr bool is_music_channel(channel_id id)
    {
        return (id == channel_id::bgm || id == channel_id::title);
    }

    static channel* get_channel(channel_id id)
    {
        while ((int32_t)_channels.size() <= (int32_t)id)
        {
            _channels.emplace_back((int32_t)_channels.size());
        }
        return &_channels[(int32_t)id];
    }

    int32_t volume_loco_to_sdl(int32_t loco)
    {
        constexpr auto range = 3500.0f;
        auto ratio = std::clamp(0.0f, (loco + range) / range, 1.0f);
        auto vol = (int32_t)(ratio * SDL_MIX_MAXVOLUME);
        return vol;
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
        std::ifstream fs(path, std::ios::in | std::ios::binary);
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
        _channels.clear();
        _channels.shrink_to_fit();
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
        // call(0x00404E53);

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
    }

    // 0x00404E58
    void dispose_dsound()
    {
        // call(0x00404E58);

        dispose_samples();
        dispose_channels();
        dispose_music();
        Mix_CloseAudio();
    }

    // 0x004899E4
    void initialise()
    {
        // call(0x004899E4);

        auto css1path = environment::get_path(environment::path_id::css1);
        _samples = load_sounds_from_css(css1path);
        _audio_initialised = 1;
    }

    // 0x00489C34
    void pause_sound()
    {
        // call(0x00489C34);
    }

    // 0x00489C58
    void unpause_sound()
    {
        // call(0x00489C58);
    }

    void play_sound(sound_id id, loc16 loc)
    {
        play_sound(id, loc, play_at_location);
    }

    void play_sound(sound_id id, int32_t pan)
    {
        play_sound(id, {}, play_at_location);
    }

    // 0x00489F1B
    void play_sound(sound_id id, loc16 loc, int32_t volume, int32_t frequency, bool obj_sound)
    {
        registers regs;
        regs.eax = (int32_t)id;
        regs.eax |= obj_sound ? 0x8000 : 0;
        regs.ecx = loc.x;
        regs.edx = loc.y;
        regs.ebp = loc.z;
        regs.ebx = frequency;
        regs.edi = volume;
        call(0x00489F1B, regs);
    }

    // 0x00489CB5
    void play_sound(sound_id id, loc16 loc, int32_t pan)
    {
        registers regs;
        regs.eax = (int32_t)id;
        regs.cx = loc.x;
        regs.dx = loc.y;
        regs.bp = loc.z;
        regs.ebx = pan;
        call(0x00489CB5, regs);
    }

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
        console::log("mix_sound(0x%X, %d, %d, %d, %d)", (int32_t)sound, b, volume, pan, freq);

        auto id = sound->id;
        if (id >= 0 && id < (int32_t)_samples.size())
        {
            auto& sample = _samples[id];
            if (sample.chunk != nullptr)
            {
                auto channel = Mix_PlayChannel(-1, sample.chunk, 0);
                Mix_Volume(channel, volume_loco_to_sdl(volume));

                auto [left, right] = pan_loco_to_sdl(pan);
                Mix_SetPanning(channel, left, right);
            }
        }
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
        console::log("play_music(%d, %d, %d, %d, %d)", id, loop, volume, d, freq);
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
        console::log("stop_music(%d)", id);
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
        console::log("set_music_volume(%d, %d)", id, volume);
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

    // 0x0048AC66
    void play_title_screen_music()
    {
        call(0x0048AC66);
    }
}
