#include "audio.h"
#include "../console.h"
#include "../environment.h"
#include "../interop/interop.hpp"
#include "../utility/stream.hpp"
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

    [[maybe_unused]] constexpr int32_t play_at_centre = 0x8000;
    constexpr int32_t play_at_location = 0x8001;

    static loco_global<uint32_t, 0x0050D1EC> _audio_initialised;
    static std::vector<Mix_Chunk*> _samples;
    static Mix_Music* _music_track;
    static int32_t _current_music = -1;

    static constexpr int32_t volume_loco_to_sdl(int32_t loco)
    {
        auto ratio = std::clamp(0.0f, (loco + 2560) / 2560.0f, 1.0f);
        auto vol = (int32_t)(ratio * SDL_MIX_MAXVOLUME);
        return vol;
    }

    static Mix_Chunk* load_sound_from_wave_memory(const WAVEFORMATEX& format, void* pcm, size_t pcmLen)
    {
        auto mem = (uint8_t*)std::malloc(pcmLen);
        std::memcpy(mem, pcm, pcmLen);
        return Mix_QuickLoad_RAW(mem, pcmLen);
    }

    static std::vector<Mix_Chunk*> load_sounds_from_css(const fs::path& path)
    {
        std::vector<Mix_Chunk*> results;
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

                auto chunk = load_sound_from_wave_memory(format, pcm.data(), pcmLen);
                results.push_back(chunk);
            }
        }
        return results;
    }

    static void dispose_samples()
    {
        for (size_t i = 0; i < _samples.size(); i++)
        {
            Mix_FreeChunk(_samples[i]);
        }
        _samples.clear();
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

        if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) != 0)
        {
            console::error("Mix_OpenAudio failed: %s", Mix_GetError());
            return;
        }
        Mix_AllocateChannels(16);
    }

    // 0x00404E58
    void dispose_dsound()
    {
        // call(0x00404E58);

        dispose_samples();
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
    bool prepare_sound(int32_t soundId, sound_instance* sound, int32_t channels, int32_t software)
    {
        console::log("prepare_sound(%d, 0x%X, %d, %d)", soundId, (int32_t)sound, channels, software);
        return false;
    }

    // 0x00404D7A
    void mix_sound(sound_instance* sound, int32_t b, int32_t volume, int32_t pan, int32_t freq)
    {
        console::log("mix_sound(0x%Xz, %d, %d, %d, %d)", (int32_t)sound, b, volume, pan, freq);
        if (b >= 0 && b < (int32_t)_samples.size())
        {
            Mix_PlayChannel(-1, _samples[b], 1);
        }
    }

    // 0x0040194E
    bool load_music(int32_t id, const char* path, int32_t c)
    {
        console::log("load_music(%d, %s, %d)", id, path, c);
        auto music = Mix_LoadMUS(path);
        if (music != nullptr)
        {
            dispose_music();
            _music_track = music;
            return true;
        }
        else
        {
            return false;
        }
    }

    // 0x00401999
    bool play_music(int32_t id, int32_t loop, int32_t volume, int32_t d, int32_t freq)
    {
        console::log("play_music(%d, %d, %d, %d, %d)", id, loop, volume, d, freq);
        if (_music_track != nullptr)
        {
            auto loops = loop ? -1 : 1;
            if (Mix_PlayMusic(_music_track, loops) == 0)
            {
                set_music_volume(id, volume);
                _current_music = id;
                return true;
            }
        }
        return false;
    }

    // 0x00401A05
    void stop_music(int32_t id)
    {
        console::log("stop_music(%d)", id);
        if (_current_music == id)
        {
            Mix_HaltMusic();
            _current_music = -1;
        }
    }

    // 0x00401AD3
    void set_music_volume(int32_t id, int32_t volume)
    {
        console::log("set_music_volume(%d, %d)", id, volume);
        Mix_VolumeMusic(volume_loco_to_sdl(volume));
    }

    // 0x00401B10
    bool is_music_playing(int32_t id)
    {
        if (_current_music == id)
        {
            return Mix_PlayingMusic() != 0;
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
